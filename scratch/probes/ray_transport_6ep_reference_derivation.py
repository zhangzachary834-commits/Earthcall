"""Closed source derivation for worker-local CPU reference binding storage.
Pure text transformation, no build/runtime imports or caller-selected source hashes.
Proposal: Aetherion / The Firmament. Codex / GPT-6.1 Sol / Sixth-One Sun /
01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import hashlib


def need(ok, message):
    if not ok:
        raise ValueError(message)


CANONICAL_SHA = 'bdcdd79eb7408b45cc48346010d6584e2d4cdedbf974ae25b5302148fa443b1f'
KIND = 'worker-local-reference-bindings-v1'
OLD_MAP = '''                    const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(pf)},{"x",PropertyValue(double(pf.x))},{"y",PropertyValue(double(pf.y))},{"z",PropertyValue(double(pf.z))},
                        {OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(double(wo.x))},{OntoMath::kOmegaYVar,PropertyValue(double(wo.y))},{OntoMath::kOmegaZVar,PropertyValue(double(wo.z))}};'''
SLOTS = '''        // Worker-private binding storage. Every mutable slot is refreshed at
        // every original sample/member site; clock is invariant for this job.
        // No AST/arithmetic/quadrature change. The separate diagnostic map below
        // intentionally remains original. MathNode::evaluate reads const vars.
        std::map<std::string,PropertyValue> vars{{"p",PropertyValue(glm::vec3(0))},{"x",PropertyValue(0.0)},{"y",PropertyValue(0.0)},{"z",PropertyValue(0.0)},
            {OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(0.0)},{OntoMath::kOmegaYVar,PropertyValue(0.0)},{OntoMath::kOmegaZVar,PropertyValue(0.0)}};
        auto &vp=vars.at("p"), &vx=vars.at("x"), &vy=vars.at("y"), &vz=vars.at("z");
        auto &vOx=vars.at(OntoMath::kOmegaXVar), &vOy=vars.at(OntoMath::kOmegaYVar), &vOz=vars.at(OntoMath::kOmegaZVar);
'''
ASSIGN = '''                    vp=PropertyValue(pf);vx=PropertyValue(double(pf.x));vy=PropertyValue(double(pf.y));vz=PropertyValue(double(pf.z));
                    vOx=PropertyValue(double(wo.x));vOy=PropertyValue(double(wo.y));vOz=PropertyValue(double(wo.z));'''
HEADER = '// Derived by worker-local-reference-bindings-v1; original math and evaluation calls unchanged.\n'


def sha(b):
    return hashlib.sha256(b).hexdigest()


def derive_source(original):
    """A closed, reversible two-site edit, pinned to the reviewed original bytes.
    This is the only derived generator admitted by the Oracle identity extension;
    arbitrary caller-supplied generator hashes are never an admission mechanism.
    """
    need(sha(original.encode()) == CANONICAL_SHA, 'unreviewed canonical reference source')
    anchor = '    auto work=[&]{\n'
    need(original.count(anchor) == original.count(OLD_MAP) == 1, 'binding derivation anchor')
    result = HEADER + original.replace(anchor, anchor+SLOTS).replace(OLD_MAP, ASSIGN)
    restored = result.removeprefix(HEADER).replace(anchor+SLOTS,anchor).replace(ASSIGN,OLD_MAP)
    need(restored == original, 'nonreversible binding derivation')
    return result


def derivation_identity(original):
    return dict(kind=KIND, canonical_generator_sha256=CANONICAL_SHA,
                derived_generator_sha256=sha(derive_source(original).encode()))

