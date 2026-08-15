import os
import re

src_dir = '/Users/zacharyzhang/documents/github/earthcall/src'

def process_file(path, func):
    with open(path, 'r') as f:
        content = f.read()
    new_content = func(content)
    if content != new_content:
        with open(path, 'w') as f:
            f.write(new_content)
        print(f"Updated {path}")

def fix_relation_hpp(content):
    content = content.replace("float weight = 1.0f;", "")
    content = re.sub(r'Relation\(const std::string& type,([^)]+),([^)]+),([^)]+)\s*float weight = 1.0f\);',
                     r'Relation(const std::string& type,\1,\2,\3 float initialWeight = -1.0f);', content)
    
    # Add getWeight() and setWeight()
    insertion = """
    // Developer mode flag for fallback auditing
    static bool s_developerMode;
    float getWeight() const;
    void setWeight(float w);
    """
    content = content.replace("std::string entityB;  // second endpoint", "std::string entityB;  // second endpoint\n" + insertion)
    
    return content

def fix_relation_cpp(content):
    content = content.replace('float weight)', 'float initialWeight)')
    content = content.replace(', weight(weight) {}', ' {}')
    content = content.replace('Relation(type, aEntity.getIdentifier(), bEntity.getIdentifier(), directed, weight)', 'Relation(type, aEntity.getIdentifier(), bEntity.getIdentifier(), directed, initialWeight)')
    
    content = content.replace('weight(weight)', '{}') # Just in case
    
    # Add getter/setter implementation and developer mode init
    impl = """
bool Relation::s_developerMode = true; // Default true for developer testing

float Relation::getWeight() const {
    PropertyValue out;
    if (getDynamicProperty("weight", out)) {
        return out.as<float>();
    }
    if (s_developerMode) {
        std::cerr << "[Relation] AUDIT WARNING: weight not explicitly settled for Relation " << getIdentifier() << ". Falling back to 1.0f in developer mode." << std::endl;
        return 1.0f;
    }
    throw std::runtime_error("Relation weight not explicitly settled by a Person.");
}

void Relation::setWeight(float w) {
    setDynamicProperty("weight", PropertyValue(w));
}
"""
    if "bool Relation::s_developerMode" not in content:
        content = content + "\n" + impl
        
    # Replace direct references to `weight` with `getWeight()` internally
    content = content.replace('<< weight <<', '<< getWeight() <<')
    content = content.replace('{"weight", weight}', '{"weight", getWeight()}')
    content = content.replace('r.weight = j.value("weight", 1.0f);', 'r.setWeight(j.value("weight", 1.0f));')
    
    # Fix constructor bodies to call setWeight if initialWeight != -1
    content = re.sub(r': type\(type\), entityA\(a\), entityB\(b\), directed\(directed\) \{\}',
                     r': type(type), entityA(a), entityB(b), directed(directed) { if (initialWeight != -1.0f) setWeight(initialWeight); }', content)

    # In buildProperties: replace property binding
    content = content.replace('ComputedProperty<float>::make("weight", this, &Relation::weight)', 'ComputedProperty<float>::make("weight", this, &Relation::getWeight)')

    return content

process_file(os.path.join(src_dir, 'Relation/Relation.hpp'), fix_relation_hpp)
process_file(os.path.join(src_dir, 'Relation/Relation.cpp'), fix_relation_cpp)

