#include <iostream>
#include <fstream>
#include <vector>
#include "cstdlib"
#include <cinttypes>
#include <array>
#include "algorithm"

using namespace std;
#define RED 0
#define GREEN 1
#define BLUE 2

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

    const T &operator()(size_t r, size_t c) const {
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

    ~Mat() {
        cout << "MAT DECONSTRUCTOR HAS BEEN CALLED!!!" << endl;
    }


};

uint8_t &protected_data(int r, int c, Mat<array<uint8_t, 3>> &data, size_t index) {
    if (r < 0 or c < 0 or r >= data.rows() or c >= data.cols()) {
        uint8_t *zero = new uint8_t;
        *zero = 0;
        return *zero;
    } else {
        return data(r, c)[index];
    }

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

    static auto dumpPPM(Mat<array<uint8_t, 3>> &data, const string &path) {
        ofstream os(path + ".ppm", std::ios::binary);
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
            data8(r, c) = static_cast<uint8_t >(data16(r, c) / 256);
        }
    }
    return data8;
}

auto greenReconstruction(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {

//    if (r < 2 or c < 2 or r >= data.rows() - 2 or c >= data.cols() - 2) {
//        return 0;
//    }

    auto tmp1 = protected_data(r, c, data, index) * 2 - protected_data(r, c - 2, data, index) -
                protected_data(r, c + 2, data, index);
    auto tmp2 = protected_data(r, c, data, index) * 2 - protected_data(r - 2, c, data, index) -
                protected_data(r + 2, c, data, index);

    auto deltaH = std::abs(protected_data(r, c - 1, data, 1) - protected_data(r, c + 1, data, 1)) +
                  std::abs(tmp1);
    auto deltaV = std::abs(protected_data(r - 1, c, data, 1) - protected_data(r + 1, c, data, 1)) +
                  std::abs(tmp2);

    if (deltaH < deltaV) {
        return (protected_data(r, c - 1, data, 1) + protected_data(r, c + 1, data, 1)) / 2 + tmp1 / 4;
    } else if (deltaH > deltaV) {
        return (protected_data(r - 1, c, data, 1) + protected_data(r + 1, c, data, 1)) / 2 + tmp2 / 4;
    } else {
        return (protected_data(r - 1, c, data, 1) + protected_data(r + 1, c, data, 1) +
                protected_data(r, c - 1, data, 1) + protected_data(r, c + 1, data, 1)) / 4 +
               (4 * protected_data(r, c, data, index) - protected_data(r, c - 2, data, index) -
                protected_data(r, c + 2, data, index) -
                protected_data(r - 2, c, data, index) -
                protected_data(r + 2, c, data, index)) / 8;
    }

}

void avarages_vert(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {
    data(r, c)[index] = (protected_data(r - 1, c, data, index) + protected_data(r + 1, c, data, index)) / 2;

}

void avarages_horiz(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {
    data(r, c)[index] = (protected_data(r, c - 1, data, index) + protected_data(r, c - 1, data, index)) / 2;
}

void redBluereconstruction(size_t r, size_t c, Mat<array<uint8_t, 3>> &data, size_t index) {
//    int index = (index == RED) ? BLUE : RED;
    auto tmp1 = protected_data(r, c, data, GREEN) * 2 - protected_data(r - 1, c - 1, data, GREEN) -
                protected_data(r + 1, c + 1, data, GREEN);
    auto tmp2 = protected_data(r, c, data, GREEN) * 2 - protected_data(r - 1, c + 1, data, GREEN) -
                protected_data(r + 1, c - 1, data, GREEN);


    auto deltaN = abs(protected_data(r - 1, c - 1, data, index) - protected_data(r + 1, c + 1, data, index)) +
                  abs(tmp1);
    auto deltaP = abs(protected_data(r - 1, c + 1, data, index) - protected_data(r + 1, c - 1, data, index)) +
                  abs(tmp2);


    if (deltaN < deltaP) {
        data(r, c)[index] =
                (protected_data(r - 1, c - 1, data, index) + protected_data(r + 1, c + 1, data, index)) / 2 +
                tmp1 / 4;
    } else if (deltaN > deltaP) {
        data(r, c)[index] =
                (protected_data(r - 1, c + 1, data, index) + protected_data(r + 1, c - 1, data, index)) / 2 +
                tmp2 / 4;
    } else {
        data(r, c)[index] =
                (protected_data(r - 1, c - 1, data, index) + protected_data(r + 1, c + 1, data, index) +
                 protected_data(r - 1, c + 1, data, index) + protected_data(r + 1, c - 1, data, index)) / 4 +
                (tmp1 + tmp2) / 8;

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

    for (int r = 0; r < bayer_pattern.rows(); ++r) {
        for (int c = 0; c < bayer_pattern.cols(); ++c) {
            if (r % 2 == 0) {
                if (c % 2 == 0) {
                    bayer_pattern(r, c)[1] = greenReconstruction(r, c, bayer_pattern, RED);
                }
            } else {
                if (c % 2 != 0) {
                    bayer_pattern(r, c)[1] = greenReconstruction(r, c, bayer_pattern, BLUE);
                }
            }
        }
    }

    PGMHelper::dumpPPM(bayer_pattern, "/home/nicola/Desktop/data_processing/bayer_decode/small_green");

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
    auto tmp = (protected_data(10, 49, bayer_pattern, GREEN));

    PGMHelper::dumpPPM(bayer_pattern, "/home/nicola/Desktop/data_processing/bayer_decode/small_color");


    return EXIT_SUCCESS;
}
