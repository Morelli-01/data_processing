//
// Created by nicola on 15/03/2024.
//
#include <fstream>
#include <bitset>
#include "iostream"

using std::cout;
using std::endl;

int error(const char *reason) {
    cout << reason << endl;
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return error("The number of parameteres is different from 2!");
    }

    std::ifstream is(argv[1], std::ios::binary);
    if (is.fail()) {
        return error("Error while opening the input file!");
    }

    std::ofstream os(argv[2]);
    if (os.fail()) {
        return error("Error while opening the output file!");
    }
    std::string str{};
    char tmp;
    while (is.get(tmp).good()) {
        str += std::bitset<8>{static_cast<unsigned long>(tmp)}.to_string();
    }
    cout << str << endl;

    for (int i = 0; i != str.size() / 11; i++) {
        std::bitset<11> bset{str.substr(i * 11, 11)};
        int value = bset.to_ulong();
        if (value>1024){
            value = ~value +1025;
        }
//        cout << bset << endl;
        os << value << endl;
    }

    return 0;
}