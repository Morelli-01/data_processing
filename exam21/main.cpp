#include <iostream>
#include <fstream>
#include <map>
#include <cstdint>
#include <iterator>
#include "string"
#include "vector"
#include "cstdint"
#include "numeric"
#include "algorithm"

using namespace std;

template<typename T>
struct Mat {
    size_t rows_;
    size_t cols_;
    vector<T> data_;

    explicit Mat(size_t rows, size_t cols) : rows_(rows), cols_(cols), data_(vector<T>(rows * cols, 0)) {}

    T &operator()(size_t r, size_t c) {
        if (r >= rows_ or c >= cols_) {
            throw logic_error("err");
        }
        return data_[r * cols_ + c];
    }

    const T &operator()(size_t r, size_t c) const {
        if (r >= rows_ or c >= cols_) {
            throw logic_error("err");
        }
        return data_[r * cols_ + c];
    }

    size_t size() {
        return cols_ * rows_;
    }


};

struct PGMHelper {

    template<typename T>
    static Mat<T> loadPGM(istream &is) {
        string tmpStr;
        is >> tmpStr;
        if (tmpStr != "P5") {
            throw logic_error("err");
        }
        is.get();
        tmpStr.clear();
        map<string, size_t> header{};
        char tmpC;
        int counter = 0;
        while ((tmpC = is.get())) {
            if (tmpC == '#') {
                while (is.get() != '\n');
                continue;
            }
            if (tmpC == ' ' or tmpC == '\t' or tmpC == '\n') {
                if (counter == 0) {
                    header["width"] = stoi(tmpStr);
                } else if (counter == 1) {
                    header["height"] = stoi(tmpStr);
                } else if (counter == 2) {
                    header["maxval"] = stoi(tmpStr);
                    break;
                }
                counter++;
                tmpStr.clear();
            } else tmpStr += tmpC;
        }

        Mat<T> mat(header["height"], header["width"]);
        is.read(reinterpret_cast<char *>(mat.data_.data()), mat.size() * sizeof(T));
        return mat;
    }

    template<typename T>
    static void dumpPGM(ostream &os, Mat<T> &mat) {
        os << "P5 " << to_string(mat.cols_) << ' ' << mat.rows_ << ' ' << to_string(numeric_limits<T>().max()) << endl;
        os.write(reinterpret_cast<const char *>(mat.data_.data()), mat.size() * sizeof(T));
    }

};

template<typename T>
struct MLT {
    using SubPart = vector<vector<T>>;
    Mat<uint8_t> pattern_;

    explicit MLT() : pattern_(8, 8) {
        pattern_ = Mat<uint8_t>(8, 8);
        pattern_.data_ = {
                1, 6, 4, 6, 2, 6, 4, 6,
                7, 7, 7, 7, 7, 7, 7, 7,
                5, 6, 5, 6, 5, 6, 5, 6,
                7, 7, 7, 7, 7, 7, 7, 7,
                3, 6, 4, 6, 3, 6, 4, 6,
                7, 7, 7, 7, 7, 7, 7, 7,
                5, 6, 5, 6, 5, 6, 5, 6,
                7, 7, 7, 7, 7, 7, 7, 7
        };
    }

    SubPart subParts(Mat<T> &mat) {
        SubPart subParts(8);
        for (size_t r = 0; r < mat.rows_; ++r) {
            for (size_t c = 0; c < mat.cols_; ++c) {
                subParts[pattern_(r % 8, c % 8)].push_back(mat(r, c));
            }
        }
        return subParts;
    }

    static void dumpMLT(ostream &os, SubPart &subPart, size_t initHeight, size_t initWidth) {
        os << "MULTIRES";
        os.write(reinterpret_cast<const char *>(&initWidth), 4);
        os.write(reinterpret_cast<const char *>(&initHeight), 4);
        for (int i = 0; i < 8; ++i) {
            os.write(reinterpret_cast<const char *>(subPart[i].data()), subPart[i].size() * sizeof(T));

        }
    }

