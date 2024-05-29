#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
#include "array"
#include "string"
#include "chrono"
#include "cmath"
#include "utility"

using namespace std;
using namespace std::chrono;
using Pixel = array<uint8_t, 3>;


struct Box {
    Pixel v1{};
    Pixel v2{};
    array<vector<uint8_t>, 3> histogram_{};

    explicit Box(vector<Pixel> &pixels) {
        std::for_each(pixels.begin(), pixels.end(), [&](Pixel &p) {
            histogram_[0].emplace_back(p[0]);
            histogram_[1].emplace_back(p[1]);
            histogram_[2].emplace_back(p[2]);
        });


        uint8_t r_min = 0, g_min = 0, b_min = 0;
        uint8_t r_max = 255, g_max = 255, b_max = 255;
        for (int i = 0; i < 256; ++i) {
            if (histogram_[0][i] != 0) {
                r_min = i;
                break;
            }
        }
        for (int i = 0; i < 256; ++i) {
            if (histogram_[1][i] != 0) {
                g_min = i;
                break;
            }
        }
        for (int i = 0; i < 256; ++i) {
            if (histogram_[2][i] != 0) {
                b_min = i;
                break;
            }
        }

        for (int i = 255; i >= 0; --i) {
            if (histogram_[0][i] != 0) {
                r_max = i;
                break;
            }
        }
        for (int i = 255; i >= 0; --i) {
            if (histogram_[1][i] != 0) {
                g_max = i;
                break;
            }
        }
        for (int i = 255; i >= 0; --i) {
            if (histogram_[2][i] != 0) {
                g_max = i;
                break;
            }
        }

        v1 = Pixel{r_min, g_min, b_min};
        v2 = Pixel{r_max, g_max, b_max};

    }

    explicit Box(Pixel v1, Pixel v2) : v1(v1), v2(v2) {}

    size_t rangeR() {
        return (v2[0] - v1[0]) + 1;
    }

    size_t rangeG() {
        return (v2[1] - v1[1]) + 1;
    }

    size_t rangeB() {
        return (v2[2] - v1[2]) + 1;
    }

    pair<size_t, size_t> getMaxRange() {
        if (rangeB() >= rangeG() and rangeB() >= rangeR()) {
            return {2, rangeB()};
        } else if (rangeG() >= rangeR()) {
            return {1, rangeG()};
        }
        return {0, rangeR()};
    }

    Pixel getMean() {
        return Pixel{static_cast<unsigned char>((v2[0] - v1[0]) / 2),
                     static_cast<unsigned char>((v2[1] - v1[1]) / 2),
                     static_cast<unsigned char>((v2[2] - v1[2]) / 2)};
    }

    Pixel getMean2() {
        array<size_t, 3> meanPixel{0, 0, 0};
        size_t n = histogram_[0].size();
        for (int i = 0; i < 3; ++i) {
            std::for_each(histogram_[i].begin(), histogram_[i].end(), [&](uint8_t &v) {
                meanPixel[i] += v;
            });
        }

        return Pixel{static_cast<unsigned char>(meanPixel[0] / n),
                     static_cast<unsigned char>(meanPixel[1] / n),
                     static_cast<unsigned char>(meanPixel[2] / n)};
    }

    pair<unique_ptr<Box>, unique_ptr<Box>> splitBoxOnMedian() {
        auto [index, range] = getMaxRange();
        size_t splitIndex = 0;
        std::sort(histogram_[index].begin(), histogram_[index].end());
        splitIndex = histogram_[index][histogram_[index].size() / 2];

        Box B1 = Box(v1, v2);
        Box B2 = Box(v1, v2);
        B1.v2[index] = splitIndex;
        B2.v1[index] = splitIndex;

        return {make_unique<Box>(B1), make_unique<Box>(B2)};

    }

    void histogram(vector<Pixel> &pixels) {
        std::for_each(pixels.begin(), pixels.end(), [&](Pixel &p) {
            if (p[0] >= v1[0] and p[0] <= v2[0] and
                p[1] >= v1[1] and p[1] <= v2[1] and
                p[2] >= v1[2] and p[2] <= v2[2]) {
                histogram_[0].emplace_back(p[0]);
                histogram_[1].emplace_back(p[1]);
                histogram_[2].emplace_back(p[2]);
            }
        });
    }

};

template<typename T>
class Mat {
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
public:
    Mat(size_t r, size_t c) : rows_(r), cols_(c) {
        data_ = vector<T>(rows_ * cols_);
    }

    array<uint8_t, 3> &operator()(size_t r, size_t c) {
        return data_.at(r * cols_ + c);
    }

    const array<uint8_t, 3> &operator()(size_t r, size_t c) const {
        return data_.at(r * cols_ + c);
    }

    size_t size() const {
        return rows_ * cols_;
    }

