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
        if (cols != v.rows) {
            cout << "can't do the mm on mat with different sizes";
            return out;
        }
        out = Mat<T>(rows, v.cols);
        for (int r1 = 0; r1 < rows; ++r1) {
            for (int c1 = 0; c1 < cols; ++c1) {
                for (int c2 = 0; c2 < v.cols; ++c2) {
                    for (int r2 = 0; r2 < v.rows; ++r2) {
                        out(r1, c2) += data_[r1 * cols + c1] + v(r2, c2);

                    }
                }
            }

        }
        return out;
    }

    Mat<T> dot(Mat<T> &v) {
        Mat<T> out(0, 0);
        if (cols != v.cols or rows != v.rows) {
            cout << "Error: matrices had different sizes!\n";
            return out;
        }
        out = Mat<T>(rows, cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                out(r, c) = data_[r * cols + c] * v(r, c);

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
        if (inData.cols != in) {
            cout << "Error: input size is incompatible\n";
            return mmProd;
        }
        return inData * parameters;
    }
};

int main() {
    Neuron n1(2, 3);
    for (auto &item: n1.parameters.data_) {
        item = 1;
    }

    Mat<double> m1(5, 2);
    for (auto &item: m1.data_) {
        item = 1;
    }
    auto matMul = n1.mm(m1);
    cout << endl;
    return EXIT_SUCCESS;
}