    pair<size_t, size_t> loadMLTinfo(istream &is) {
        string tmpStr;
        for (int i = 0; i < 8; ++i)tmpStr += is.get();

        if (tmpStr != "MULTIRES") {
            throw logic_error("err");
        }
        size_t width = 0;
        for (int i = 0; i < 4; ++i) {
            width = width | (is.get() << (8 * i));
        }
        size_t height = 0;
        for (int i = 0; i < 4; ++i) {
            height = height | (is.get() << (8 * i));
        }
        return {height, width};

    }

    void loadMLTlevel(istream &is, Mat<uint8_t> &mat, size_t level, string &prefix) {
        SubPart subPart(8);
        mat.data_.clear();
        if (level >= 1) {
            for (int r = 0; r < mat.rows_; r += 8) {
                for (int c = 0; c < mat.cols_; c += 8) {
                    mat(r, c) = is.get();
                }
            }

            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 8) * 8, (c / 8) * 8);
                }
            }
            ofstream os(prefix + "_1.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 2) {
            for (int r = 0; r < mat.rows_; r += 8) {
                for (int c = 4; c < mat.cols_; c += 8) {
                    mat(r, c) = is.get();
                }
            }
            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 8) * 8, (c / 4) * 4);
                }
            }
            ofstream os(prefix + "_2.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 3) {
            for (int r = 4; r < mat.rows_; r += 8) {
                for (int c = 0; c < mat.cols_; c += 4) {
                    mat(r, c) = is.get();
                }
            }
            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 4) * 4, (c / 4) * 4);
                }
            }
            ofstream os(prefix + "_3.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 4) {
            for (int r = 0; r < mat.rows_; r += 4) {
                for (int c = 2; c < mat.cols_; c += 4) {
                    mat(r, c) = is.get();
                }
            }
            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 4) * 4, (c / 2) * 2);
                }
            }
            ofstream os(prefix + "_4.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 5) {
            for (int r = 2; r < mat.rows_; r += 4) {
                for (int c = 0; c < mat.cols_; c += 2) {
                    mat(r, c) = is.get();
                }
            }
            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 2) * 2, (c / 2) * 2);
                }
            }
            ofstream os(prefix + "_5.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 6) {
            for (int r = 0; r < mat.rows_; r += 2) {
                for (int c = 1; c < mat.cols_; c += 2) {
                    mat(r, c) = is.get();
                }
            }
            for (int r = 0; r < mat.rows_; ++r) {
                for (int c = 0; c < mat.cols_; ++c) {
                    mat(r, c) = mat((r / 2) * 2, c);
                }
            }
            ofstream os(prefix + "_6.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }
        if (level >= 7) {
            for (int r = 1; r < mat.rows_; r += 2) {
                for (int c = 0; c < mat.cols_; c += 1) {
                    mat(r, c) = is.get();
                }
            }
            ofstream os(prefix + "_7.pgm", ios::binary);
            PGMHelper::dumpPGM(os, mat);
        }

    }
};

int main(int argc, char **argv) {
    if (argc != 4) {
        perror("Error\n");
        return EXIT_FAILURE;
    }


    string mode{argv[1]};
    if (mode == "c") {
        try {
            ifstream is(argv[2], ios::binary);
            ofstream os(argv[3], ios::binary);
            if (os.fail() or is.fail()) {
                return EXIT_FAILURE;
            }

            auto data = PGMHelper::loadPGM<uint8_t>(is);
            MLT<uint8_t> mlt{};
            vector<vector<uint8_t>> subparts = mlt.subParts(data);
            MLT<uint8_t>::dumpMLT(os, subparts, data.rows_, data.cols_);
            return EXIT_SUCCESS;
        } catch (logic_error &e) {
            return EXIT_FAILURE;
        }
    } else if (mode == "d") {
        try {
            ifstream is(argv[2], ios::binary);
            string prefix{argv[3]};
            if (is.fail()) {
                return EXIT_FAILURE;
            }

            MLT<uint8_t> mlt{};
            auto [h, w] = mlt.loadMLTinfo(is);
            Mat<uint8_t> data(h, w);

            mlt.loadMLTlevel(is, data, 7, prefix);

            return EXIT_SUCCESS;
        } catch (logic_error &e) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_FAILURE;
}
