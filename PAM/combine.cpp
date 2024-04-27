//
// Created by nicola on 25/04/2024.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include "algorithm"

using namespace std;
using Pixel = array<uint8_t, 3>;

template<typename T>
struct Mat {
    size_t rows_;
    size_t cols_;
    vector<T> data_;

    Mat(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_ = vector<T>(cols * rows);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols_ + c];
    }

    const T &operator()(size_t r, size_t c) const {
        return data_[r * cols_ + c];
    }

    T *data() {
        return data_.data();
    }

    const T *data() const {
        return data_.data();
    }

    size_t size() {
        return cols_ * rows_;
    }

};


struct PamHelper {

    static Mat<uint8_t> loadPamGray(ifstream &is) {
        Mat<uint8_t> mat(0, 0);
        string tmpStr{};
        is >> tmpStr;
        if (tmpStr != "P7") {
            return mat;
        }
        is >> tmpStr;
        map<string, size_t> header_{};
        string tmpV{};
        while (tmpStr != "ENDHDR") {
            is >> tmpV;

            if (tmpStr == "TUPLTYPE") {
                is >> tmpStr;
                continue;
            }
            header_[tmpStr] = atoi(tmpV.c_str());
            is >> tmpStr;
        }
        is.get();

//        mat = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
        mat.rows_ = header_["HEIGHT"];
        mat.cols_ = header_["WIDTH"];
        mat.data_ = vector<uint8_t>(mat.rows_ *mat.cols_);

        is.read(reinterpret_cast<char *>(mat.data()), mat.cols_*mat.rows_);

        return mat;
    }

    static void dumpPamGray(Mat<uint8_t>& matR, Mat<uint8_t>& matG, Mat<uint8_t>& matB, ofstream &os) {

        os.write("P7", 2);
        os.put('\n');

        os.write("WIDTH ", 6);
        os.write(to_string(matR.cols_).c_str(), to_string(matR.cols_).size());
        os.put('\n');

        os.write("HEIGHT ", 7);
        os.write(to_string(matR.rows_).c_str(), to_string(matR.rows_).size());
        os.put('\n');

        os.write("DEPTH ", 6);
        os.write("3", 1);
        os.put('\n');

        os.write("MAXVAL ", 7);
        os.write("255", 3);
        os.put('\n');

        os.write("TUPLTYPE  ", 9);
        os.write("RGB", 3);
        os.put('\n');

        os.write("ENDHDR", 6);
        os.put('\n');
        for (int i = 0; i < matR.cols_ * matR.rows_; ++i) {
            os.put(matR.data_[i]);
            os.put(matG.data_[i]);
            os.put(matB.data_[i]);
        }

    }
};


int main(int argc, char **argv) {

    if (argc != 2) {
        perror("Error a different number from 2 parameters has been passed!!\n");
        return EXIT_FAILURE;
    }

    string file_path{argv[1]};




    ifstream isR(file_path + "_R.pam", ios::binary);
    if (isR.fail()) {
        return EXIT_FAILURE;
    }
    Mat<uint8_t> matR = PamHelper::loadPamGray(isR);
    isR.close();

    ifstream isG(file_path + "_G.pam", ios::binary);
    if (isG.fail()) {
        return EXIT_FAILURE;
    }
    Mat<uint8_t> matG = PamHelper::loadPamGray(isG);
    isG.close();

    ifstream isB(file_path + "_B.pam", ios::binary);
    if (isB.fail()) {
        return EXIT_FAILURE;
    }
    Mat<uint8_t> matB = PamHelper::loadPamGray(isB);
    isB.close();

    ofstream os(file_path + "_reconstructed.pam", ios::binary | ios::trunc);
    if (os.fail()) {
        return EXIT_FAILURE;
    }
    PamHelper::dumpPamGray(matR, matG, matB, os);
    os.close();

    return EXIT_SUCCESS;
}
