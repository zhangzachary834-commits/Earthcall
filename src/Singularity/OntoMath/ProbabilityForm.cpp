#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include <random>

namespace OntoMath {
namespace Probability {

// Thread-local random number generator to avoid lock contention during parallel evaluations
static thread_local std::mt19937 generator(std::random_device{}());

static Piecewise makeMomentIntegral(const Piecewise& pdf, const std::string& varName, int power) {
    Piecewise out = pdf;
    
    auto varNode = std::make_shared<MathNode>();
    varNode->op = MathNode::Op::ValueLeaf;
    varNode->variableName = varName;
    
    std::shared_ptr<MathNode> multFactor = varNode;
    if (power == 2) {
        auto p2 = std::make_shared<MathNode>();
        p2->op = MathNode::Op::Scale;
        p2->children.push_back(std::make_unique<MathNode>(*varNode));
        p2->children.push_back(std::make_unique<MathNode>(*varNode));
        multFactor = p2;
    }

    for (auto& piece : out.pieces) {
        if (!piece.mathNode) continue;
        auto multNode = std::make_shared<MathNode>();
        multNode->op = MathNode::Op::Scale;
        multNode->children.push_back(std::make_unique<MathNode>(*multFactor));
        multNode->children.push_back(std::make_unique<MathNode>(*piece.mathNode));
        piece.mathNode = multNode;
    }
    return out;
}

// Adaptive MCMC for Multivariate Symbolic Distributions
static std::vector<double> runAdaptiveMCMC(const Piecewise& pdf, const std::vector<std::string>& varNames, int steps, std::vector<double>* outExpected = nullptr, std::vector<double>* outCovariance = nullptr) {
    int N = varNames.size();
    if (N == 0) return {};
    
    std::vector<double> currentX(N, 0.0);
    std::vector<double> sigma(N, 1.0);
    std::normal_distribution<double> norm(0.0, 1.0);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    auto evalPdf = [&](const std::vector<double>& x) -> double {
        std::map<std::string, PropertyValue> vars;
        for (int i=0; i<N; ++i) vars[varNames[i]] = PropertyValue(x[i]);
        auto res = pdf.evaluate(vars);
        if (!res) return 0.0;
        double val = 0.0;
        propertyValueToNumber(*res, val);
        return std::max(0.0, val);
    };

    double currentP = evalPdf(currentX);
    
    std::vector<double> sumX(N, 0.0);
    std::vector<double> sumXX(N * N, 0.0);
    int validSamples = 0;

    for (int i = 0; i < steps; ++i) {
        std::vector<double> candX(N);
        for (int d = 0; d < N; ++d) candX[d] = currentX[d] + norm(generator) * sigma[d];
        
        double candP = evalPdf(candX);
        if (candP > 0.0) {
            double alpha = (currentP > 0.0) ? (candP / currentP) : 2.0;
            if (alpha >= 1.0 || uniform(generator) < alpha) {
                currentX = candX;
                currentP = candP;
            }
        }
        
        // Accumulate for moments on the second half of the chain
        if (i > steps / 2) {
            for (int d = 0; d < N; ++d) sumX[d] += currentX[d];
            for (int r = 0; r < N; ++r) {
                for (int c = 0; c < N; ++c) {
                    sumXX[r * N + c] += currentX[r] * currentX[c];
                }
            }
            validSamples++;
            
            // Adapt independent sigmas slightly
            for (int d = 0; d < N; ++d) {
                double mean = sumX[d] / validSamples;
                double var = (sumXX[d * N + d] / validSamples) - (mean * mean);
                if (var > 0.0) sigma[d] = std::sqrt(var);
            }
        }
    }
    
    if (validSamples > 0) {
        if (outExpected) {
            outExpected->resize(N);
            for (int d = 0; d < N; ++d) (*outExpected)[d] = sumX[d] / validSamples;
        }
        if (outCovariance) {
            outCovariance->resize(N * N);
            for (int r = 0; r < N; ++r) {
                for (int c = 0; c < N; ++c) {
                    double meanR = sumX[r] / validSamples;
                    double meanC = sumX[c] / validSamples;
                    (*outCovariance)[r * N + c] = (sumXX[r * N + c] / validSamples) - (meanR * meanC);
                }
            }
        }
    }
    
    return currentX;
}


std::vector<double> Distribution::sample() const {
    if (kind == Kind::Symbolic) {
        if (!pdf || variableNames.empty()) return {};

        // For 1D we use exact moments to drive a fast MCMC
        if (variableNames.size() == 1) {
            double currentX = expectedValue();
            if (std::isnan(currentX)) currentX = 0.0;

            double var = variance();
            double sigma = (std::isnan(var) || var <= 0.0) ? 1.0 : std::sqrt(var);
            
            std::normal_distribution<double> proposalDist(0.0, sigma);
            std::uniform_real_distribution<double> uniformDist(0.0, 1.0);

            auto evalPdf = [&](double x) -> double {
                std::map<std::string, PropertyValue> vars;
                vars[variableNames[0]] = PropertyValue(x);
                auto res = pdf->evaluate(vars);
                if (!res) return 0.0;
                double val = 0.0;
                propertyValueToNumber(*res, val);
                return std::max(0.0, val);
            };

            double currentP = evalPdf(currentX);

            if (currentP <= 0.0) {
                for (const auto& piece : pdf->pieces) {
                    if (piece.hasLo && piece.hasHi) {
                        currentX = (piece.lo + piece.hi) / 2.0;
                        currentP = evalPdf(currentX);
                        if (currentP > 0.0) break;
                    } else if (piece.hasLo) {
                        currentX = piece.lo + 1.0;
                        currentP = evalPdf(currentX);
                        if (currentP > 0.0) break;
                    }
                }
            }

            const int BURN_IN_STEPS = 100;
            for (int i = 0; i < BURN_IN_STEPS; ++i) {
                double candX = currentX + proposalDist(generator);
                double candP = evalPdf(candX);
                
                if (candP > 0.0) {
                    double alpha = (currentP > 0.0) ? (candP / currentP) : 2.0;
                    if (alpha >= 1.0 || uniformDist(generator) < alpha) {
                        currentX = candX;
                        currentP = candP;
                    }
                }
            }
            return {currentX};
        } else {
            // N-Dimensional Adaptive MCMC
            return runAdaptiveMCMC(*pdf, variableNames, 500, nullptr, nullptr);
        }
    }

    if (params.empty() && kind != Kind::Uniform && kind != Kind::Gaussian && kind != Kind::Exponential) return {0.0};

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            std::uniform_real_distribution<double> dist(min, max);
            return {dist(generator)};
        }
        case Kind::Gaussian: {
            double mean = params.empty() ? 0.0 : params[0];
            double stddev = (params.size() > 1) ? params[1] : 1.0;
            std::normal_distribution<double> dist(mean, stddev);
            return {dist(generator)};
        }
        case Kind::Bernoulli: {
            double p = params.empty() ? 0.5 : params[0];
            std::bernoulli_distribution dist(p);
            return {dist(generator) ? 1.0 : 0.0};
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            std::exponential_distribution<double> dist(lambda);
            return {dist(generator)};
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            std::gamma_distribution<double> dist(alpha, beta);
            return {dist(generator)};
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::weibull_distribution<double> dist(a, b);
            return {dist(generator)};
        }
        case Kind::ExtremeValue: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::extreme_value_distribution<double> dist(a, b);
            return {dist(generator)};
        }
        case Kind::Poisson: {
            double mean = params.empty() ? 1.0 : params[0];
            std::poisson_distribution<int> dist(mean);
            return {static_cast<double>(dist(generator))};
        }
        case Kind::Binomial: {
            int t = params.empty() ? 1 : static_cast<int>(params[0]);
            double p = (params.size() > 1) ? params[1] : 0.5;
            std::binomial_distribution<int> dist(t, p);
            return {static_cast<double>(dist(generator))};
        }
        case Kind::NegativeBinomial: {
            int k = params.empty() ? 1 : static_cast<int>(params[0]);
            double p = (params.size() > 1) ? params[1] : 0.5;
            std::negative_binomial_distribution<int> dist(k, p);
            return {static_cast<double>(dist(generator))};
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            std::geometric_distribution<int> dist(p);
            return {static_cast<double>(dist(generator))};
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            std::lognormal_distribution<double> dist(m, s);
            return {dist(generator)};
        }
        case Kind::ChiSquared: {
            double n = params.empty() ? 1.0 : params[0];
            std::chi_squared_distribution<double> dist(n);
            return {dist(generator)};
        }
        case Kind::Cauchy: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::cauchy_distribution<double> dist(a, b);
            return {dist(generator)};
        }
        case Kind::FisherF: {
            double m = params.empty() ? 1.0 : params[0];
            double n = (params.size() > 1) ? params[1] : 1.0;
            std::fisher_f_distribution<double> dist(m, n);
            return {dist(generator)};
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            std::student_t_distribution<double> dist(n);
            return {dist(generator)};
        }
        default:
            return {0.0};
    }
}

