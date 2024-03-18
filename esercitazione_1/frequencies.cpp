//
// Created by nicola on 15/03/2024.
//
#include <fstream>
#include <iomanip>
#include <array>
#include "iostream"

using std::cout;
using std::endl;

int error(const char *reason) {
    cout << reason << endl;
    return -1;
}

struct stats {
    std::array<int, 256> _data{};

    void operator()(const unsigned char byte) {
        _data[byte]++;
    }

    int operator[](const unsigned char i) {
        return _data[i];
    }
};

int main(int argc, char **argv) {
    if (argc != 3) {
        return error("The number of parameters was different then 2!");
    }
    std::ifstream is(argv[1], std::ios::binary);
    if (is.fail()) {
        return error("Encountered problem while opening input file");
    }

    std::ofstream os(argv[2]);
    if (is.fail()) {
        return error("Encountered problem while opening output file");
    }

    char tmp;
    stats data;
    while (is.get(tmp)) {

        data(tmp);
    }

    for (auto i = 0; i != 256; i++) {
        if (data[i] != 0) {
            os << std::setw(2) << std::hex << i << "    " << data[i] << endl;
        }
    }

    return 0;
}