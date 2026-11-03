import re

with open("tests/law/synthesis_studio_app_test.cpp", "r") as f:
    text = f.read()

patch = """
        std::cout << "lastStrokeX is " << readNumber(*stateStudio, "lastStrokeX") << "\\n";
        auto v_opt = stateStudio->getProperty("lastStrokeX");
        if (v_opt) {
            std::cout << "lastStrokeX prop exists! value=" << v_opt->asNumber().value_or(-999) << "\\n";
        } else {
            std::cout << "lastStrokeX prop DOES NOT EXIST!\\n";
        }
"""
text = text.replace('std::cout << "lastStrokeX: " << readNumber(*stateStudio, "lastStrokeX") << std::endl;', patch)

with open("tests/law/synthesis_studio_app_test.cpp", "w") as f:
    f.write(text)