std::vector<double> Distribution::expectedVector() const {
    if (kind == Kind::Symbolic) {
        if (!pdf || variableNames.empty()) return {};
        
        if (variableNames.size() == 1) {
            Piecewise xp = makeMomentIntegral(*pdf, variableNames[0], 1);
            std::map<std::string, double> emptyVars;
            std::string why;
            auto val = definiteIntegral(xp, variableNames[0], -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), emptyVars, &why);
            if (val) return {*val};
            return {0.0};
        } else {
            std::vector<double> expected;
            runAdaptiveMCMC(*pdf, variableNames, 1000, &expected, nullptr);
            return expected;
        }
    }

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            return {(min + max) / 2.0};
        }
        case Kind::Gaussian: {
            return {params.empty() ? 0.0 : params[0]};
        }
        case Kind::Bernoulli: {
            return {params.empty() ? 0.5 : params[0]};
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            return {1.0 / lambda};
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            return {alpha * beta};
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            return {b * std::tgamma(1.0 + 1.0 / a)};
        }
        case Kind::ExtremeValue: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            return {a + b * 0.5772156649}; // Euler-Mascheroni constant
        }
        case Kind::Poisson: {
            return {params.empty() ? 1.0 : params[0]};
        }
        case Kind::Binomial: {
            double t = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return {t * p};
        }
        case Kind::NegativeBinomial: {
            double k = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return {(k * (1.0 - p)) / p};
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            return {(1.0 - p) / p}; // Number of failures before first success
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            return {std::exp(m + (s * s) / 2.0)};
        }
        case Kind::ChiSquared: {
            return {params.empty() ? 1.0 : params[0]};
        }
        case Kind::Cauchy: {
            return {std::numeric_limits<double>::quiet_NaN()};
        }
        case Kind::FisherF: {
            double n = (params.size() > 1) ? params[1] : 1.0;
            if (n <= 2.0) return {std::numeric_limits<double>::quiet_NaN()};
            return {n / (n - 2.0)};
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            if (n <= 1.0) return {std::numeric_limits<double>::quiet_NaN()};
            return {0.0};
        }
        default:
            return {0.0};
    }
}

