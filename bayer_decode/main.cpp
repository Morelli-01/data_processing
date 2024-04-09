#include <iostream>
#include <fstream>
#include <vector>
#include "cstdlib"
#include <cinttypes>
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
        ifstream is(path);
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
        is.read(reinterpret_cast<char *>(data.data()), height * width * sizeof(uint16_t));
        std::for_each(data.dataVector().begin(), data.dataVector().end(), [](uint16_t &x) {
            x = std::byteswap(x);
        });
        return data;
    }

    static auto dumpPGM(Mat<uint8_t> &data, const string &path) {
        ofstream os(path, std::ios::binary);
        if (os.fail()) {
            perror("Error while opening ouput file");
            return EXIT_FAILURE;
        }
        os << "P5\n";
        os << to_string(data.cols()) << ' ';
        os << to_string(data.rows()) << ' ';
        os << "255\n";
//        os.write(reinterpret_cast<const char *>(data.dataVector().data()), data.rows() * data.cols());
        std::for_each(data.dataVector().begin(), data.dataVector().end(), [&](uint8_t &x) {
            os.put(static_cast<char>(x));
        });

    }
};

Mat<uint8_t>& quantize(Mat<uint16_t> data16) {
    Mat<uint8_t> data8 = Mat<uint8_t>(data16.rows(), data16.cols());
    for (int r = 0; r < data8.rows(); ++r) {
        for (int c = 0; c < data8.cols(); ++c) {
            data8(r, c) = static_cast<uint8_t >(data16(r, c) / 256);
        }
    }
    return (Mat<uint8_t> &) std::move(data8);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("Error: just 2 parameters");
        return EXIT_FAILURE;
    }
    Mat pgmImage = PGMHelper::parsePGM(argv[1]);
//    Mat<uint8_t> pgmImageQtz = quantize(pgmImage);
    Mat<uint8_t > data8 = quantize(pgmImage);
    cout << data8.dataVector().size() << endl;
    PGMHelper::dumpPGM(data8, argv[2]);
    cout << endl;
    return EXIT_SUCCESS;
}
