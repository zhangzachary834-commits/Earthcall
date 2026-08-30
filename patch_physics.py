import sys
file_path = "src/ZonesOfEarth/Physics/Physics.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("rel->aId() == obj.getIdentifier()", "rel->aId() == objId")
content = content.replace("if(obj.getIdentifier()==id)", "if(objId==id)")

search_str = """        if (!t.explicitObjects.empty()) {
            for (auto* p : t.explicitObjects) if (p == &obj) return true;
            // If explicitObjects is provided, we treat it as the only set unless other filters also match
            // fallthrough to allow other filters as well
        }"""
insert_str = search_str + "\n        std::string objId = obj.getIdentifier();"
content = content.replace(search_str, insert_str)

with open(file_path, "w") as f:
    f.write(content)
