#include <iostream>
#include <fstream>
#include <vector>
#include "cstdlib"
#include <cinttypes>
#include <array>
#include "algorithm"

using namespace std;

template<typename T>
class Mat {
    size_t rows_;
    size_t cols_;
    vector<T> data_;
public:
    Mat(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        cout << "MAT CONSTRUCTOR HAS BEEN CALLED!!!" << endl;

        data_ = vector<T>(rows_ * cols_);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols_ + c];
    }

//    const T &operator()(size_t r, size_t c) const {
//        return data_[r * cols_ + c];
//    }

    vector<T> &dataVector() {
        return data_;
    }

    T *data() {
        return data_.data();
    }

    size_t &rows() {
        return rows_;
    }

    size_t &cols() {
        return cols_;
    }

    ~Mat() {
        cout << "MAT DECONSTRUCTOR HAS BEEN CALLED!!!" << endl;
    }
};

struct PGMHelper {

    static auto parsePGM(const string &path) {
        Mat<uint16_t> data(0, 0);
        ifstream is(path, std::ios::binary);
        if (is.fail()) {
            perror("Error while opening input file!");
            return data;
        }
        string tmp;
        is >> tmp;
        if (tmp != "P5") {
            return data;
        }
        int width;
        is >> width;
        int height;
        is >> height;
        int maxval;
        is >> maxval;
        cout << is.get();
        data = Mat<uint16_t>(height, width);
        is.read(reinterpret_cast<char *>(data.dataVector().data()), data.dataVector().size() * sizeof(uint16_t));
        std::for_each(data.dataVector().begin(), data.dataVector().end(), [](uint16_t &x) {
            x = std::byteswap(x);
        });
        return data;
    }

    static auto dumpPGM(Mat<uint8_t> &data, const string &path) {
        ofstream os(path + "_gray.pgm", std::ios::binary);
        if (os.fail()) {
            perror("Error while opening ouput file");
            return EXIT_FAILURE;
        }
        os << "P5\n";
        os << to_string(data.cols()) << ' ';
        os << to_string(data.rows()) << ' ';
        os << "255\n";
        os.write(reinterpret_cast<const char *>(data.dataVector().data()), data.rows() * data.cols());

        return EXIT_SUCCESS;
    }

    static auto dumpPGM(Mat<array<uint8_t, 3>> &data, const string &path) {
        ofstream os(path + "_gray.pgm", std::ios::binary);
        if (os.fail()) {
            perror("Error while opening ouput file");
            return EXIT_FAILURE;
        }
        os << "P5\n";
        os << to_string(data.cols()) << ' ';
        os << to_string(data.rows()) << ' ';
        os << "255\n";
        os.write(reinterpret_cast<const char *>(data.dataVector().data()), data.rows() * data.cols());

        return EXIT_SUCCESS;
    }
};

Mat<uint8_t> &quantize(Mat<uint16_t> &data16, Mat<uint8_t> &data8) {
    for (int r = 0; r < data8.rows(); ++r) {
        for (int c = 0; c < data8.cols(); ++c) {
            data8(r, c) = static_cast<uint8_t >(data16(r, c) / 256);
        }
    }
    return data8;
}

auto greenReconstruction(size_t r, size_t c, Mat<array<uint8_t, 3>> data, size_t index) {

    if (r < 2 or c < 2 or r >= data.rows() - 2 or c >= data.cols() - 2) {
        return 0;
    }

    auto tmp1 = data(r, c)[index] * 2 - data(r, c - 2)[index] - data(r, c + 2)[index];
    auto tmp2 = data(r, c)[index] * 2 - data(r - 2, c)[index] - data(r + 2, c)[index];

    auto deltaH = std::abs(data(r, c - 1)[1] - data(r, c + 1)[1]) +
                  std::abs(tmp1);
    auto deltaV = std::abs(data(r - 1, c)[1] - data(r + 1, c)[1]) +
                  std::abs(tmp2);

    if (deltaH < deltaV) {
        return (data(r, c - 1)[1] + data(r, c + 1)[1]) / 2 + tmp1 / 4;
    } else if (deltaH > deltaV) {
        return (data(r - 1, c)[1] + data(r + 1, c)[1]) / 2 + tmp2 / 4;
    } else {
        return (data(r - 1, c)[1] + data(r + 1, c)[1] + data(r, c - 1)[1] + data(r, c + 1)[1]) / 4 +
               (4 * data(r, c)[index] - data(r, c - 2)[index] - data(r, c + 2)[index] - data(r - 2, c)[index] -
                data(r + 2, c)[index]) / 8;
    }

}


int main(int argc, char **argv) {
    if (argc != 3) {
        perror("Error: just 2 parameters");
        return EXIT_FAILURE;
    }
    Mat pgmImage = PGMHelper::parsePGM(argv[1]);

    Mat<uint8_t> data8 = Mat<uint8_t>(pgmImage.rows(), pgmImage.cols());
    quantize(pgmImage, data8);

    PGMHelper::dumpPGM(data8, argv[2]);
    cout << endl;

    Mat<array<uint8_t, 3>> bayer_pattern(data8.rows(), data8.cols());

    for (int r = 0; r < bayer_pattern.rows(); ++r) {
        for (int c = 0; c < bayer_pattern.cols(); ++c) {
            if (r % 2 == 0) {
                if (c % 2 == 0) {
                    bayer_pattern(r, c)[0] = data8(r, c);
                } else {
                    bayer_pattern(r, c)[1] = data8(r, c);
                }

            } else {
                if (c % 2 == 0) {
                    bayer_pattern(r, c)[1] = data8(r, c);
                } else {
                    bayer_pattern(r, c)[2] = data8(r, c);
                }

            }
        }
    }
    PGMHelper::dumpPGM(bayer_pattern, "C:\\Users\\nicol\\Desktop\\data_processing\\bayer_decode\\D5100LL004003_color");


    return EXIT_SUCCESS;
}
