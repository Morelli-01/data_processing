//
// Created by nicola on 5/12/24.
//

#include "cstdlib"
#include "cstdint"
#include "vector"
#include "mat.h"

#define IDLE 0
#define RUN 1
#define COPY 2
#define EOD 128

using namespace std;

void PackBitsEncode(const mat<uint8_t> &img, std::vector<uint8_t> &encoded) {
    vector<uint8_t> encodedBuf{};
    vector<uint8_t> copyBuf{};
    uint8_t runElem;
    int status = IDLE;
    int counter = 0;
    for (int i = 0; i < img.size(); ++i) {
        uint8_t elem = img.rawdata()[i];
        uint8_t nextElem = img.rawdata()[i + 1];
        status = elem == nextElem ? RUN : COPY;

        switch (status) {
            case COPY: {
                counter = -1;
                do {
                    copyBuf.push_back(elem);
                    counter++;

                    i++;
                    elem = img.rawdata()[i];
                    nextElem = img.rawdata()[i + 1];

                    if (counter == 127 or i == img.size()) {
                        break;
                    }
                } while (elem != nextElem);
                encodedBuf.push_back(counter);
                for (auto &v: copyBuf) {
                    encodedBuf.push_back(v);
                }
                copyBuf.clear();
                counter = 0;
                i--;
                break;
            }
            case RUN: {
                i++;
                runElem = elem;
                uint8_t elemPrec = img.rawdata()[i];
                counter = 1;
                do {
                    counter++;
                    i++;
                    elem = img.rawdata()[i];
                    elemPrec = img.rawdata()[i - 1];
                    if (counter == 128 or i == img.size()) {
                        break;
                    }
                } while (elem == elemPrec);
                encodedBuf.push_back(257 - counter);
                encodedBuf.push_back(runElem);
                counter = 0;
                i--;
                break;
            }

        }


    }
    encodedBuf.push_back(EOD);
    encoded = encodedBuf;
}

std::string Base64Encode(const std::vector<uint8_t> &v) {
    auto vCopy = v;
    while (vCopy.size() % 3 != 0) {
        vCopy.push_back(0x80);
    }

    string out = "";
    for (int i = 0; i < vCopy.size() / 3; ++i) {
        uint32_t byteBuf = 0;
        byteBuf = (byteBuf << 8) | vCopy[i * 3];
        byteBuf = (byteBuf << 8) | vCopy[i * 3 + 1];
        byteBuf = (byteBuf << 8) | vCopy[i * 3 + 2];

        array<uint8_t, 4> groups{};
        groups[0] = (byteBuf >> 18) & 63;
        groups[1] = (byteBuf >> 12) & 63;
        groups[2] = (byteBuf >> 6) & 63;
        groups[3] = (byteBuf >> 0) & 63;

        for (auto &item: groups) {
            if (0 <= item and item <= 25) item += 65;
            else if (26 <= item and item <= 51)item += 71;
            else if (52 <= item and item <= 61)item -= 4;
            else if (item == 62) item = 43;
            else if (item == 63) item = 47;
            out += static_cast<char>(item);
        }

    }
    return out;
}


//
//int main() {
//    ifstream is("/home/nicola/Desktop/data_processing/exam7/files/compressme3.txt", ios::binary);
//    if (is.fail())return EXIT_FAILURE;
//    vector<uint8_t> data{};
//    char c;
//    while ((c = is.get()) != EOF) {
//        data.push_back(c);
//    }
//    vector<uint8_t> encoded;
//    PackBitsEncode(data, encoded);
//    ofstream os("/home/nicola/Desktop/data_processing/exam7/files/out.bin", ios::binary | ios::trunc);
//    os.write(reinterpret_cast<const char *>(encoded.data()), encoded.size());
//    os.close();
//    return EXIT_SUCCESS;
//}