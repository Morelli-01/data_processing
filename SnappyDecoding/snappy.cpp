#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <iomanip>


#define LITERAL 0
#define COPY1BYTE 1
#define COPY2BYTE 2
#define COPY4BYTE 3

using namespace std;

uint16_t byteSwap(uint16_t x) {
    return (x >> 8) | ((x & 0x00FF) << 8);
}

uint32_t byteSwap(uint32_t x) {
    return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}

class BitReader {
    istream &is_;
    uint8_t buffer_ = 0;
    int len = 0;

    uint8_t readBit() {
        if (len == 0) {
            len = 8;
            buffer_ = is_.get();
        }
        len--;
        return buffer_ >> len & 1;
    }

public:
    int nByte = 0;

    explicit BitReader(istream &is) : is_(is) {}

    uint64_t operator()(int nBits) {
        uint64_t value = 0;
        for (int i = 0; i < nBits; ++i) {
            value = (value << 1) | readBit();
        }
        return value;
    }

    istream &getIs() const {
        return is_;
    }
};


class SnappyDecoder {
    BitReader br_;
    uint64_t originalSSize = 0;
    ostream &os_;
public:
    explicit SnappyDecoder(istream &is, ostream &os) : br_(is), os_(os) {}

    void parsePreamble() {
        bool flag = true;
        vector<uint8_t> bytes{};
        uint64_t value = 0;
        while (flag) {
            flag = br_(1);
            bytes.emplace_back(br_(7));
        }
        for (int i = bytes.size() - 1; i >= 0; i--) {
            value = (value << 7) | bytes.at(i);
        }
        originalSSize = value;
    }

    void parseData() {
        uint8_t tmpC;
        int length;
        while (br_.nByte < originalSSize) {
            cout << endl;
            tmpC = br_(8);
            int element = tmpC & 3;
            switch (element) {
                case LITERAL: {
                    uint64_t type = tmpC >> 2;
                    if (type > 59) {
                        uint64_t totalB = 0;
                        for (int i = 0; i < type - 59; ++i) {
                            auto c = br_(8);
                            totalB = totalB | (c << (8 * i));
                        }
                        type = totalB;
                    }
                    for (int i = 0; i <= type; ++i) {
                        os_.put(br_(8));
                        br_.nByte++;
                    }
                    break;
                }
                case COPY1BYTE: {
                    uint16_t offset = 0;
                    int length = ((tmpC & 0b00011100) >> 2) + 4;
                    offset = (offset << 3) | (tmpC >> 5);
                    offset = (offset << 8) | br_(8);
                    auto oldPos = br_.getIs().tellg();
                    auto pos = oldPos.operator-(offset + 2);
                    br_.getIs().seekg(pos);
                    for (int i = 0; i < length; ++i) {
                        os_.put(br_(8));
                        br_.nByte++;
                    }
                    br_.getIs().seekg(oldPos);
                    break;
                }
                case COPY2BYTE: {
                    uint16_t offset = 0;
                    int length = ((tmpC & 0b11111100) >> 2) + 1;
                    offset = br_(16);
                    offset = byteSwap(offset);
                    auto oldPos = br_.getIs().tellg();
                    auto pos = oldPos.operator-(offset + 1);
                    br_.getIs().seekg(pos);
                    for (int i = 0; i < length; ++i) {
                        os_.put(br_(8));
                        br_.nByte++;

                    }
                    br_.getIs().seekg(oldPos);

                    break;
                }
                case COPY4BYTE: {
                    uint32_t offset = 0;
                    int length = ((tmpC & 0b11111100) >> 2) + 1;
                    offset = br_(32);
                    offset = byteSwap(offset);
                    auto oldPos = br_.getIs().tellg();
                    auto pos = oldPos.operator-(offset + 5);
                    br_.getIs().seekg(pos);
                    for (int i = 0; i < length; ++i) {
                        os_.put(br_(8));
                        br_.nByte++;
                    }
                    br_.getIs().seekg(oldPos);
                    break;
                }
            }


        }
    }
};


int main(int argc, char **argv) {
    if (argc != 3) {
        perror("The snappy decoder must be invoked passing just 2 parameters:\n<input_file> <output_file>\n");
        return EXIT_FAILURE;
    }
    ifstream is(argv[1], ios::binary);
    ofstream os(argv[2], ios::binary);
    if (is.fail() or os.fail()) {
        perror("Error while trying to open input/output file!\n");
        return EXIT_FAILURE;
    }
    SnappyDecoder sd_(is, cout);
    sd_.parsePreamble();
    sd_.parseData();
    cout << endl;

    return EXIT_SUCCESS;

}