    vector<T> &data() {
        return data_;
    }

    size_t rows() const {
        return rows_;
    }

    size_t cols() const {
        return cols_;
    }

    static Mat<Pixel> readFromBytes(istream &is) {

        string tmp;
        is >> tmp;
        map<string, size_t> header_{};

        while (tmp != "ENDHDR") {
            if (tmp == "P7") {
                is >> tmp;
                continue;
            }
            auto h = tmp;
            is >> tmp;

            header_[h] = atoi(tmp.c_str());

            is >> tmp;
        }
        is.get();

        Mat mat = Mat<Pixel>(header_["HEIGHT"], header_["WIDTH"]);

        is.read(reinterpret_cast<char *>(mat.data().data()), mat.size() * 3);

        return mat;
    }

    void dumpPam(ostream &os) {
        os.write(string{"P7"}.c_str(), 2);
        os.put('\n');

        os.write(string{"WIDTH "}.c_str(), 6);
        os.write(to_string(cols_).c_str(), to_string(cols_).size());
        os.put('\n');

        os.write(string{"HEIGHT "}.c_str(), 7);
        os.write(to_string(rows_).c_str(), to_string(rows_).size());
        os.put('\n');

        os.write(string{"DEPTH "}.c_str(), 6);
        os.put('3');
        os.put('\n');

        os.write(string{"MAXVAL "}.c_str(), 7);
        os.write(string{"255"}.c_str(), 3);
        os.put('\n');

        os.write(string{"TUPLTYPE "}.c_str(), 9);
        os.write(string{"RGB"}.c_str(), 3);
        os.put('\n');

        os.write(string{"ENDHDR"}.c_str(), 6);
        os.put('\n');

        os.write(reinterpret_cast<const char *>(data_.data()), size() * 3);

    }

    static void dumpPamPalette(ostream &os, vector<Pixel> &palette) {
        os.write(string{"P7"}.c_str(), 2);
        os.put('\n');

        os.write(string{"WIDTH "}.c_str(), 6);
        os.write(string{"10"}.c_str(), 2);
        os.put('\n');

        os.write(string{"HEIGHT "}.c_str(), 7);
        os.write(to_string(palette.size() * 5).c_str(), to_string(palette.size() * 5).size());
        os.put('\n');

        os.write(string{"DEPTH "}.c_str(), 6);
        os.put('3');
        os.put('\n');

        os.write(string{"MAXVAL "}.c_str(), 7);
        os.write(string{"255"}.c_str(), 3);
        os.put('\n');

        os.write(string{"TUPLTYPE "}.c_str(), 9);
        os.write(string{"RGB"}.c_str(), 3);
        os.put('\n');

        os.write(string{"ENDHDR"}.c_str(), 6);
        os.put('\n');

        for (int i = 0; i < palette.size(); ++i) {
            for (int j = 0; j < 10; ++j) {
                for (int k = 0; k < 5; ++k) {
                    os.put(palette[i][0]);
                    os.put(palette[i][1]);
                    os.put(palette[i][2]);
                }

            }
        }

    }
};

struct MedianCut {

    istream &is_;
    ostream &os_;
    size_t N_;

    explicit MedianCut(istream &is, ostream &os, size_t N = 10) : is_(is), os_(os), N_(N) {}

    void algoritm() {
        Mat mat = Mat<Pixel>::readFromBytes(is_);

//        vector<Pixel> palette = getOptimalPalette(N_, mat.data());
        vector<Pixel> palette = getPaletteKMeans(N_, mat.data());
//        vector<Pixel> palette = getPalette(N_);
        ofstream os2(R"(/home/nicola/Desktop/data_processing/mcut/palette.pam)", ios::binary | ios::trunc);
        Mat<uint8_t>::dumpPamPalette(os2, palette);

        Mat qtzMat = Mat<Pixel>(mat.rows(), mat.cols());

        for (int r = 0; r < mat.rows(); ++r) {
            for (int c = 0; c < mat.cols(); ++c) {

                auto pixel = mat(r, c);
                vector<float> dst(palette.size());
                for (int i = 0; i < palette.size(); ++i) {
                    float dstTmp = 0;
                    dstTmp = abs(pixel[0] - palette[i][0]) + abs(pixel[1] - palette[i][1]) +
                             abs(pixel[2] - palette[i][2]);
                    dst[i] = dstTmp;
                }

                qtzMat(r, c) = palette[std::distance(dst.begin(), std::min_element(dst.begin(), dst.end()))];

            }
        }

        qtzMat.dumpPam(os_);

    }

    static vector<Pixel> getPalette(size_t N) {

        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<short> distrib(0, 255);
        vector<Pixel> palette{};

        for (int i = 0; i < N; ++i) {
            palette.emplace_back(
                    Pixel{static_cast<unsigned char>(distrib(gen)), static_cast<unsigned char>(distrib(gen)),
                          static_cast<unsigned char>(distrib(gen))});
        }

        return palette;
    }