std::vector<double> Distribution::covarianceMatrix() const {
    if (kind == Kind::Symbolic) {
        if (!pdf || variableNames.empty()) return {};
        
        if (variableNames.size() == 1) {
            Piecewise x2p = makeMomentIntegral(*pdf, variableNames[0], 2);
            std::map<std::string, double> emptyVars;
            std::string why;
            
            auto e_x2 = definiteIntegral(x2p, variableNames[0], -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), emptyVars, &why);
            if (!e_x2) return {0.0};
            
            double ex = expectedValue();
            return {*e_x2 - (ex * ex)};
        } else {
            std::vector<double> cov;
            runAdaptiveMCMC(*pdf, variableNames, 1000, nullptr, &cov);
            return cov;
        }
    }

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            return {((max - min) * (max - min)) / 12.0};
        }
        case Kind::Gaussian: {
            double s = (params.size() > 1) ? params[1] : 1.0;
            return {s * s};
        }
        case Kind::Bernoulli: {
            double p = params.empty() ? 0.5 : params[0];
            return {p * (1.0 - p)};
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            return {1.0 / (lambda * lambda)};
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            return {alpha * beta * beta};
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            double mu = expectedValue();
            return {b * b * std::tgamma(1.0 + 2.0 / a) - mu * mu};
        }
        case Kind::ExtremeValue: {
            double b = (params.size() > 1) ? params[1] : 1.0;
            return {b * b * 1.6449340668};
        }
        case Kind::Poisson: {
            return {params.empty() ? 1.0 : params[0]};
        }
        case Kind::Binomial: {
            double t = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return {t * p * (1.0 - p)};
        }
        case Kind::NegativeBinomial: {
            double k = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return {(k * (1.0 - p)) / (p * p)};
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            return {(1.0 - p) / (p * p)};
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            return {(std::exp(s * s) - 1.0) * std::exp(2.0 * m + s * s)};
        }
        case Kind::ChiSquared: {
            double n = params.empty() ? 1.0 : params[0];
            return {2.0 * n};
        }
        case Kind::Cauchy: {
            return {std::numeric_limits<double>::quiet_NaN()};
        }
        case Kind::FisherF: {
            double m = params.empty() ? 1.0 : params[0];
            double n = (params.size() > 1) ? params[1] : 1.0;
            if (n <= 4.0) return {std::numeric_limits<double>::quiet_NaN()};
            return {(2.0 * n * n * (m + n - 2.0)) / (m * (n - 2.0) * (n - 2.0) * (n - 4.0))};
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            if (n <= 2.0) return {std::numeric_limits<double>::quiet_NaN()};
            return {n / (n - 2.0)};
        }
        default:
            return {0.0};
    }
}

