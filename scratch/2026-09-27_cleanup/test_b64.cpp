#include <iostream>
#include <string>
#include <vector>

std::vector<uint8_t> base64Decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    int val=0, valb=-8;
    for (uint8_t c : in) {
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) {
            out.push_back(char((val>>valb)&0xFF));
            valb-=8;
        }
    }
    return std::vector<uint8_t>(out.begin(), out.end());
}

int main() {
    std::string b64(21848, 'A');
    std::cout << base64Decode(b64).size() << std::endl;
    return 0;
}
