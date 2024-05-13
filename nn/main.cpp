#include <iostream>
#include "cstdint"
#include "cstdlib"
#include "vector"

using namespace std;

template<typename T>
struct Mat {
    size_t rows;
    size_t cols;
    vector<T> data_;

    Mat(size_t rows, size_t cols) : rows(rows), cols(cols) {
        data_ = vector<T>(rows * cols);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols + c];
    }

    const T &operator()(size_t r, size_t c) const {
        return data_[r * cols + c];
    }

    size_t size() { return cols * rows; }

    Mat<T> operator*(Mat<T> &v) {
        Mat<T> out(0, 0);
        if (v.cols != rows) {
            cout << "can't do the mm on mat with different sizes";
            return out;
        }
        out = Mat<T>(v.rows, cols);
        for (int r = 0; r < v.rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                for (int r1 = 0; r1 < rows; r1++) {
                    out(r, c) += v(r, c) * data_[r1 * cols + c];
                }
            }
        }
        return out;
    }
};

struct Neuron {
    size_t in;
    size_t out;
    Mat<double> parameters;

    Neuron(size_t in, size_t out) : in(in), out(out), parameters(in, out) {}

    Mat<double> mm(Mat<double> &inData) {
        Mat<double> mmProd(0, 0);
        if (inData.size() != in) {
            cout << "Error: input size is incompatible\n";
            return mmProd;
        }

        return parameters * inData;

    }
};

int main() {
    Neuron n1(2, 3);
    for (auto &item: n1.parameters.data_) {
        item = 1;
    }

    Mat<double> m1(1, 2);
    for (auto &item: m1.data_) {
        item = 1;
    }
    auto matMul = n1.mm(m1);
    cout << endl;
    return EXIT_SUCCESS;
}