    static vector<Pixel> getOptimalPalette(size_t N, vector<Pixel> &initialBucket) {
        vector<unique_ptr<Box>> boxes{};
        boxes.push_back(make_unique<Box>(initialBucket));

        while (boxes.size() < N) {

            unique_ptr<Box> largestBox = std::move(boxes.back());
            boxes.pop_back();

            auto [B1, B2] = largestBox->splitBoxOnMedian();
            B1->histogram(initialBucket);
            B2->histogram(initialBucket);


            boxes.push_back(std::move(B1));
            boxes.push_back(std::move(B2));
            std::sort(boxes.begin(), boxes.end(), [](unique_ptr<Box> &lhs, unique_ptr<Box> &rhs) {
//                return (lhs->rangeG() + lhs->rangeR() + lhs->rangeB()) <
//                       (rhs->rangeG() + rhs->rangeR() + rhs->rangeB());
                return lhs->getMaxRange().second < rhs->getMaxRange().second;
            });
        }

        vector<Pixel> palette{};
        for (auto &b: boxes) {
            palette.push_back(b->getMean2());
        }

        return palette;
    }

    static vector<Pixel> getPaletteKMeans(size_t N, vector<Pixel> &pixels) {
        vector<Pixel> palette = getPalette(N);
        vector<Pixel> oldPalette(N);
        while (!isEqual(oldPalette, palette)) {

//            cout << format("oldPalette: {},{},{}  |  {},{},{}  |  {},{},{}  |  {},{},{}  |  {},{},{}",
//                           oldPalette[0][0], oldPalette[0][1], oldPalette[0][2]
//                           ,oldPalette[1][0],oldPalette[1][1],oldPalette[1][2]
//                           ,oldPalette[2][0],oldPalette[2][1],oldPalette[2][2]
//                           ,oldPalette[3][0],oldPalette[3][1],oldPalette[3][2],
//                           oldPalette[4][0],oldPalette[4][1],oldPalette[4][2])<<endl;
//            cout << format("palette: {},{},{}  |  {},{},{}  |  {},{},{}  |  {},{},{}  |  {},{},{}",
//                           palette[0][0], palette[0][1], palette[0][2]
//                    ,palette[1][0],palette[1][1],palette[1][2]
//                    ,palette[2][0],palette[2][1],palette[2][2]
//                    ,palette[3][0],palette[3][1],palette[3][2],
//                           palette[4][0],palette[4][1],palette[4][2])<<endl;
            oldPalette = palette;
            vector<vector<Pixel>> subGroups(N);
            std::for_each(pixels.begin(), pixels.end(), [&](Pixel &p) {
                vector<float> dst(palette.size());
                for (int i = 0; i < palette.size(); ++i) {
                    float dstTmp = 0;
                    dstTmp = abs(p[0] - palette[i][0]) + abs(p[1] - palette[i][1]) +
                             abs(p[2] - palette[i][2]);
                    dst[i] = dstTmp;
                }

                subGroups[std::distance(dst.begin(), std::min_element(dst.begin(), dst.end()))].push_back(p);
            });
            for (int i = 0; i < N; ++i) {
                array<size_t, 3> meanPixel = {0, 0, 0};
                for (auto &p: subGroups[i]) {
                    meanPixel[0] += p[0];
                    meanPixel[1] += p[1];
                    meanPixel[2] += p[2];
                }

                meanPixel[0] /= subGroups[i].size();
                meanPixel[1] /= subGroups[i].size();
                meanPixel[2] /= subGroups[i].size();
                palette[i] = Pixel{static_cast<unsigned char>(meanPixel[0]), static_cast<unsigned char>(meanPixel[1]),
                                   static_cast<unsigned char>(meanPixel[2])};
            }
        }


        return palette;
    }

    static bool isEqual(vector<Pixel> &lhs, vector<Pixel> &rhs) {
        if (lhs.size() != rhs.size())return false;

        for (int i = 0; i < lhs.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                if (abs(lhs[i][j] - rhs[i][j]) > 2 )return false;
            }
        }
        return true;
    }
};


int main() {
    auto start = steady_clock::now();

    ifstream is(R"(/home/nicola/Desktop/data_processing/images/test2.pam)", ios::binary);
    ofstream os(R"(/home/nicola/Desktop/data_processing/mcut/quantizedImg.pam)", ios::binary | ios::trunc);
    if (is.fail() or os.fail()) {
        perror("Error while opening input\\output file\n");
        return EXIT_FAILURE;
    }

    MedianCut mcut(is, os, 10);
    mcut.algoritm();

    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to execute decompression was :" << elapsed_ms.count() << "ms" << endl;
    return EXIT_SUCCESS;
}

