#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <map>
#include <valarray>

#define IDLE 0
#define RUN 1
#define END 2
#define WRITE 3
using namespace std;

class BitWriter {
    ostream &os_;
    uint8_t buffer_;
    size_t len = 0;
    int bytesWritten = 0;

    ostream &writeBit(uint8_t bit) {
        buffer_ = (buffer_ << 1) | bit;
        len++;
        if (len >= 8) {
            os_.put(buffer_);
            buffer_ = 0;
            len = 0;
            bytesWritten++;
//            if (bytesWritten >= 517) {
//                cout << endl;
//            }
        }
        return os_;
    }

    void flush() {
        while (len != 0) {
            writeBit(0);
        }
    }

public:
    explicit BitWriter(ostream &os) : os_(os), buffer_{} {}

    ostream &operator()(uint64_t value, size_t nBits) {
        for (int i = nBits - 1; i >= 0; i--) {
            writeBit((value >> i) & 1);
        }
        return os_;
    }

    ostream &operator()(const string &str) {
        for (const char &x: str) {
            for (int i = 8 - 1; i >= 0; i--) {
                writeBit((x >> i) & 1);
            }
        }
        return os_;
    }

    ~BitWriter() {
        flush();
    }
};



bool lz78encode(const std::string &input_filename, const std::string &output_filename, int maxbits) {
    bool logging = false;
    if (maxbits < 1 or maxbits > 30) {
        return false;
    }
    ifstream is(input_filename, ios::binary);
    ofstream os(output_filename, ios::binary);
    if (is.fail() or os.fail()) {
        perror("ERROR WHILE OPENING INPUT/OUTPUT FILE!!!\n");
        return false;
    }


    BitWriter bw_(os);

    bw_("LZ78");
    bw_(maxbits, 5);
    int state = IDLE;
    int oldState = IDLE;
    int lastEntry = 0;
    int dim = 0;
    char tmpChar;
    string tmpStr{};
    map<string, uint8_t> dict{};
    while (is.good()) {
        tmpChar = is.get();
        if (tmpChar != EOF) {
            tmpStr += tmpChar;
        } else {
            oldState = state;
            state = END;
        }

        switch (state) {
            case IDLE:
                if (dict.find(tmpStr) != dict.end()) {
                    state = RUN;
                    lastEntry = dict.find(tmpStr)->second;
                    break;
                } else {
                    state = WRITE;
                }

            case RUN:
                if (dict.find(tmpStr) == dict.end()) {
                    state = WRITE;
                } else {
                    lastEntry = dict.find(tmpStr)->second;
                    break;
                }
            case WRITE:
                if (logging) {
                    cout << "(" << lastEntry << "," << tmpChar << ")" << endl;
                }
                bw_(lastEntry, dim);
                bw_(tmpChar, 8);

                dict.insert({tmpStr, dict.size() + 1});
                lastEntry = 0;
                tmpStr = "";
                state = IDLE;
                dim = static_cast<int>(log2(dict.size())) + 1;
                if (pow(2, maxbits) == dict.size()) {
                    dim = 0;
                    dict.clear();
                    if (logging) {
                        cout << "[svuoto]" << endl;
                    }
                }
            case END:
                if (oldState == RUN) {
                    char lastC = tmpStr.back();
                    tmpStr.pop_back();
                    lastEntry = dict.find(tmpStr) == dict.end() ? 0 : (int) (dict.find(tmpStr)->second);
                    if (logging) {
                        cout << "(" << lastEntry << "," << lastC << ")" << endl;
                    }
                    bw_(lastEntry, dim);
                    bw_(lastC, 8);
                }
                break;
        }
    }
    return true;
}

//int main() {
//    return lz78encode(R"(C:\Users\nicol\Desktop\data_processing\LZ78\bibbia.txt)",
//                        R"(C:\Users\nicol\Desktop\data_processing\LZ78\output_test1.bin)", 10);
//
//}
