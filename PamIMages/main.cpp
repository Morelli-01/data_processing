#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <variant>

#define GREYSCALE 0
#define RGB 1

using std::cout;
using std::endl;

template<typename T>
class Mat {
    size_t cols_;
    size_t rows_;
    std::vector<T> data_;
public:

    Mat(size_t row, size_t col) : cols_(col), rows_(row) {
        data_ = std::vector<T>(rows_ * cols_);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols_ + c];
    }

    const T &operator()(size_t r, size_t c) const {
        return data_[r * cols_ + c];
    }

    T &rowdata(size_t index) {
        return data_[index];
    }

    const T &rowdata(size_t index) const {
        return data_[index];
    }

    size_t cols() const {
        return cols_;
    }

    size_t rows() const {
        return rows_;
    }

    int size() const {
        return cols_ * rows_;
    }

};

class PamHelper {

public:
    static std::variant<std::pair<std::map<std::string, int>, Mat<uint8_t>>,
            std::pair<std::map<std::string, int>, std::array<Mat<uint8_t>, 3> >,
            void const *>
    parsePam(const std::string &path) {
        std::ifstream is(path, std::ios::binary);
        if (!is.good()) {
            perror("Problem encountered while tring to open the input file");
            return nullptr;
        }
        std::string tmp = std::string{(char) is.get()};
        tmp += std::string{(char) is.get()};
        if (tmp != "P7") {
            perror("The file you're trying to open is not a .pam file\n");
            return nullptr;
        }
        std::map<std::string, int> header_;

        header_.insert({"WIDTH", -1});
        header_.insert({"HEIGHT", -1});
        header_.insert({"DEPTH", -1});
        header_.insert({"MAXVAL", -1});
        header_.insert({"TUPLTYPE", -1});
        header_.insert({"ENDHDR", -1});
        tmp = "";
        std::string h{};
        std::string v{};
        while (header_["ENDHDR"] == -1) {
            tmp = is.get();
            if (tmp == "\n") {
                continue;
            }
            h += tmp;

            if (header_.contains(h)) {
                if (h == "ENDHDR") {
                    header_["ENDHDR"] = 0;
                    continue;
                }
                is.get();

                if (h == "TUPLTYPE") {
                    while ((tmp = is.get()) != "\n") v += tmp;
                    if (v == "GRAYSCALE") header_[h] = GREYSCALE;
                    else header_[h] = RGB;
                } else {
                    while ((tmp = is.get()) != "\n") v += tmp;

                    header_[h] = atoi(v.c_str());
                }


                v = "";
                h = "";
            }
        }
        is.get();

        if (header_["TUPLTYPE"]) {
            Mat mat_R = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
            Mat mat_G = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
            Mat mat_B = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
            std::array<Mat<uint8_t>, 3> data = {mat_R, mat_G, mat_B};
            for (int r = 0; r < header_["HEIGHT"]; ++r) {
                for (int c = 0; c < header_["WIDTH"]; ++c) {
                    for (int i = 0; i <3; ++i) {
                        data[i](r, c) = is.get();

                    }
                }
            }

            return std::pair{header_, data};
        } else {
            Mat data = Mat<uint8_t>(header_["HEIGHT"], header_["WIDTH"]);
            uint8_t d;
            for (int r = 0; r < data.rows(); ++r) {
                for (int c = 0; c < data.cols(); ++c) {
                    data(r, c) = is.get();
                }
            }
            return std::pair{header_, data};
        }

    }

