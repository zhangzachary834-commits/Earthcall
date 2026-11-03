# Audio Sink and Law Feedback

**How hardware limitations push readable ontology feedback into the Law evaluation trace.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md` (Separating authored OntoMath intent from hardware geometry IR)

---

## The Interrelation

Earthcall establishes a rigid boundary between "Authored Truth" (Laws, OntoMath) and the "Physical Substrate" (GPU rendering, audio hardware). However, communication across this boundary is not strictly one-way.

### Hardware Limitations as Semantic Strings
In `ActionModel.hpp`, `AudioSink` callbacks use a function signature like `std::function<bool(Singular& subject, double frequency, double amplitude, const std::string& timbre, std::string& reason)>`. When the audio hardware refuses to play a sound (e.g., due to channel exhaustion or invalid frequencies), it does not just throw an opaque C++ exception or fail silently. It propagates a human-readable string `reason` back up.

### Ontological Legibility
This string is pushed back into the Law evaluation trace. By doing this, the hardware constraint is transformed into ontological feedback for the First Mover. Instead of a "black box" failure, the Law system can "read" the hardware's refusal, maintaining the No Black Box principle even when interfacing with closed OS-level audio drivers.