std::optional<PropertyValue> evaluateStochastic(const std::string& distType, const std::vector<std::optional<PropertyValue>>& evaluatedArgs) {
    std::vector<double> numericParams;
    for (const auto& arg : evaluatedArgs) {
        if (!arg) return std::nullopt; // Undefined inputs propagate to undefined output
        double n = 0.0;
        if (!propertyValueToNumber(*arg, n)) return std::nullopt;
        numericParams.push_back(n);
    }

    Distribution::Kind kind = Distribution::Kind::Uniform;
    if (distType == "gaussian" || distType == "normal") {
        kind = Distribution::Kind::Gaussian;
    } else if (distType == "bernoulli") {
        kind = Distribution::Kind::Bernoulli;
    } else if (distType == "exponential") {
        kind = Distribution::Kind::Exponential;
    } else if (distType == "gamma") {
        kind = Distribution::Kind::Gamma;
    } else if (distType == "weibull") {
        kind = Distribution::Kind::Weibull;
    } else if (distType == "extreme_value") {
        kind = Distribution::Kind::ExtremeValue;
    } else if (distType == "poisson") {
        kind = Distribution::Kind::Poisson;
    } else if (distType == "binomial") {
        kind = Distribution::Kind::Binomial;
    } else if (distType == "negative_binomial") {
        kind = Distribution::Kind::NegativeBinomial;
    } else if (distType == "geometric") {
        kind = Distribution::Kind::Geometric;
    } else if (distType == "lognormal") {
        kind = Distribution::Kind::LogNormal;
    } else if (distType == "chisquared") {
        kind = Distribution::Kind::ChiSquared;
    } else if (distType == "cauchy") {
        kind = Distribution::Kind::Cauchy;
    } else if (distType == "fisher_f") {
        kind = Distribution::Kind::FisherF;
    } else if (distType == "student_t") {
        kind = Distribution::Kind::StudentT;
    }

    Distribution dist(kind, numericParams);
    
    auto s = dist.sample();
    if (s.empty()) return PropertyValue(0.0);
    if (s.size() == 1) return PropertyValue(s[0]);
    
    auto list = std::make_shared<PropertyList>();
    for (double x : s) list->elements.push_back(PropertyValue(x));
    return PropertyValue(list);
}

} // namespace Probability
} // namespace OntoMath
