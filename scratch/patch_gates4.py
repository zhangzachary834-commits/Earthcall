import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """void Law::recompile() {
    if (_conditionModel) {
        _conditionPredicates.clear();
        addCondition(_conditionModel->describe(), _conditionModel->compile());
    }"""

replace_str = """void Law::recompile() {
    _compiledGates.clear();
    if (_conditionModel) {
        _conditionPredicates.clear();
        addCondition(_conditionModel->describe(), _conditionModel->compile());
        
        std::vector<const ConditionNode*> gates;
        _conditionModel->collectHoistableGates(gates);
        for (const auto* gate : gates) {
            _compiledGates.push_back(gate->compile());
        }
    }"""

find_str2 = """bool LawManager::gatesHold(const Law& law) const {
    const ConditionNode* model = law.conditionModel();
    if (!model) return true;

    std::vector<const ConditionNode*> gates;
    model->collectHoistableGates(gates);
    if (gates.empty()) return true;

    // A gate ignores its subject, so ANY Singular answers it identically — and
    // the law itself is one, always alive, and never a member of the world it
    // is asked about. Deliberately not a member of the population: a world may
    // legitimately be empty at this moment.
    ECA::Event probe;
    probe.type = "law-gate";
    Singular& standIn = const_cast<Law&>(law);
    bool gatesInitiallyTrue = true;
    for (const ConditionNode* gate : gates) {
        if (!gate->compile()(probe, standIn)) {
            gatesInitiallyTrue = false;
            break;
        }
    }

    // If the gate is already false, the law won't run AT ALL, so it can't possibly
    // execute its actions to flip the gate. We can safely hoist the FALSE!
    if (!gatesInitiallyTrue) return false;"""

replace_str2 = """bool LawManager::gatesHold(const Law& law) const {
    const auto& compiledGates = law.compiledGates();
    if (compiledGates.empty()) return true;

    // A gate ignores its subject, so ANY Singular answers it identically — and
    // the law itself is one, always alive, and never a member of the world it
    // is asked about. Deliberately not a member of the population: a world may
    // legitimately be empty at this moment.
    ECA::Event probe;
    probe.type = "law-gate";
    Singular& standIn = const_cast<Law&>(law);
    bool gatesInitiallyTrue = true;
    for (const auto& predicate : compiledGates) {
        if (!predicate(probe, standIn)) {
            gatesInitiallyTrue = false;
            break;
        }
    }

    // If the gate is already false, the law won't run AT ALL, so it can't possibly
    // execute its actions to flip the gate. We can safely hoist the FALSE!
    if (!gatesInitiallyTrue) return false;"""

if find_str in content and find_str2 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp gatesHold optimization!")
else:
    print("Could not find string in Law.cpp!")
