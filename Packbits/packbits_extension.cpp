//
// Created by nicola on 03/04/24.
//
//
// Created by nicola on 02/04/2024.
//


#include <cstdlib>
#include <fstream>
#include <cstdint>
#include <vector>
#include "iostream"

#define UNINITIALIZED_STATE 0
#define INITIAL_STATE 1
#define NRUN_STATE 2
#define RUN_STATE 3
#define UNSTABLE_STATE 4
#define END_STATE 5

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
    int state = INITIAL_STATE;
    int old_state = INITIAL_STATE;
public:
    explicit PackbitsWriter(std::ostream &os_, std::istream &is_) : is_(is_), os_(os_) {}

    void operator()() {
        uint8_t lhs = is_.get();
        uint8_t rhs;
        std::vector<uint8_t> values{};
        outerWhile:
        while (is_.good()) {
//            if (lhs == 0x0D) {
//                rhs = 0x0A;
//            } else {
//                rhs = is_.get();
//                if (rhs == 0x0A) {
//                    rhs = 0x0D;
//                }
//            }
            rhs = is_.get();

            if (!is_.good()) {
                old_state = state;
                state = END_STATE;
                values.push_back(lhs);
            }
            switch (state) {
                case INITIAL_STATE:
                    values.clear();
                    if (lhs != rhs) state = NRUN_STATE;
                    else state = UNSTABLE_STATE;
                    values.push_back(lhs);
                    break;
                case UNSTABLE_STATE:
                    if (lhs == rhs) {
                        state = RUN_STATE;
                        if (old_state == NRUN_STATE) {
                            values.pop_back();
                            printNRun(values.size(), values);
                            values.clear();
                            values.push_back(lhs);
                            old_state = INITIAL_STATE;
                        }
                    } else {
                        state = NRUN_STATE;
                    }
                    values.push_back(lhs);
                    break;

                case RUN_STATE:
                    if (lhs != rhs or values.size() >= 127) {
                        values.push_back(lhs);
                        printRun(values.size(), values[0]);
                        state = INITIAL_STATE;
                    }
                    values.push_back(lhs);
                    break;
                case NRUN_STATE:
                    if (lhs == rhs) {
                        state = UNSTABLE_STATE;
                        old_state = NRUN_STATE;
                    }
                    values.push_back(lhs);
                    if (values.size() >= 128) {
                        printNRun(values.size(), values);
                        state = INITIAL_STATE;
                    }
                    break;
                default:
                    if (old_state == NRUN_STATE) {
                        printNRun(values.size(), values);
                    } else {
                        printRun(values.size(), values[0]);
                    }
                    values.push_back(lhs);
                    break;

            }

            lhs = rhs;
        }

        os_.put(END_CHARACTER);

    }

    void printRun(uint8_t len, uint8_t c) {
        os_.put(257 - len);
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