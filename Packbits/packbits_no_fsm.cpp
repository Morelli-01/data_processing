//
// Created by nicola on 02/04/2024.
//


#include <cstdlib>
#include <fstream>
#include <cstdint>
#include <vector>
#include "iostream"

using std::cout;
using std::endl;
#define END_CHARACTER 128

int error(const std::string &reason) {
    cout << reason << endl;
    return EXIT_FAILURE;
}

class PackbitsWriter {
    std::ostream &os_;
    std::istream &is_;

public:
    explicit PackbitsWriter(std::ostream &os_, std::istream &is_) : is_(is_), os_(os_) {}

    void operator()() {
        uint8_t lhs = is_.get();
        uint8_t rhs;
        int run = 0;
        uint8_t nRun = 0;
        std::vector<uint8_t> values{};
        while (lhs != 255) {
            rhs = is_.get();
            if (lhs == rhs) {
                if (nRun > 0) {
                    printNRun(nRun, values);
                    nRun = 0;
                    values.clear();
                }

                run++;
                if (run >= 128) {
                    printRun(run - 1, lhs);
                    run = 0;
                }
            } else {
                if (run > 0) {
                    printRun(run, lhs);
                    run = 0;
                    lhs = rhs;
                    continue;
                }
                values.push_back(lhs);
                nRun++;
                if (nRun >= 128) {
                    printNRun(nRun, values);
                    nRun = 0;
                    values.clear();
                }
            }
            lhs = rhs;
        }
        if (run > 0) printRun(run, lhs);
        else if (nRun > 0) printNRun(nRun, values);

        os_.put(END_CHARACTER);

    }

    void printRun(uint8_t len, uint8_t c) {
        os_.put(257 - (len + 1));
        os_.put(c);
    }

    void printNRun(uint8_t len, const std::vector<uint8_t> &values) {

        os_.put(len - 1);
        for (int j = 0; j < len; ++j) {
            os_.put(values[j]);
        }
    }
};

class PackbitsReader {
    std::istream &is_;
    std::ostream &os_;


public:
    explicit PackbitsReader(std::istream &is_, std::ostream &os_) : os_(os_), is_(is_) {}

    void operator()() {
        uint8_t len;
        while ((len = is_.get()) != 128 and is_.good()) {
//            if (len == 255)break;
            if (len >= 129 and len <= 255) {
                len = 257 - len;
                uint8_t c = is_.get();
                for (int i = 0; i < len; ++i) {
                    os_.put(c);
                }
            } else if (len >= 0 and len <= 127) {
                for (int i = 0; i <= len; ++i)
                    os_.put(static_cast<char>(is_.get()));
            }
        }
    }
};

int main(int argc, char **argv) {
    if (argc != 4) {
        return error(std::string{"ERROR: the program must be invoked with 3 parameters!"});
    }
    std::ifstream is(argv[2], std::ios::binary);
    std::ofstream os(argv[3], std::ios::binary);
    if (!is) {
        return error("ERROR: invalid input file");
    }
    if (!os) {
        return error("ERROR: invalid output file");
    }
    if (std::string{argv[1]} == "c") {
        PackbitsWriter pw_(os, is);
        pw_();
    } else if (std::string{argv[1]} == "d") {
        PackbitsReader pr_(is, os);
        pr_();
    } else { return EXIT_FAILURE; }


    return EXIT_SUCCESS;
}