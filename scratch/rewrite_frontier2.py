with open("docs/architecture/law/LAW_EXECUTION_FRONTIER.md", "r") as f:
    content = f.read()

jit_addition = """
* **The Engine of Causality (CPU Prophetic JIT):** The CPU abandons the Bytecode VM and uses LLVM to compile authored Laws directly into native x86_64/ARM machine code at runtime. By leveraging the existing **Prophetic Rete** to mathematically guarantee memory shapes, this JIT achieves true 1.0x native C++ speeds (see Analysis doc for the mathematical proof). It calculates all causality, Rete propagation, and structural state changes instantly.
  * *Important Constraint:* LLVM is a massive dependency, and JIT compilation requires writable-then-executable (W^X) memory pages. This requires specific JIT entitlements on Apple platforms and is strictly prohibited on many modern consoles. For these platforms, the JIT degrades gracefully back to the C++ Bytecode VM, which remains the authoritative and portable execution form.
"""

content = content.replace(
    "* **The Engine of Causality (CPU Prophetic JIT):** The CPU abandons the Bytecode VM and uses LLVM to compile authored Laws directly into native x86_64/ARM machine code at runtime. By leveraging the existing **Prophetic Rete** to mathematically guarantee memory shapes, this JIT achieves true 1.0x native C++ speeds (see Analysis doc for the mathematical proof). It calculates all causality, Rete propagation, and structural state changes instantly.",
    jit_addition.strip()
)

with open("docs/architecture/law/LAW_EXECUTION_FRONTIER.md", "w") as f:
    f.write(content)
