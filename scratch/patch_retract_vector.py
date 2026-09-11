import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

# Replace std::vector::erase with std::swap and pop_back for O(1) removal

# 1. ReteNetwork::retractFirst
content = content.replace("""void ReteNetwork::retractFirst(std::size_t count) {
    if (count == 0) return;
    if (count >= _facts.size()) {
        count = _facts.size();
    }

    std::unordered_set<std::string> removedIds;
    std::vector<FactPtr> new_facts;
    for (std::size_t i = 0; i < count; ++i) {
        if (!_facts[i]->isState) {
            removedIds.insert(_facts[i]->id);
        } else {
            new_facts.push_back(_facts[i]);
        }
    }
    for (std::size_t i = count; i < _facts.size(); ++i) {
        new_facts.push_back(_facts[i]);
    }
    _facts = std::move(new_facts);""", """void ReteNetwork::retractFirst(std::size_t count) {
    if (count == 0) return;
    if (count >= _facts.size()) count = _facts.size();

    std::unordered_set<std::string> removedIds;
    std::vector<FactPtr> new_facts;
    for (std::size_t i = 0; i < count; ++i) {
        if (!_facts[i]->isState) {
            removedIds.insert(_facts[i]->id);
        } else {
            new_facts.push_back(_facts[i]);
        }
    }
    for (std::size_t i = count; i < _facts.size(); ++i) {
        new_facts.push_back(_facts[i]);
    }
    _facts = std::move(new_facts);""")

# 2. retractFact: _stateFactsBySubjectAttr
content = content.replace("""        if (sIt != _stateFactsBySubjectAttr.end()) {
            auto& vec = sIt->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const FactPtr& f) { return f->id == factId; }), vec.end());
            if (vec.empty()) _stateFactsBySubjectAttr.erase(sIt);
        }""", """        if (sIt != _stateFactsBySubjectAttr.end()) {
            auto& vec = sIt->second;
            auto it = std::find_if(vec.begin(), vec.end(), [&](const FactPtr& f) { return f->id == factId; });
            if (it != vec.end()) {
                *it = std::move(vec.back());
                vec.pop_back();
            }
            if (vec.empty()) _stateFactsBySubjectAttr.erase(sIt);
        }""")

# 3. retractFact: _facts
content = content.replace("""    auto it = std::find_if(_facts.begin(), _facts.end(), [&](const FactPtr& f) { return f->id == factId; });
    if (it != _facts.end()) _facts.erase(it);""", """    auto it = std::find_if(_facts.begin(), _facts.end(), [&](const FactPtr& f) { return f->id == factId; });
    if (it != _facts.end()) {
        *it = std::move(_facts.back());
        _facts.pop_back();
    }""")

# 4. retractFact: alpha->memory
content = content.replace("""                auto mIt = std::find_if(alpha->memory.begin(), alpha->memory.end(),
                                        [&](const FactPtr& f) { return f->id == factId; });
                if (mIt != alpha->memory.end()) alpha->memory.erase(mIt);""", """                auto mIt = std::find_if(alpha->memory.begin(), alpha->memory.end(),
                                        [&](const FactPtr& f) { return f->id == factId; });
                if (mIt != alpha->memory.end()) {
                    *mIt = std::move(alpha->memory.back());
                    alpha->memory.pop_back();
                }""")

# 5. retractFact: _betaNodes.memory
content = content.replace("""                bNodeIt->memory.erase(std::remove_if(bNodeIt->memory.begin(), bNodeIt->memory.end(),
                                                    [&](const ReteToken& token) {
                                                        for (const auto& f : token.facts) if (f->id == factId) return true;
                                                        return false;
                                                    }),
                                      bNodeIt->memory.end());""", """                auto bit = std::find_if(bNodeIt->memory.begin(), bNodeIt->memory.end(),
                                                    [&](const ReteToken& token) {
                                                        for (const auto& f : token.facts) if (f->id == factId) return true;
                                                        return false;
                                                    });
                if (bit != bNodeIt->memory.end()) {
                    *bit = std::move(bNodeIt->memory.back());
                    bNodeIt->memory.pop_back();
                }""")

# 6. retractFact: _agenda
content = content.replace("""    if (_agendaFactIds.count(factId)) {
        _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(),
                                     [&](const ReteActivation& act) {
                                         for (const auto& f : act.token.facts) if (f->id == factId) return true;
                                         return false;
                                     }),
                      _agenda.end());
        _agendaFactIds.erase(factId);
    }""", """    if (_agendaFactIds.count(factId)) {
        auto agit = std::find_if(_agenda.begin(), _agenda.end(),
                                     [&](const ReteActivation& act) {
                                         for (const auto& f : act.token.facts) if (f->id == factId) return true;
                                         return false;
                                     });
        if (agit != _agenda.end()) {
            *agit = std::move(_agenda.back());
            _agenda.pop_back();
        }
        _agendaFactIds.erase(factId);
    }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
