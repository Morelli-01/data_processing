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
        data_ = vector<T>(cols_ * rows_);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols_ + c];
    }

    const T &operator()(size_t r, size_t c) const {
        return data_[r * cols_ + c];
    }

    auto data() {
        return data_.data();
    }


};


struct PamHelper {

    static Mat<Pixel> loadPam(istream &is) {
        Mat<Pixel> mat(0, 0);
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
        mat = Mat<Pixel>(header_["HEIGHT"], header_["WIDTH"]);
        is.read(reinterpret_cast<char *>(mat.data()), header_["HEIGHT"] * header_["WIDTH"] * 3);

        cout << endl;
        return mat;
    }

    static void dumpRGB(string file_path, Mat<Pixel> &data) {

        vector<ofstream> os{};
        os.emplace_back(file_path + "_R.pam", ios::binary | ios::trunc);
        os.emplace_back(file_path + "_G.pam", ios::binary | ios::trunc);
        os.emplace_back(file_path + "_B.pam", ios::binary | ios::trunc);
        if (os[0].fail() or os[1].fail() or os[2].fail()) {
            perror("Error while opening output file!!\n");
            return;
        }
        for (int i = 0; i < 3; ++i) {
            ofstream& os1 = os[i];
            os1.write("P7", 2);
            os1.put('\n');

            os1.write("WIDTH ", 6);
            os1.write(to_string(data.cols_).c_str(), to_string(data.cols_).size());
            os1.put('\n');

            os1.write("HEIGHT ", 7);
            os1.write(to_string(data.rows_).c_str(), to_string(data.rows_).size());
            os1.put('\n');

            os1.write("DEPTH ", 6);
            os1.write("1", 1);
            os1.put('\n');

            os1.write("MAXVAL ", 7);
            os1.write("255", 3);
            os1.put('\n');

            os1.write("TUPLTYPE  ", 9);
            os1.write("GRAYSCALE", 9);
            os1.put('\n');

            os1.write("ENDHDR", 6);
            os1.put('\n');

            std::for_each(data.data_.begin(), data.data_.end(),[&](Pixel& p){
                os1.put(p[i]);
            });

        }


    }

    static Mat<uint8_t> loadPamGray(istream &is) {
        Mat<uint8_t> mat(0,0);
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
        mat = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
        is.read(reinterpret_cast<char *>(mat.data()), header_["HEIGHT"] * header_["WIDTH"]);

        cout << endl;
        return mat;
    }

};


int main(int argc, char **argv) {

    if (argc != 2) {
        perror("Error a different number from 2 parameters has been passed!!\n");
        return EXIT_FAILURE;
    }

    ifstream is(argv[1], ios::binary);
    if (is.fail()) {
        perror("Error while opening input file!!\n");
        return EXIT_FAILURE;
    }
    string file_path{argv[1]};
    file_path = file_path.replace(file_path.find(".pam"), 4, "");


    Mat<Pixel> img = PamHelper::loadPam(is);
    PamHelper::dumpRGB(file_path, img);

    return EXIT_SUCCESS;
}
