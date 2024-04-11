#include <iostream>
#include <fstream>
#include <vector>
#include "cstdlib"
#include <cinttypes>
#include <array>
#include <algorithm>
#include <chrono>
#include <memory>
    
using namespace std::chrono;
using namespace std;

#define RED 0
#define GREEN 1
#define BLUE 2

template<typename T>
class Mat {
    size_t rows_;
    size_t cols_;
    vector<T> data_;
    T *zero_;
public:
    Mat(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_ = vector<T>(rows_ * cols_);
        zero_ = nullptr;
    }

    Mat(const Mat &data) {
        cout << "copy constructor was called" << endl;
    }

    T &operator()(int r, int c) {
        if (zero_ != nullptr) {

            if (r < 0 or c < 0 or r >= rows_ or c >= cols_) {
                return *zero_;
            }
        }
        return data_[r * cols_ + c];
    }

    const T &operator()(int r, int c) const {
        if (r < 0 or c < 0 or r >= rows_ or c >= cols_) {
            return zero_;
        }
        return data_[r * cols_ + c];
    }

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

    void setZero(T *zero) {
        zero_ = zero;
    }


};

uint16_t byteSwap(uint16_t x) {
    return (x >> 8) | ((x & 0x00FF) << 8);
}

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
        is.get();
        data = Mat<uint16_t>(height, width);
        is.read(reinterpret_cast<char *>(data.dataVector().data()), data.dataVector().size() * sizeof(uint16_t));
        std::for_each(data.dataVector().begin(), data.dataVector().end(), [](uint16_t &x) {
            x = byteSwap(x);
        });
        return data;
    }

    static auto dumpPGM(Mat<uint8_t> &data, const string &path, const string &prefix) {
        ofstream os(path + prefix + ".pgm", std::ios::binary);
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

    static auto dumpPPM(Mat<array<uint8_t, 3>> &data, const string &path, const string &prefix) {
        ofstream os(path + prefix + ".ppm", std::ios::binary);
        if (os.fail()) {
            perror("Error while opening ouput file");
            return EXIT_FAILURE;
        }
        os << "P6\n";
        os << to_string(data.cols()) << ' ';
        os << to_string(data.rows()) << ' ';
        os << "255\n";
        os.write(reinterpret_cast<const char *>(data.dataVector().data()), data.rows() * data.cols() * 3);

        return EXIT_SUCCESS;
    }
};

Mat<uint8_t> &quantize(Mat<uint16_t> &data16, Mat<uint8_t> &data8) {
    for (int r = 0; r < data8.rows(); ++r) {
        for (int c = 0; c < data8.cols(); ++c) {
            data8(r, c) = clamp(data16(r, c) / 255, 0, UINT8_MAX);
        }
    }
    return data8;
}