    static void
    dumpPam(std::map<std::string, int> &header, std::variant<Mat<uint8_t>, std::array<Mat<uint8_t>, 3>> variant,
            std::ostream &os) {
        os.write("P7", 2);
        os.put('\n');
        os << "WIDTH " << header["WIDTH"] << endl;
        os << "HEIGHT " << header["HEIGHT"] << endl;
        os << "DEPTH " << header["DEPTH"] << endl;
        os << "MAXVAL " << header["MAXVAL"] << endl;
        os << "TUPLTYPE " << (header["TULPTYPE"] == 0 ? "GRAYSCALE" : "RGB") << endl;
        os << "ENDHDR" << endl;
        if (std::holds_alternative<Mat<uint8_t>>(variant)) {
            auto data = std::get<Mat<uint8_t>>(variant);
            for (int r = 0; r < data.rows(); ++r) {
                for (int c = 0; c < data.cols(); ++c) {
                    os.put(data(r, c));
                }
            }
        } else if (std::holds_alternative<std::array<Mat<uint8_t>, 3>>(variant)){
            auto data = std::get<std::array<Mat<uint8_t>, 3>>(variant);
            for (int r = 0; r < header["HEIGHT"]; ++r) {
                for (int c = 0; c < header["WIDTH"]; ++c) {
                    for (int i = 0; i <3; ++i) {
                        os.put(data[i](r, c));
                    }
                }
            }
        }
    }

    static std::map<std::string, int> getStdHeader(int w = -1, int h = -1, int d = -1, int max = -1, int type = -1) {
        std::map<std::string, int> header_;

        header_.insert({"WIDTH", w});
        header_.insert({"HEIGHT", h});
        header_.insert({"DEPTH", d});
        header_.insert({"MAXVAL", max});
        header_.insert({"TUPLTYPE", type});
        header_.insert({"ENDHDR", -1});
        return header_;
    }
};

using Result = std::pair<std::map<std::string, int>, Mat<uint8_t>>;

void esercizio_1() {
    std::map<std::string, int> header = PamHelper::getStdHeader(256, 256, 1, 255, GREYSCALE);
    Mat mat = Mat<uint8_t>(header["HEIGHT"], header["WIDTH"]);
    for (int r = 0; r < mat.rows(); ++r) {
        for (int c = 0; c < mat.cols(); ++c) {
            mat(r, c) = r;
        }
    }
    std::ofstream os("/home/nicola/Desktop/data_processing/PamIMages/es1.pam", std::ios::binary);
    PamHelper::dumpPam(header, mat, os);
}

void esercizio_2() {
    auto pamImage = PamHelper::parsePam("/home/nicola/Desktop/data_processing/PamIMages/frog.pam");
    if (std::holds_alternative<std::pair<std::map<std::string, int>, Mat<uint8_t>>>(pamImage)) {
        auto [header, data] = std::get<Result>(pamImage);

        Mat flipped_data = Mat<uint8_t>(header["HEIGHT"], header["WIDTH"]);
        for (int r = 0; r < data.rows(); ++r) {
            for (int c = 0; c < data.cols(); ++c) {
                flipped_data(r, c) = data(header["HEIGHT"] - r - 1, c);
            }
        }
        std::ofstream os("/home/nicola/Desktop/data_processing/PamIMages/es2.pam", std::ios::binary);
        PamHelper::dumpPam(header, flipped_data, os);

    }
}


int main() {
    auto pamImage = PamHelper::parsePam("/home/nicola/Desktop/data_processing/PamIMages/laptop.pam");
    if (std::holds_alternative<std::pair<std::map<std::string, int>, std::array<Mat<uint8_t>, 3>>>(pamImage)) {
        auto [header, data] = std::get<std::pair<std::map<std::string, int>, std::array<Mat<uint8_t>, 3>>>(pamImage);
        std::ofstream os("/home/nicola/Desktop/data_processing/PamIMages/es3.pam", std::ios::binary);
        PamHelper::dumpPam(header, data, os);

    }


//        auto [header, data] = std::get<Result>(pamImage);
//
//        Mat flipped_data = Mat<uint8_t>(header["HEIGHT"], header["WIDTH"]);
//        for (int r = 0; r < data.rows(); ++r) {
//            for (int c = 0; c < data.cols(); ++c) {
//                flipped_data(r, c) = data(r, header["WIDTH"] - c - 1);
//            }
//        }
//        std::ofstream os("/home/nicola/Desktop/data_processing/PamIMages/es3.pam", std::ios::binary);
//        PamHelper::dumpPam(header, flipped_data, os);
//
//    }
    esercizio_2();

    return EXIT_SUCCESS;
}
