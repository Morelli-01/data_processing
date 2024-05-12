//
// Created by nicola on 5/11/24.
//
#include <string>
#include <map>
#include "cstdlib"
#include "cstdint"
#include "iostream"
#include "fstream"
#include "mat.h"
#include "ppm.h"
#include <algorithm>

#define IDLE 0
#define RUN 1
#define COPY 2
#define EOD 128

using namespace std;

struct PPMHelper {

    static mat<vec3b> loadPPM(string file_path) {
        ifstream is(file_path, ios::binary);
        if (is.fail()) throw logic_error("error while opening input file\n");
        char tmpC;
        string tmpStr;
        map<uint8_t, int> header;
        is >> tmpStr;
        if (tmpStr != "P6") {
            throw logic_error("the opened file is not a ppm\n");
        }
        int count = 0;
        while (count < 3) {
            tmpStr.clear();
            is >> tmpStr;

            if (tmpStr.contains("#")) {
                while ((tmpC = is.get()) != '\n');
                continue;
            }
            header[count] = stoi(tmpStr);
            count++;
        }
        tmpC = is.get();
        mat<vec3b> data(header[1], header[0]);
        is.read(data.rawdata(), data.size() * sizeof(vec3b));
        return data;
    }

};

bool LoadPPM(const std::string &filename, mat<vec3b> &img) {

    try {
        img = PPMHelper::loadPPM(filename);
        return true;
    } catch (logic_error &error) {
        return false;
    }

}

void SplitRGB(const mat<vec3b> &img, mat<uint8_t> &img_r, mat<uint8_t> &img_g, mat<uint8_t> &img_b) {
    mat<uint8_t> img_R(img.rows(), img.cols());
    mat<uint8_t> img_G(img.rows(), img.cols());
    mat<uint8_t> img_B(img.rows(), img.cols());
    for (int r = 0; r < img.rows(); ++r) {
        for (int c = 0; c < img.cols(); ++c) {
            img_R(r, c) = img(r, c)[0];
            img_G(r, c) = img(r, c)[1];
            img_B(r, c) = img(r, c)[2];
        }
    }
    img_r = img_R;
    img_g = img_G;
    img_b = img_B;
}

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

int main() {
    string file_path = "/home/nicola/Desktop/data_processing/exam7/files/test.ppm";
    mat<vec3b> data;
    mat<uint8_t> img_R;
    mat<uint8_t> img_G;
    mat<uint8_t> img_B;
    LoadPPM(file_path, data);
    SplitRGB(data, img_R, img_G, img_B);
    vector<uint8_t> encodedR{};
    vector<uint8_t> encodedG{};
    vector<uint8_t> encodedB{};
    PackBitsEncode(img_R, encodedR);
    PackBitsEncode(img_G, encodedG);
    PackBitsEncode(img_B, encodedB);
    string base64R = Base64Encode(encodedR);
    string base64G = Base64Encode(encodedG);
    string base64B = Base64Encode(encodedB);
    cout << endl;
    return EXIT_SUCCESS;
}
