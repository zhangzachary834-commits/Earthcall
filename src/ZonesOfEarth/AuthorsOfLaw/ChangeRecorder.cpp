#include "ChangeRecorder.hpp"

#include "ConstructedBeing/Singular/Singular.hpp"

#include <cmath>

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

double meanOf(const std::vector<std::pair<float, double>>& s) {
    double sum = 0.0;
    for (const auto& p : s) sum += p.second;
    return s.empty() ? 0.0 : sum / static_cast<double>(s.size());
}

double rmse(const std::vector<std::pair<float, double>>& s, const CurveModel& c) {
    if (s.empty()) return 0.0;
    double acc = 0.0;
    for (const auto& p : s) {
        const double e = p.second - c.evaluate(p.first);
        acc += e * e;
    }
    return std::sqrt(acc / static_cast<double>(s.size()));
}

// Ordinary least squares line fit: v = b + m t.
CurveModel fitLinear(const std::vector<std::pair<float, double>>& s) {
    const double n = static_cast<double>(s.size());
    double st = 0.0, sv = 0.0, stt = 0.0, stv = 0.0;
    for (const auto& p : s) {
        st += p.first;
        sv += p.second;
        stt += static_cast<double>(p.first) * p.first;
        stv += p.first * p.second;
    }
    const double denom = n * stt - st * st;
    double m = 0.0, b = sv / n;
    if (std::fabs(denom) > 1e-12) {
        m = (n * stv - st * sv) / denom;
        b = (sv - m * st) / n;
    }
    return CurveModel::polynomial({b, m});
}

// 3x3 determinant (row-major).
double det3(const double m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Sinusoid fit: frequency from zero crossings of the mean-subtracted signal,
// then a full three-basis least squares over {1, sin ωt, cos ωt} — the DC
// term must be IN the basis, because over a non-integer number of periods the
// sample mean is not the true bias, and pre-centering by it leaks offset into
// amplitude. (A sin θ + B cos θ = R sin(θ + φ), R = √(A²+B²), φ = atan2(B,A).)
bool fitSinusoid(const std::vector<std::pair<float, double>>& s, CurveModel& out) {
    if (s.size() < 8) return false;
    const double mu = meanOf(s);   // for crossing detection only
    const double T = static_cast<double>(s.back().first) - s.front().first;
    if (T <= 0.0) return false;

    int crossings = 0;
    for (std::size_t i = 1; i < s.size(); ++i) {
        const double a = s[i - 1].second - mu;
        const double b = s[i].second - mu;
        if ((a < 0.0 && b >= 0.0) || (a >= 0.0 && b < 0.0)) ++crossings;
    }
    if (crossings < 2) return false;
    // The crossing count is only a SEED: over a non-integer number of periods
    // the mean sits off the true bias, which perturbs the count (e.g. 10
    // crossings where 4.5 periods have 9). Refine by grid search around the
    // seed, keeping the frequency whose least-squares fit has minimal error.
    const double seedFreq = static_cast<double>(crossings) / (2.0 * T);

    const double n = static_cast<double>(s.size());
    const auto solveAt = [&](double freq, CurveModel& model) {
        const double w = kTwoPi * freq;
        double Ss = 0.0, Sc = 0.0, Sss = 0.0, Scc = 0.0, Ssc = 0.0;
        double Sv = 0.0, Svs = 0.0, Svc = 0.0;
        for (const auto& p : s) {
            const double sn = std::sin(w * p.first);
            const double cs = std::cos(w * p.first);
            const double v = p.second;
            Ss += sn;   Sc += cs;
            Sss += sn * sn;   Scc += cs * cs;   Ssc += sn * cs;
            Sv += v;   Svs += v * sn;   Svc += v * cs;
        }
        // Normal equations for v ≈ C + A sin + B cos, solved by Cramer's rule.
        const double M[3][3] = {{n, Ss, Sc}, {Ss, Sss, Ssc}, {Sc, Ssc, Scc}};
        const double d = det3(M);
        if (std::fabs(d) < 1e-12) return false;
        const double MC[3][3] = {{Sv, Ss, Sc}, {Svs, Sss, Ssc}, {Svc, Ssc, Scc}};
        const double MA[3][3] = {{n, Sv, Sc}, {Ss, Svs, Ssc}, {Sc, Svc, Scc}};
        const double MB[3][3] = {{n, Ss, Sv}, {Ss, Sss, Svs}, {Sc, Ssc, Svc}};
        const double C = det3(MC) / d;
        const double A = det3(MA) / d;
        const double B = det3(MB) / d;
        model = CurveModel::sinusoid(std::sqrt(A * A + B * B), freq, std::atan2(B, A), C);
        return true;
    };

    bool found = false;
    double bestErr = 0.0;
    for (int step = -40; step <= 40; ++step) {
        const double freq = seedFreq * (1.0 + 0.01 * step);   // ±40% around seed
        if (freq <= 0.0) continue;
        CurveModel candidate;
        if (!solveAt(freq, candidate)) continue;
        const double err = rmse(s, candidate);
        if (!found || err < bestErr) {
            found = true;
            bestErr = err;
            out = candidate;
        }
    }
    return found;
}

Automation::Channel* channelFor(const std::string& path, Automation::Channel& storage) {
    using C = Automation::Channel;
    if (path == "position.x") { storage = C::PosX; return &storage; }
    if (path == "position.y") { storage = C::PosY; return &storage; }
    if (path == "position.z") { storage = C::PosZ; return &storage; }
    if (path == "rotation.x") { storage = C::RotX; return &storage; }
    if (path == "rotation.y") { storage = C::RotY; return &storage; }
    if (path == "rotation.z") { storage = C::RotZ; return &storage; }
    return nullptr;
}

} // namespace

