import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("""    for (Law* law : activeLaws) {""", """    for (Law* law : activeLaws) {
        static int count_active = 0;
        count_active++;
        if (count_active == 1000) {
            printf("--- REACHED 1000 ACTIVE LAWS ---\\n");
        }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
