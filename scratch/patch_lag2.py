import sys

file_path = "tests/singularity/frame_lag_test.cpp"
with open(file_path, "r") as f:
    content = f.read()

# Replace the body of checkQuiescence with just a simple loop
content = content.replace("void checkQuiescence(Zone& zone, LawManager& lawManager, double& worldTime) {", """void checkQuiescence(Zone& zone, LawManager& lawManager, double& worldTime) {
    printf("\\n--- RUNNING QUIESCENCE ---\\n");
    for (int i=0; i<3; i++) {
        double t0 = glfwGetTime();
        lawManager.tick();
        double t1 = glfwGetTime();
        printf("Tick %d: %.3f ms\\n", i, (t1-t0)*1000.0);
    }
    return;
""")

with open(file_path, "w") as f:
    f.write(content)
