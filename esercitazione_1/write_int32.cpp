//
// Created by nicola on 15/03/2024.
//

#include <fstream>
#include "iostream"

using std::cout;
using std::endl;

int error(const char *reason) {
    cout << reason << endl;
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return error("The number of parameters was different then 2!");
    }
    std::ifstream is(argv[1]);
    if (is.fail()) {
        return error("Encountered problem while opening input file");
    }

    std::ofstream os(argv[2], std::ios::binary);
    if (is.fail()) {
        return error("Encountered problem while opening output file");
    }

    int tmp;
    while (is >> tmp) {
        os.write(reinterpret_cast<char *>(&tmp), 4);
//        cout << endl;
    }

}