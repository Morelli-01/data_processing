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

    std::ifstream is(argv[1]);
    if (is.fail()) {
        return error("Error while opening the input file!");
    }

    std::ofstream os(argv[2], std::ios::binary);
    if (os.fail()) {
        return error("Error while opening the output file!");
    }
    int tmp;
    std::string str{};
    while (is >> tmp) {
//        cout << tmp << endl;
        unsigned int tmp2;
        if (tmp < 0) {
            tmp2 = ~tmp + 1025;
        }else{
            tmp2 = tmp;
        }

        cout << std::bitset<11>{tmp2}.to_ulong() << endl;
        str += std::bitset<11>{tmp2}.to_string();

    }
    cout << str << endl;
    while (str.size() % 8 != 0) {
        str.append("0");
    }
    cout << str << endl;

    for (int i = 0; i != str.size() / 8; i++) {
        char tmp = std::bitset<8>{str.substr(8 * i, 8)}.to_ulong();
//        cout << tmp << endl;
        os.write(&tmp, 1);
    }
//    cout << str.size() << endl;
    return 0;
}