#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <queue>
#include <chrono>

#define LITERAL 0
#define COPY1BYTE 1
#define COPY2BYTE 2
#define COPY4BYTE 3

using namespace std;
using namespace std::chrono;

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
//    ofstream os_;
    fstream fs_;
    string outputPath;
//    queue<uint8_t> outBuffer{};
public:
    explicit SnappyDecoder(istream &is, const string &oPath) : br_(is), outputPath(oPath) {
//        os_ = ofstream(oPath, ios::binary | ios::trunc);
        fs_ = fstream(oPath, ios::binary | ios::trunc | ios::in | ios::out);
    }

    void parseCopy(int length, uint32_t offset) {
        vector<uint8_t> byteBuff(length);
//        os_.close();
        auto oldPos = fs_.tellp();
        auto newPos = oldPos.operator-(offset);
        fs_.seekp(newPos);
        fs_.read(reinterpret_cast<char *>(byteBuff.data()), length * sizeof(uint8_t));
        fs_.seekg(oldPos);
        fs_.write(reinterpret_cast<const char *>(byteBuff.data()), byteBuff.size() * sizeof(uint8_t));
//        ifstream tmpstream(outputPath, ios::binary | ios::ate);
//        tmpstream.seekg(tmpstream.tellg().operator-(offset));
//        tmpstream.read(reinterpret_cast<char *>(byteBuff.data()), length * sizeof(uint8_t));
        br_.nByte += length;
//        tmpstream.close();
//        os_ = ofstream(outputPath, ios::binary | ios::app);
//        os_.write(reinterpret_cast<const char *>(byteBuff.data()), byteBuff.size() * sizeof(uint8_t));
    }


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
//            cout << endl;
            tmpC = br_(8);
            int state = tmpC & 3;
            switch (state) {
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
                        auto t = br_(8);
                        fs_.put(t);
//                        cout.put(t);
                        br_.nByte++;
                    }
                    break;
                }
                case COPY1BYTE: {
                    uint16_t offset = 0;
                    int length = ((tmpC & 0b00011100) >> 2) + 4;
                    offset = (offset << 3) | (tmpC >> 5);
                    offset = (offset << 8) | br_(8);
                    parseCopy(length, offset);
//                    parseBuff(length, offset);
                    break;
                }
                case COPY2BYTE: {
                    uint16_t offset = 0;
                    int length = ((tmpC & 0b11111100) >> 2) + 1;
                    offset = br_(16);
                    offset = byteSwap(offset);
                    parseCopy(length, offset);
                    break;
                }
                case COPY4BYTE: {
                    uint32_t offset = 0;
                    int length = ((tmpC & 0b11111100) >> 2) + 1;
                    offset = br_(32);
                    offset = byteSwap(offset);
                    parseCopy(length, offset);
                    break;
                }
            }


        }
        fs_.close();
    }
};


int main(int argc, char **argv) {
    auto start = steady_clock::now();


    if (argc != 3) {
        perror("The snappy decoder must be invoked passing just 2 parameters:\n<input_file> <output_file>\n");
        return EXIT_FAILURE;
    }
    ifstream is(argv[1], ios::binary);
    if (is.fail()) {
        perror("Error while trying to open input file!\n");
        return EXIT_FAILURE;
    }
    SnappyDecoder sd_(is, argv[2]);
    sd_.parsePreamble();
    sd_.parseData();

//    fstream fs(argv[2], ios::binary | ios::in | ios::out | ios::trunc);
//    fs << "This is the first sentence\n";
//    fs << "This is the second sentence\n";
//    fs.seekp(ios::beg);
//    char tmp;
//    while (((tmp = fs.get()) | true) & fs.good())
//        cout << tmp;

    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to execute decompression was :" << elapsed_ms.count() << "ms" << endl;
    return EXIT_SUCCESS;

}
