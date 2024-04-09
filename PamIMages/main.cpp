//
// Created by nicola on 05/04/2024.
//
#include <cstdlib>
#include <string>
#include <variant>
#include <fstream>
#include <map>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include "iostream"
#include "chrono"

using std::cout;
using std::endl;
#define GRAYSCALE 0
#define RGB 1
using namespace std;
using namespace std::chrono;

template<typename T>
class Mat {
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
    std::function<size_t(size_t, size_t)> getIndex;
public:
    Mat(size_t rows_, size_t cols_) : rows_(rows_), cols_(cols_) {
        data_ = std::vector<T>(rows_ * cols_);
        getIndex = [&](size_t r, size_t c) -> size_t {
            return r * this->cols_ + c;
        };
    }

    T &operator()(size_t r, size_t c) {
        return data_[getIndex(r, c)];
    }

    T &operator()(size_t r, size_t c) const {
        return data_[getIndex(r, c)];
    }

    size_t rows() const {
        return rows_;
    }

    size_t cols() const {
        return cols_;
    }

    const std::vector<T> &data() const {
        return data_;
    }

    std::vector<T> &data() {
        return data_;
    }

    size_t size() const {
        return data_.size();
    }


    void setGetIndex(std::function<size_t(size_t, size_t)> f) {
        getIndex = std::move(f);
    }

};

class PamHelper {

public:
    static std::map<std::string, int> parseHeader(std::istream &is) {
        string file_type{};
        file_type += static_cast<char>(is.get());
        file_type += static_cast<char>(is.get());
        file_type += static_cast<char>(is.get());

        if (file_type != "P7\n") {
            perror("The file passed as input is not a .pam file!!!");
            exit(-1);
        }
        char tmp;
        string tmpStr{};
        string field{};
        map<string, int> header_ = getStdHeader();

        while (header_["ENDHDR"] == -1) {
            tmp = is.get();
            switch (tmp) {
                case '#':
                    while (is.get() != '\n');
                    break;
                case ' ':
                    field = tmpStr;
                    tmpStr.clear();
                    break;
                case '\n':
                    if (tmpStr == "ENDHDR") header_[tmpStr] = 0;
                    else if (field == "TUPLTYPE") header_[field] = (tmpStr == "RGB") ? RGB : GRAYSCALE;
                    else header_[field] = atoi(tmpStr.c_str());
                    tmpStr.clear();
                    break;
                default:
                    tmpStr += tmp;
            }
        }
        return std::move(header_);
    }

    static vector<unique_ptr<Mat<uint8_t >>> parsePam(const std::string &path) {
        std::ifstream is(path, std::ios::binary);
        if (is.fail()) {
            perror("Error encountered while opening input file");
            exit(-1);
        }
        map<string, int> header_ = parseHeader(is);
        vector<unique_ptr<Mat<uint8_t>>> vec{};

        if (header_["TUPLTYPE"] == RGB) {
            vec.push_back(make_unique<Mat<uint8_t>>(header_["HEIGHT"], header_["WIDTH"]));
            vec.push_back(make_unique<Mat<uint8_t>>(header_["HEIGHT"], header_["WIDTH"]));
            vec.push_back(make_unique<Mat<uint8_t>>(header_["HEIGHT"], header_["WIDTH"]));
            for (int r = 0; r < header_["HEIGHT"]; ++r) {
                for (int c = 0; c < header_["WIDTH"]; ++c) {
                    for (int i = 0; i < 3; ++i) {
                        (*vec[i])(r, c) = is.get();
                    }
                }

            }
        } else {
            unique_ptr<Mat<uint8_t>> data = make_unique<Mat<uint8_t>>(header_["HEIGHT"], header_["WIDTH"]);
            for (int r = 0; r < data->rows(); ++r) {
                for (int c = 0; c < data->cols(); ++c) {
                    (*data)(r, c) = is.get();
                }

            }
            vec.push_back(std::move(data));

        }
        return std::move(vec);


    }

