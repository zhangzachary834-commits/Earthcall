#include "Singularity/OntoMath/ProbabilityForm.hpp"
#include <random>

namespace OntoMath {
namespace Probability {

// Thread-local random number generator to avoid lock contention during parallel evaluations
static thread_local std::mt19937 generator(std::random_device{}());

double Distribution::sample() const {
    if (kind == Kind::Symbolic) {
        if (!pdf) return 0.0;

        // Metropolis-Hastings MCMC Sampler
        // 1. Find a starting point (use exact Expected Value if available, else 0.0)
        double currentX = expectedValue();
        if (std::isnan(currentX)) {
            currentX = 0.0;
        }

        // 2. Determine optimal proposal step size using exact Variance!
        // This is a beautiful synergy of analytical math driving the numerical sampler.
        double var = variance();
        double sigma = (std::isnan(var) || var <= 0.0) ? 1.0 : std::sqrt(var);
        
        std::normal_distribution<double> proposalDist(0.0, sigma);
        std::uniform_real_distribution<double> uniformDist(0.0, 1.0);

        auto evalPdf = [&](double x) -> double {
            std::map<std::string, PropertyValue> vars;
            vars[variableName] = PropertyValue(x);
            auto res = pdf->evaluate(vars);
            if (!res) return 0.0;
            double val = 0.0;
            propertyValueToNumber(*res, val);
            return std::max(0.0, val);
        };

        double currentP = evalPdf(currentX);

        // If the starting point has zero probability (e.g. out of bounds), we need to find ANY valid piece.
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

        // 3. Burn-in Phase (100 steps is sufficient for an optimal step size to converge locally)
        const int BURN_IN_STEPS = 100;
        for (int i = 0; i < BURN_IN_STEPS; ++i) {
            double candX = currentX + proposalDist(generator);
            double candP = evalPdf(candX);
            
            if (candP > 0.0) {
                double alpha = (currentP > 0.0) ? (candP / currentP) : 2.0; // Always accept if current was 0
                if (alpha >= 1.0 || uniformDist(generator) < alpha) {
                    currentX = candX;
                    currentP = candP;
                }
            }
        }
        
        return currentX;
    }

    if (params.empty() && kind != Kind::Uniform && kind != Kind::Gaussian && kind != Kind::Exponential) return 0.0;

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            std::uniform_real_distribution<double> dist(min, max);
            return dist(generator);
        }
        case Kind::Gaussian: {
            double mean = params.empty() ? 0.0 : params[0];
            double stddev = (params.size() > 1) ? params[1] : 1.0;
            std::normal_distribution<double> dist(mean, stddev);
            return dist(generator);
        }
        case Kind::Bernoulli: {
            double p = params.empty() ? 0.5 : params[0];
            std::bernoulli_distribution dist(p);
            return dist(generator) ? 1.0 : 0.0;
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            std::exponential_distribution<double> dist(lambda);
            return dist(generator);
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            std::gamma_distribution<double> dist(alpha, beta);
            return dist(generator);
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::weibull_distribution<double> dist(a, b);
            return dist(generator);
        }
        case Kind::ExtremeValue: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::extreme_value_distribution<double> dist(a, b);
            return dist(generator);
        }
        case Kind::Poisson: {
            double mean = params.empty() ? 1.0 : params[0];
            std::poisson_distribution<int> dist(mean);
            return dist(generator);
        }
        case Kind::Binomial: {
            int t = params.empty() ? 1 : static_cast<int>(params[0]);
            double p = (params.size() > 1) ? params[1] : 0.5;
            std::binomial_distribution<int> dist(t, p);
            return dist(generator);
        }
        case Kind::NegativeBinomial: {
            int k = params.empty() ? 1 : static_cast<int>(params[0]);
            double p = (params.size() > 1) ? params[1] : 0.5;
            std::negative_binomial_distribution<int> dist(k, p);
            return dist(generator);
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            std::geometric_distribution<int> dist(p);
            return dist(generator);
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            std::lognormal_distribution<double> dist(m, s);
            return dist(generator);
        }
        case Kind::ChiSquared: {
            double n = params.empty() ? 1.0 : params[0];
            std::chi_squared_distribution<double> dist(n);
            return dist(generator);
        }
        case Kind::Cauchy: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            std::cauchy_distribution<double> dist(a, b);
            return dist(generator);
        }
        case Kind::FisherF: {
            double m = params.empty() ? 1.0 : params[0];
            double n = (params.size() > 1) ? params[1] : 1.0;
            std::fisher_f_distribution<double> dist(m, n);
            return dist(generator);
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            std::student_t_distribution<double> dist(n);
            return dist(generator);
        }
        default:
            return 0.0;
    }
}

