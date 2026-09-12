import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    std::vector<Law::ApplicationRecord> records;
    for (int round = 0; round < _maxChainRounds && _dirty; ++round) { if(round == 7) std::cout << "MAX CHAIN ROUNDS HIT!" << std::endl;
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by _maxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        // Straight to the drain: the agenda is already complete. (An
        // _rete.evaluate() call sat here whose result was discarded — the
        // last trace of the rebuild-every-frame design.)
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        static int total_agenda_size = 0;
        total_agenda_size += agenda.size();"""

replace_str = """    std::vector<Law::ApplicationRecord> records;
    static int total_agenda_size = 0;
    for (int round = 0; round < _maxChainRounds && _dirty; ++round) { if(round == 7) std::cout << "MAX CHAIN ROUNDS HIT!" << std::endl;
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by _maxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        // Straight to the drain: the agenda is already complete. (An
        // _rete.evaluate() call sat here whose result was discarded — the
        // last trace of the rebuild-every-frame design.)
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        total_agenda_size += agenda.size();"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 12!")
else:
    print("Could not find string in Law.cpp!")