    static std::map<std::string, int> getStdHeader(int w = -1, int h = -1, int d = -1, int max = -1, int type = -1) {
        map<std::string, int> header_;

        header_.insert({"WIDTH", w});
        header_.insert({"HEIGHT", h});
        header_.insert({"DEPTH", d});
        header_.insert({"MAXVAL", max});
        header_.insert({"TUPLTYPE", type});
        header_.insert({"ENDHDR", -1});
        return std::move(header_);
    }

    static void dumpHeader(std::map<std::string, int> &header, std::ostream &os) {
        os.write("P7", 2);
        os.put('\n');
        os << "WIDTH " << header["WIDTH"] << endl;
        os << "HEIGHT " << header["HEIGHT"] << endl;
        os << "DEPTH " << header["DEPTH"] << endl;
        os << "MAXVAL " << header["MAXVAL"] << endl;
        os << "TUPLTYPE " << (header["TUPLTYPE"] == 0 ? "GRAYSCALE" : "RGB") << endl;
        os << "ENDHDR" << endl;
    }

    static void dumpPam(const vector<unique_ptr<Mat<uint8_t >>> &vec, const std::string &path) {
        std::ofstream os(path, std::ios::binary);
        if (os.fail()) {
            perror("Error encountered while opening output file");
            exit(-1);
        }

//        auto &mat_R = *(vec[0].get());
        map<string, int> header_ = getStdHeader(vec[0]->cols(), vec[0]->rows(), vec.size(), 255,
                                                vec.size() == 1 ? GRAYSCALE : RGB);
        dumpHeader(header_, os);
        if (header_["TUPLTYPE"]) {

            for (int r = 0; r < vec[0]->rows(); ++r) {
                for (int c = 0; c < vec[0]->cols(); ++c) {
                    for (int i = 0; i < 3; ++i) {
                        os.put((*vec[i])(r, c));
//                        os.put(vec[i].get()->(r, c));
                    }

                }
            }
        } else {
            for (int r = 0; r < vec[0]->rows(); ++r) {
                for (int c = 0; c < vec[0]->cols(); ++c) {
                    os.put((*vec[0])(r, c));
                }
            }
        }
        cout << endl;
    }
};

void esercizio_1() {
    Mat<uint8_t> data(256, 256);

    int i = 0;
    std::for_each(data.data().begin(), data.data().end(), [&](uint8_t &x) {
        x = i / 256;
        i++;
    });

    vector<unique_ptr<Mat<uint8_t >>> vec;
    vec.push_back(make_unique<Mat<uint8_t>>(data));

    PamHelper::dumpPam(vec, std::string{"C:\\Users\\nicol\\Desktop\\data_processing\\PamIMages\\es2.pam"});
}

void esercizio_2() {
    vector<unique_ptr<Mat<uint8_t >>> vec = PamHelper::parsePam(
            std::string{"C:\\Users\\nicol\\Desktop\\data_processing\\PamIMages\\frog.pam"});

    std::for_each(vec.begin(), vec.end(), [](unique_ptr<Mat<uint8_t >> &v) {
        v->setGetIndex([&](size_t r, size_t c) -> size_t {
            return (v->rows() - r - 1) * v->cols() + c;
        });
    });

    PamHelper::dumpPam(vec, std::string{"C:\\Users\\nicol\\Desktop\\data_processing\\PamIMages\\es2.pam"});
}

void esercizio_3() {
    vector<unique_ptr<Mat<uint8_t >>> vec = PamHelper::parsePam(
            std::string{"C:\\Users\\nicol\\Desktop\\data_processing\\PamIMages\\laptop.pam"});

    std::for_each(vec.begin(), vec.end(), [](unique_ptr<Mat<uint8_t >> &v) {
        v->setGetIndex([&](size_t r, size_t c) -> size_t {
            return r * v->cols() + v->cols() - c - 1;
        });
    });

    PamHelper::dumpPam(vec, std::string{"C:\\Users\\nicol\\Desktop\\data_processing\\PamIMages\\es3.pam"});
}

int main() {
    auto start = steady_clock::now();


    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to execute decompression was :" << elapsed_ms.count() << "ms" << endl;

    return EXIT_SUCCESS;
}
