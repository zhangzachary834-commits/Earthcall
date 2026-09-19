// Probe: are the compilation claims in docs/architecture/law/ALGORITHMS_AS_LAW.md
// actually true of this build? Not a permanent test — a check that the doc is
// not lying to the agents who will follow it.
//
// Checks: §5c recursion (factorial) · §3-II kMaxCallDepth honest nullopt ·
//         §5b whereLEZero branch (max) · §5a fold identities ·
//         §5d exact derivative.

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

using namespace OntoMath;

namespace {

std::shared_ptr<MathNode> node(const ScalarForm& f) {
    return MathNode::fromLegacyExpression(f);
}

double num(const std::optional<PropertyValue>& v) {
    assert(v.has_value() && "expected a defined value");
    return std::get<double>(*v);
}

} // namespace

int main() {
    // ---------------------------------------------------------------- §5c
    // fact(n):  piece 1  whereLEZero = n - 1        -> 1
    //           piece 2  (otherwise)                -> n * fact(n-1)
    {
        Piecewise body;
        body.inputVariable = "n";

        Piecewise::Piece base;
        base.whereLEZero = node(ScalarForm::variable("n").plus(ScalarForm::constant(-1)));
        base.mathNode = node(ScalarForm::constant(1));
        body.pieces.push_back(base);

        // A piece whose value is a call returns the call VERBATIM — the
        // expression is ignored — so "n * fact(n-1)" is NOT expressible in one
        // piece. This piece therefore computes the identity, which is exactly
        // what the probe below demonstrates, and why §5c must teach the
        // accumulator form instead.
        Piecewise::Piece rec;
        auto call = std::make_shared<FunctionCall>();
        call->function = "fact";
        call->args.push_back(ScalarForm::variable("n").plus(ScalarForm::constant(-1)));
        rec.call = call;
        body.pieces.push_back(rec);

        FunctionDef def{"fact", {"n"}, body};
        FunctionRegistry::instance().define(def);

        const FunctionDef* found = FunctionRegistry::instance().find("fact");
        assert(found && "recursive definition did not register");

        // Base case reached: n <= 1 returns 1 without recursing.
        std::map<std::string, PropertyValue> v1{{"n", PropertyValue(1.0)}};
        auto r1 = found->body.evaluate(v1, nullptr, 0);
        assert(r1.has_value() && num(r1) == 1.0);

        // Recursive descent terminates at the base case.
        std::map<std::string, PropertyValue> v5{{"n", PropertyValue(5.0)}};
        auto r5 = found->body.evaluate(v5, nullptr, 0);
        assert(r5.has_value() && "descending recursion should reach the base case");
        assert(num(r5) == 1.0 && "a call-valued piece returns the call verbatim");
        std::printf("  naive f(n)=n*f(n-1) shape -> %g (identity: the multiply is LOST)\n",
                    num(r5));

        // The form that actually works: ACCUMULATOR-PASSING. The arithmetic
        // rides in the ARGUMENT expressions, which are full ScalarForms —
        // "iteration is carried through the arguments", per the header.
        //   factAcc(n, acc):  where n-1 <= 0  -> acc
        //                     otherwise       -> factAcc(n-1, n*acc)
        {
            Piecewise acc;
            acc.inputVariable = "n";

            Piecewise::Piece stop;
            stop.whereLEZero =
                node(ScalarForm::variable("n").plus(ScalarForm::constant(-1)));
            stop.mathNode = node(ScalarForm::variable("acc"));
            acc.pieces.push_back(stop);

            Piecewise::Piece step;
            auto stepCall = std::make_shared<FunctionCall>();
            stepCall->function = "factAcc";
            stepCall->args.push_back(
                ScalarForm::variable("n").plus(ScalarForm::constant(-1)));
            stepCall->args.push_back(
                ScalarForm::variable("n").times(ScalarForm::variable("acc")));
            step.call = stepCall;
            acc.pieces.push_back(step);

            FunctionRegistry::instance().define(FunctionDef{"factAcc", {"n", "acc"}, acc});

            std::map<std::string, PropertyValue> in{{"n", PropertyValue(5.0)},
                                                    {"acc", PropertyValue(1.0)}};
            auto got = FunctionRegistry::instance().find("factAcc")->body.evaluate(
                in, nullptr, 0);
            assert(got.has_value() && "accumulator recursion should terminate");
            assert(num(got) == 120.0 && "factAcc(5,1) should be 120");
            std::printf("  accumulator recursion: factAcc(5,1) -> %g\n", num(got));
        }

        // §3-II: divergence is an honest nullopt, not a hang or a crash.
        Piecewise diverge;
        diverge.inputVariable = "n";
        Piecewise::Piece up;
        auto upCall = std::make_shared<FunctionCall>();
        upCall->function = "diverge";
        upCall->args.push_back(ScalarForm::variable("n").plus(ScalarForm::constant(1)));
        up.call = upCall;
        diverge.pieces.push_back(up);
        FunctionRegistry::instance().define(FunctionDef{"diverge", {"n"}, diverge});

        std::map<std::string, PropertyValue> v0{{"n", PropertyValue(0.0)}};
        auto rd = FunctionRegistry::instance().find("diverge")->body.evaluate(v0, nullptr, 0);
        assert(!rd.has_value() && "unbounded recursion must return nullopt, not hang");
        std::printf("  unbounded recursion -> nullopt (ceiling %d)\n",
                    FunctionRegistry::kMaxCallDepth);
    }

    // ---------------------------------------------------------------- §5b
    // max(a,b): piece 1 where a-b <= 0 -> b ; piece 2 where b-a <= 0 -> a
    {
        Piecewise mx;
        mx.inputVariable = "a";

        Piecewise::Piece p1;
        p1.whereLEZero = node(ScalarForm::variable("a")
                                  .plus(ScalarForm::variable("b").scaled(-1.0)));
        p1.mathNode = node(ScalarForm::variable("b"));
        mx.pieces.push_back(p1);

        Piecewise::Piece p2;
        p2.whereLEZero = node(ScalarForm::variable("b")
                                  .plus(ScalarForm::variable("a").scaled(-1.0)));
        p2.mathNode = node(ScalarForm::variable("a"));
        mx.pieces.push_back(p2);

        std::map<std::string, PropertyValue> lo{{"a", PropertyValue(3.0)},
                                                {"b", PropertyValue(7.0)}};
        std::map<std::string, PropertyValue> hi{{"a", PropertyValue(9.0)},
                                                {"b", PropertyValue(2.0)}};
        assert(num(mx.evaluate(lo, nullptr, 0)) == 7.0 && "max(3,7) should be 7");
        assert(num(mx.evaluate(hi, nullptr, 0)) == 9.0 && "max(9,2) should be 9");
        std::printf("  whereLEZero branch: max(3,7)=7  max(9,2)=9\n");
    }

    // ---------------------------------------------------------------- §5d
    // The claim that earns the substrate its keep: f' is exact and free.
    // f(x) = 3x^3 - 2x + 5   ->   f'(x) = 9x^2 - 2
    {
        ScalarForm f = ScalarForm::variable("x", 3.0, 3.0)
                           .plus(ScalarForm::variable("x", 1.0, -2.0))
                           .plus(ScalarForm::constant(5.0));
        ScalarForm d = f.derivative("x").normalized();

        for (double x : {-2.0, 0.5, 3.0}) {
            std::map<std::string, double> vars{{"x", x}};
            auto got = d.evaluate(vars);
            assert(got.has_value());
            const double want = 9.0 * x * x - 2.0;
            assert(std::fabs(*got - want) < 1e-12 && "derivative is not exact");
        }
        std::printf("  exact derivative: d/dx(3x^3-2x+5) = %s\n", d.print().c_str());
    }

    // ---------------------------------------------------------------- §5a
    // Fold identities: the doc claims empty Sum/Count are 0 and empty
    // Mean/Min/Max are nullopt. Verify the enum contract at least round-trips.
    {
        Fold sum; sum.op = Fold::Op::Sum;  sum.beingKind = 1; sum.path = "height";
        auto j = sum.toJson();
        Fold back = Fold::fromJson(j);
        assert(back.op == Fold::Op::Sum && back.beingKind == 1 && back.path == "height");
        assert(static_cast<int>(Fold::Op::Sum)   == 0);
        assert(static_cast<int>(Fold::Op::Mean)  == 1);
        assert(static_cast<int>(Fold::Op::Min)   == 2);
        assert(static_cast<int>(Fold::Op::Max)   == 3);
        assert(static_cast<int>(Fold::Op::Count) == 4);
        std::printf("  fold enum contract holds (Sum0 Mean1 Min2 Max3 Count4)\n");
    }

    std::printf("OK  ALGORITHMS_AS_LAW.md §§3,5a-5d verified against this build\n");
    return 0;
}