void greenReconstruction(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {


    auto tmp1 = data(r, c)[index] * 2 - data(r, c - 2)[index] - data(r, c + 2)[index];
    auto tmp2 = data(r, c)[index] * 2 - data(r - 2, c)[index] - data(r + 2, c)[index];

    auto deltaH = std::abs(data(r, c - 1)[GREEN] - data(r, c + 1)[GREEN]) + std::abs(tmp1);
    auto deltaV = std::abs(data(r - 1, c)[GREEN] - data(r + 1, c)[GREEN]) + std::abs(tmp1) + std::abs(tmp2);

    if (deltaH < deltaV) {
        data(r, c)[GREEN] = clamp((data(r, c - 1)[GREEN] + data(r, c + 1)[GREEN]) / 2 + (tmp1) / 4, 0, UINT8_MAX);
    } else if (deltaH > deltaV) {
        data(r, c)[GREEN] = clamp((data(r - 1, c)[GREEN] + data(r - 1, c)[GREEN]) / 2 + (tmp2) / 4, 0, UINT8_MAX);
    } else {
        data(r, c)[GREEN] = clamp(
                (data(r, c - 1)[GREEN] + data(r, c + 1)[GREEN] + data(r - 1, c)[GREEN] + data(r - 1, c)[GREEN]) / 4 +
                (tmp1 + tmp2) / 8, 0, UINT8_MAX);
    }

}

void avarages_vert(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {
    data(r, c)[index] = (data(r - 1, c)[index] + data(r + 1, c)[index]) / 2;
}

void avarages_horiz(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {
    data(r, c)[index] = (data(r, c - 1)[index] + data(r, c + 1)[index]) / 2;
}

void redBluereconstruction(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {

    auto tmp1 = data(r, c)[GREEN] * 2 - data(r - 1, c - 1)[GREEN] - data(r + 1, c + 1)[GREEN];
    auto tmp2 = data(r, c)[GREEN] * 2 - data(r - 1, c + 1)[GREEN] - data(r + 1, c - 1)[GREEN];


    auto deltaN = abs(data(r - 1, c - 1)[index] - data(r + 1, c + 1)[index]) + abs(tmp1);
    auto deltaP = abs(data(r - 1, c + 1)[index] - data(r + 1, c - 1)[index]) + abs(tmp2);


    if (deltaN < deltaP) {
        data(r, c)[index] = clamp((data(r - 1, c - 1)[index] + data(r + 1, c + 1)[index]) / 2 + (tmp1) / 4, 0,
                                  UINT8_MAX);
    } else if (deltaN > deltaP) {
        data(r, c)[index] = clamp((data(r - 1, c + 1)[index] + data(r + 1, c - 1)[index]) / 2 + (tmp2) / 4, 0,
                                  UINT8_MAX);
    } else {
        data(r, c)[index] = clamp((data(r - 1, c - 1)[index] + data(r + 1, c + 1)[index] +
                                   data(r - 1, c + 1)[index] + data(r + 1, c - 1)[index]) / 4 +
                                  (tmp1 + tmp2) / 8, 0, UINT8_MAX);
    }
}

int main(int argc, char **argv) {


    auto start = steady_clock::now();

    if (argc != 3) {
        perror("Error: just 2 parameters");
        return EXIT_FAILURE;
    }

    Mat pgmImage = PGMHelper::parsePGM(argv[1]);

    Mat<uint8_t> data8 = Mat<uint8_t>(pgmImage.rows(), pgmImage.cols());

    quantize(pgmImage, data8);

    PGMHelper::dumpPGM(data8, argv[2], "_gray");

    Mat<array<uint8_t, 3>> bayer_pattern(data8.rows(), data8.cols());
    unique_ptr<array<uint8_t, 3>> zero = make_unique<array<uint8_t, 3>>(array < uint8_t, 3 > {0, 0, 0});
    bayer_pattern.setZero(zero.get());
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
    PGMHelper::dumpPPM(bayer_pattern, argv[2], "_bayer");

    for (int r = 0; r < bayer_pattern.rows(); ++r) {
        for (int c = 0; c < bayer_pattern.cols(); ++c) {
            if (r % 2 == 0) {
                if (c % 2 == 0) {
                    greenReconstruction(r, c, bayer_pattern, RED);
                }
            } else {
                if (c % 2 != 0) {
                    greenReconstruction(r, c, bayer_pattern, BLUE);
                }
            }
        }
    }
    PGMHelper::dumpPPM(bayer_pattern, argv[2], "_green");

    for (int r = 0; r < bayer_pattern.rows(); ++r) {
        for (int c = 0; c < bayer_pattern.cols(); ++c) {
            if (r % 2 == 0) {
                if (c % 2 == 1) {
                    avarages_horiz(r, c, bayer_pattern, RED);
                    avarages_vert(r, c, bayer_pattern, BLUE);
                } else {
                    redBluereconstruction(r, c, bayer_pattern, BLUE);
                }
            } else {
                if (c % 2 == 0) {
                    avarages_horiz(r, c, bayer_pattern, BLUE);
                    avarages_vert(r, c, bayer_pattern, RED);
                } else {
                    redBluereconstruction(r, c, bayer_pattern, RED);
                }
            }
        }
    }
    PGMHelper::dumpPPM(bayer_pattern, argv[2], "_interp");


    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to execute decompression was :" << elapsed_ms.count() << "ms" << endl;

    return EXIT_SUCCESS;
}