static Piecewise makeMomentIntegral(const Piecewise& pdf, const std::string& varName, int power) {
    Piecewise out = pdf;
    
    auto varNode = std::make_shared<MathNode>();
    varNode->op = MathNode::Op::ValueLeaf;
    varNode->variableName = varName;
    
    std::shared_ptr<MathNode> multFactor = varNode;
    if (power == 2) {
        auto p2 = std::make_shared<MathNode>();
        p2->op = MathNode::Op::Scale; // Scalar * Scalar multiplication
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

double Distribution::expectedValue() const {
    if (kind == Kind::Symbolic) {
        if (!pdf) return 0.0;
        Piecewise xp = makeMomentIntegral(*pdf, variableName, 1);
        
        std::map<std::string, double> emptyVars;
        std::string why;
        auto val = definiteIntegral(xp, variableName, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), emptyVars, &why);
        if (val) return *val;
        return 0.0; // Fallback if irreducible analytically
    }

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            return (min + max) / 2.0;
        }
        case Kind::Gaussian: {
            return params.empty() ? 0.0 : params[0];
        }
        case Kind::Bernoulli: {
            return params.empty() ? 0.5 : params[0];
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            return 1.0 / lambda;
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            return alpha * beta;
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            return b * std::tgamma(1.0 + 1.0 / a);
        }
        case Kind::ExtremeValue: {
            double a = params.empty() ? 0.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            return a + b * 0.5772156649; // Euler-Mascheroni constant
        }
        case Kind::Poisson: {
            return params.empty() ? 1.0 : params[0];
        }
        case Kind::Binomial: {
            double t = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return t * p;
        }
        case Kind::NegativeBinomial: {
            double k = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return (k * (1.0 - p)) / p;
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            return (1.0 - p) / p; // Number of failures before first success
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            return std::exp(m + (s * s) / 2.0);
        }
        case Kind::ChiSquared: {
            return params.empty() ? 1.0 : params[0];
        }
        case Kind::Cauchy: {
            return std::numeric_limits<double>::quiet_NaN(); // Undefined for Cauchy
        }
        case Kind::FisherF: {
            double n = (params.size() > 1) ? params[1] : 1.0;
            if (n <= 2.0) return std::numeric_limits<double>::quiet_NaN();
            return n / (n - 2.0);
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            if (n <= 1.0) return std::numeric_limits<double>::quiet_NaN();
            return 0.0;
        }
        default:
            return 0.0;
    }
}

double Distribution::variance() const {
    if (kind == Kind::Symbolic) {
        if (!pdf) return 0.0;
        
        // Var(X) = E[X^2] - (E[X])^2
        Piecewise x2p = makeMomentIntegral(*pdf, variableName, 2);
        std::map<std::string, double> emptyVars;
        std::string why;
        
        auto e_x2 = definiteIntegral(x2p, variableName, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), emptyVars, &why);
        if (!e_x2) return 0.0;
        
        double ex = expectedValue();
        return *e_x2 - (ex * ex);
    }

    switch (kind) {
        case Kind::Uniform: {
            double min = params.empty() ? 0.0 : params[0];
            double max = (params.size() > 1) ? params[1] : 1.0;
            return ((max - min) * (max - min)) / 12.0;
        }
        case Kind::Gaussian: {
            double s = (params.size() > 1) ? params[1] : 1.0;
            return s * s;
        }
        case Kind::Bernoulli: {
            double p = params.empty() ? 0.5 : params[0];
            return p * (1.0 - p);
        }
        case Kind::Exponential: {
            double lambda = params.empty() ? 1.0 : params[0];
            return 1.0 / (lambda * lambda);
        }
        case Kind::Gamma: {
            double alpha = params.empty() ? 1.0 : params[0];
            double beta = (params.size() > 1) ? params[1] : 1.0;
            return alpha * beta * beta;
        }
        case Kind::Weibull: {
            double a = params.empty() ? 1.0 : params[0];
            double b = (params.size() > 1) ? params[1] : 1.0;
            double mu = expectedValue();
            return b * b * std::tgamma(1.0 + 2.0 / a) - mu * mu;
        }
        case Kind::ExtremeValue: {
            double b = (params.size() > 1) ? params[1] : 1.0;
            return b * b * 1.6449340668; // pi^2 / 6
        }
        case Kind::Poisson: {
            return params.empty() ? 1.0 : params[0];
        }
        case Kind::Binomial: {
            double t = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return t * p * (1.0 - p);
        }
        case Kind::NegativeBinomial: {
            double k = params.empty() ? 1.0 : params[0];
            double p = (params.size() > 1) ? params[1] : 0.5;
            return (k * (1.0 - p)) / (p * p);
        }
        case Kind::Geometric: {
            double p = params.empty() ? 0.5 : params[0];
            return (1.0 - p) / (p * p);
        }
        case Kind::LogNormal: {
            double m = params.empty() ? 0.0 : params[0];
            double s = (params.size() > 1) ? params[1] : 1.0;
            return (std::exp(s * s) - 1.0) * std::exp(2.0 * m + s * s);
        }
        case Kind::ChiSquared: {
            double n = params.empty() ? 1.0 : params[0];
            return 2.0 * n;
        }
        case Kind::Cauchy: {
            return std::numeric_limits<double>::quiet_NaN();
        }
        case Kind::FisherF: {
            double m = params.empty() ? 1.0 : params[0];
            double n = (params.size() > 1) ? params[1] : 1.0;
            if (n <= 4.0) return std::numeric_limits<double>::quiet_NaN();
            return (2.0 * n * n * (m + n - 2.0)) / (m * (n - 2.0) * (n - 2.0) * (n - 4.0));
        }
        case Kind::StudentT: {
            double n = params.empty() ? 1.0 : params[0];
            if (n <= 2.0) return std::numeric_limits<double>::quiet_NaN();
            return n / (n - 2.0);
        }
        default:
            return 0.0;
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
    return PropertyValue(dist.sample());
}

} // namespace Probability
} // namespace OntoMath
