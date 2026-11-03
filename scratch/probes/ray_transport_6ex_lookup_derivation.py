"""Closed external OntoMath numeric-lookup experiment; no renderer arithmetic.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import hashlib

PARENT_SHA='cf41aaa87aa7e339d97c5e60b80be15f6959c72c0ab52cede5dd625bf84dde0d'

def sha(s):return hashlib.sha256(s.encode()).hexdigest()

def edits(source):
 if sha(source)!=PARENT_SHA:raise ValueError('unreviewed parent ScalarForm.cpp')
 a=source.index('std::optional<double> Term::evaluate(');b=source.index('\nnamespace {',a)
 term=source[a:b]
 a=source.index('std::optional<double> ScalarForm::evaluate(');b=source.index('\nInterval ScalarForm::evalRange',a)
 form=source[a:b]
 a=source.index('            std::map<std::string, double> scalarVars;',source.index('std::optional<PropertyValue> MathNode::evaluate('));b=source.index('            if (!res) return std::nullopt;',a)
 leaf=source[a:b]
 # Retain the original multiplication / power / transcendental and sum order.
 generic_term=term.replace('std::optional<double> Term::evaluate(const std::map<std::string, double>& vars) const {','template<class Lookup>\nstd::optional<double> evaluateTermLookup(const Term& term, const Lookup& lookup) {')
 generic_term=generic_term.replace('double value = coefficient;','double value = term.coefficient;').replace(': factors)',': term.factors)').replace(': trans)',': term.trans)')
 generic_term=generic_term.replace('auto it = vars.find(name);\n        if (it == vars.end()) return std::nullopt;   // unbound symbol: no value\n        value *= std::pow(it->second, exp);','auto x = lookup(name);\n        if (!x) return std::nullopt;   // same strict unbound refusal\n        value *= std::pow(*x, exp);')
 generic_term=generic_term.replace('auto it = vars.find(tf.variable);\n        if (it == vars.end()) return std::nullopt;\n        auto v = tf.evaluate(it->second);','auto x = lookup(tf.variable);\n        if (!x) return std::nullopt;\n        auto v = tf.evaluate(*x);')
 if 'vars.' in generic_term:raise ValueError('term seam drift')
 generic_form=form.replace('std::optional<double> ScalarForm::evaluate(const std::map<std::string, double>& vars) const {','template<class Lookup>\nstd::optional<double> evaluateScalarLookup(const ScalarForm& form, const Lookup& lookup) {').replace(': terms)',': form.terms)').replace('term.evaluate(vars)','evaluateTermLookup(term, lookup)')
 numeric='''[](const std::string& name) -> std::optional<double> { return std::nullopt; }'''
 lookup='''[&](const std::string& name) -> std::optional<double> {
        auto it = vars.find(name);
        if (it == vars.end()) return std::nullopt;
        return it->second;
    }'''
 term_new='''namespace {
// §6ex external experiment. Local lookup carries no retained state; ordered
// arithmetic is shared by the public numeric-map and PropertyValue adapters.
'''+generic_term+generic_form+'''} // namespace

std::optional<double> Term::evaluate(const std::map<std::string, double>& vars) const {
    return evaluateTermLookup(*this, '''+lookup+''');
}
'''
 form_new='''std::optional<double> ScalarForm::evaluate(const std::map<std::string, double>& vars) const {
    return evaluateScalarLookup(*this, '''+lookup+''');
}
'''
 leaf_new='''            // Immediate, pure conversion of the current call's bindings.
            // No map materialization, cross-call cache or thread-local state.
            auto res = evaluateScalarLookup(scalarForm,
                [&](const std::string& name) -> std::optional<double> {
                    auto it = vars.find(name);
                    if (it == vars.end()) return std::nullopt;
                    double value = 0.0;
                    if (!propertyValueToNumber(it->second, value)) return std::nullopt;
                    return value;
                });
'''
 return [(term,term_new),(form,form_new),(leaf,leaf_new)]

def derive(source):
 pairs=edits(source);result=source
 for old,new in pairs:
  if result.count(old)!=1:raise ValueError('nonunique source seam')
  result=result.replace(old,new)
 reverse=result
 for old,new in reversed(pairs):
  if reverse.count(new)!=1:raise ValueError('nonunique reverse seam')
  reverse=reverse.replace(new,old)
 if reverse!=source:raise ValueError('not reversible')
 return result

if __name__=='__main__':
 import sys
 from pathlib import Path
 s=Path(sys.argv[1]).read_text();d=derive(s);Path(sys.argv[2]).write_text(d)
 print('parent',sha(s),'derived',sha(d),'closed edits',len(edits(s)))