void ChangeRecorder::watch(const std::string& dottedPath) {
    Trace trace;
    trace.path = PropertyPath::parse(dottedPath);
    _traces.push_back(std::move(trace));
}

void ChangeRecorder::clearWatches() {
    _traces.clear();
}

void ChangeRecorder::begin(Singular& subject) {
    _subject = &subject;
    _t = 0.0f;
    for (auto& trace : _traces) trace.samples.clear();
}

void ChangeRecorder::sample(float dt) {
    if (!_subject) return;
    _t += dt;
    for (auto& trace : _traces) {
        PropertyValue v;
        double x = 0.0;
        if (trace.path.getValue(*_subject, v) == PropertyPath::PathResult::Ok && propertyValueToNumber(v, x)) {
            trace.samples.emplace_back(_t, x);
        }
    }
}

CurveModel ChangeRecorder::fitSeries(const std::vector<std::pair<float, double>>& samples) {
    if (samples.size() < 2) {
        return CurveModel::constant(samples.empty() ? 0.0 : samples.front().second);
    }

    // Constant: the value never meaningfully moved.
    const double mu = meanOf(samples);
    double var = 0.0;
    for (const auto& p : samples) var += (p.second - mu) * (p.second - mu);
    const double stddev = std::sqrt(var / static_cast<double>(samples.size()));
    if (stddev < 1e-9 + 1e-6 * std::fabs(mu)) {
        return CurveModel::constant(mu);
    }

    const CurveModel linear = fitLinear(samples);
    CurveModel sinusoid;
    const bool haveSin = fitSinusoid(samples, sinusoid);

    if (!haveSin) return linear;
    return rmse(samples, sinusoid) < rmse(samples, linear) ? sinusoid : linear;
}

ActionModel ChangeRecorder::fit() const {
    std::vector<ActionNode> drives;
    for (const auto& trace : _traces) {
        CurveModel curve = fitSeries(trace.samples);
        // A constant fit means nothing happened to this variable — the
        // demonstration wasn't about it. No node.
        if (curve.form == CurveModel::Form::Constant) continue;

        ActionNode drive;
        drive.kind = ActionNode::Kind::Drive;
        drive.path = trace.path;
        drive.curve = std::move(curve);
        // input left unbound: the author chooses the domain when assembling
        // the law (time-periodic playback belongs to Automation — toClip()).
        drives.push_back(std::move(drive));
    }

    if (drives.size() == 1) return drives.front();
    return ActionNode::sequence(std::move(drives));   // empty sequence = nothing learned
}

Automation::Clip ChangeRecorder::toClip(const std::string& clipName) const {
    Automation::Clip clip;
    clip.name = clipName;
    for (const auto& trace : _traces) {
        Automation::Channel storage;
        Automation::Channel* channel = channelFor(trace.path.toString(), storage);
        if (!channel || trace.samples.empty()) continue;

        CurveModel curve = fitSeries(trace.samples);
        if (curve.form != CurveModel::Form::Sinusoid) continue;

        // evalTrack = bias + amplitude * sin(2π(t·freq + phase)) — additive
        // over the rest pose, so express bias against where the demonstration
        // started (the rest pose is captured at clip-add time).
        Automation::Track track;
        track.channel = *channel;
        track.wave = Automation::Wave::Sine;
        track.amplitude = static_cast<float>(curve.amplitude);
        track.frequency = static_cast<float>(curve.frequency);
        double turns = curve.phase / kTwoPi;
        turns -= std::floor(turns);   // wrap to [0,1)
        track.phase = static_cast<float>(turns);
        track.bias = static_cast<float>(curve.bias - trace.samples.front().second);
        clip.tracks.push_back(track);
    }
    return clip;
}
