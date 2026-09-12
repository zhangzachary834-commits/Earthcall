import sys

file_path = "tests/singularity/frame_lag_test.cpp"
with open(file_path, "r") as f:
    content = f.read()

target = """    std::vector<PopulationCost> costs;
    for (double n : populations) {
        costs.push_back(costForPopulation(static_cast<int>(n), 300.0));
        const PopulationCost& c = costs.back();
        std::printf("  %5d objects -> frame %8.3f ms  "
                    "(zone %8.3f [g:%.3f r:%.3f a:%.3f p:%.3f]  relations %6.3f  law %8.3f)  over %d frames\\n",
                    static_cast<int>(n), c.total, c.zone, c.groundScan, c.rotation, c.automation, c.physics, c.relations, c.law, c.frames);
    }"""

replacement = """    std::vector<PopulationCost> costs;
    for (double n : populations) {
        //costs.push_back(costForPopulation(static_cast<int>(n), 300.0));
        //const PopulationCost& c = costs.back();
        continue;
    }"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, "w") as f:
        f.write(content)
    print("Skipped scaling")
else:
    print("Could not find scaling code")
