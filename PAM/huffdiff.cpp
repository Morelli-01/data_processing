//
// Created by nicola on 26/04/2024.
//
#include "array"
#include "cmath"
#include "fstream"
#include "map"
#include "string"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

struct BitWriter {
    ostream &os_;
    uint8_t buffer_;
    size_t len = 0;

    explicit BitWriter(ostream &os) : os_(os), buffer_(0) {}

    ostream &writeBit(uint8_t bit) {
        buffer_ = (buffer_ << 1) | bit;
        len++;

        if (len == 8) {
            os_.put(static_cast<char>(buffer_));
            buffer_ = 0;
            len = 0;
        }
        return os_;
    }

    ostream &flush() {
        while (len != 0) {
            writeBit(0);
        }
        return os_;
    }

    ~BitWriter() { flush(); }

    ostream &operator()(uint64_t bits, int size) {
        for (int i = size - 1; i >= 0; i--) {
            writeBit((bits >> i) & 1);
        }
        return os_;
    }
};

struct BitReader {
    istream &is_;
    uint8_t buffer_;
    size_t size_;

    explicit BitReader(istream &is) : is_(is), buffer_(is_.get()), size_(8) {}

    uint8_t readBit() {
        if (size_ == 0) {
            buffer_ = is_.get();
            size_ = 7;
        } else {
            size_--;
        }
        return (buffer_ >> size_) & 1;
    }

    uint64_t operator()(size_t nbits) {
        uint64_t v = 0;

        for (int i = 0; i < nbits; ++i) {
            v = (v << 1) | readBit();
        }

        return v;
    }
};

struct Code {
    uint16_t sym;
    size_t size;
    double prob;
    uint64_t code;
};

struct Node {
    Code c;
    unique_ptr<Node> lhs;
    unique_ptr<Node> rhs;
};

template<typename T>
struct Mat {
    size_t rows_;
    size_t cols_;
    vector<T> data_;

    Mat(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_ = vector<T>(cols * rows);
    }

    T &operator()(size_t r, size_t c) {
        return data_[r * cols_ + c];
    }

    const T &operator()(size_t r, size_t c) const {
        return data_[r * cols_ + c];
    }

    T *data() {
        return data_.data();
    }

    size_t size() {
        return cols_ * rows_;
    }
};

struct PamHelper {
    static Mat<uint8_t> loadPamGray(ifstream &is) {
        Mat<uint8_t> mat(0, 0);
        string tmpStr{};
        is >> tmpStr;
        if (tmpStr != "P7") {
            return mat;
        }
        is >> tmpStr;
        map<string, size_t> header_{};
        string tmpV{};
        while (tmpStr != "ENDHDR") {
            is >> tmpV;

            if (tmpStr == "TUPLTYPE") {
                is >> tmpStr;
                continue;
            }
            header_[tmpStr] = atoi(tmpV.c_str());
            is >> tmpStr;
        }
        is.get();

        mat.rows_ = header_["HEIGHT"];
        mat.cols_ = header_["WIDTH"];
        mat.data_ = vector<uint8_t>(mat.rows_ * mat.cols_);

        is.read(reinterpret_cast<char *>(mat.data()), static_cast<int>(mat.cols_ * mat.rows_));

        return mat;
    }

    static void dumpPamGray(ostream &os, Mat<uint8_t> &mat) {

        os.write("P7", 2);
        os.put('\n');

        os.write("WIDTH ", 6);
        os.write(to_string(mat.cols_).c_str(), to_string(mat.cols_).size());
        os.put('\n');

        os.write("HEIGHT ", 7);
        os.write(to_string(mat.rows_).c_str(), to_string(mat.rows_).size());
        os.put('\n');

        os.write("DEPTH ", 6);
        os.write("1", 1);
        os.put('\n');

        os.write("MAXVAL ", 7);
        os.write("255", 3);
        os.put('\n');

        os.write("TUPLTYPE  ", 9);
        os.write("GRAYSCALE", 9);
        os.put('\n');

        os.write("ENDHDR", 6);
        os.put('\n');

        os.write(reinterpret_cast<const char *>(mat.data_.data()), mat.size());
    }
};

struct HuffDiffEncoder {

    array<size_t, 512> freq{};
    Mat<uint16_t> diffMat;
    size_t numSym = 0;
    vector<Node> distrib{};
    vector<Code> codes{};
    BitWriter bw_;

    explicit HuffDiffEncoder(Mat<uint8_t> &mat, ostream &os) : diffMat(mat.rows_, mat.cols_), bw_(os) {
        diffMat(0, 0) = mat(0, 0) + 255;
        for (int r = 1; r < mat.rows_; ++r) {
            diffMat(r, 0) = ((int) mat(r, 0) - (int) mat(r - 1, 0)) + 255;
        }

        for (int r = 0; r < mat.rows_; ++r) {
            for (int c = 1; c < mat.cols_; ++c) {
                diffMat(r, c) = ((int) mat(r, c) - (int) mat(r, c - 1)) + 255;
            }
        }
    }

    void computeFreq() {
        std::for_each(diffMat.data_.begin(), diffMat.data_.end(), [&](uint16_t &v) {
            freq[v]++;
            numSym++;
        });
    }

    void computeDistribution() {
        for (int i = 0; i < 512; ++i) {
            if (freq[i] == 0) continue;
            distrib.emplace_back(Code(i, 0, (double) freq[i] / (double) numSym, 0), nullptr, nullptr);
        }
        std::sort(distrib.begin(), distrib.end(), cmpNode);
    }

    void createTree() {
        while (distrib.size() > 1) {
            unique_ptr<Node> rhs = make_unique<Node>(std::move(distrib.back()));
            distrib.pop_back();
            unique_ptr<Node> lhs = make_unique<Node>(std::move(distrib.back()));
            distrib.pop_back();
            Node branch = Node(Code(UINT16_MAX, 0, lhs->c.prob + rhs->c.prob),
                               std::move(lhs), std::move(rhs));
            distrib.push_back(std::move(branch));
            for (int i = distrib.size() - 1; i > 0; --i) {
                if (distrib[i].c.prob > distrib[i - 1].c.prob) {
                    std::swap(distrib[i], distrib[i - 1]);
                } else
                    break;
            }
        }
    }

    void getCodes(Node *node, uint64_t code = 0, int len = -1) {
        len++;
        if (node->lhs == nullptr and node->rhs == nullptr) {
            //            cout << "Found a code\n";
            //            cout << "Total codes: " << codes.size() << endl;
            codes.emplace_back(node->c.sym, len, node->c.prob, code);
        }
        if (node->lhs != nullptr) {
            getCodes(node->lhs.get(), (code << 1) | 1, len);
        }
        if (node->rhs != nullptr) {
            getCodes(node->rhs.get(), (code << 1) | 0, len);
        }
    }

    void getCanonical() {
        uint64_t code = 0;
        size_t len = 0;
        for (auto &c: codes) {
            code = code << (c.size - len);
            len = c.size;
            c.code = code;
            code++;
        }
    }

    void algoritm() {
        computeFreq();
        computeDistribution();
        createTree();
        getCodes(&distrib[0]);
        std::sort(codes.begin(), codes.end(), cmpCode);
        getCanonical();
    }

    void encode() {
        string magicNumber = "HUFFDIFF";
        std::for_each(magicNumber.begin(), magicNumber.end(), [&](char &c) {
            bw_(c, 8);
        });

        //matdiff WIDTH encoded as little endian - 32bits
        bw_((diffMat.cols_ >> 0) & 255, 8);
        bw_((diffMat.cols_ >> 8) & 255, 8);
        bw_((diffMat.cols_ >> 16) & 255, 8);
        bw_((diffMat.cols_ >> 24) & 255, 8);
        //matdiff HEIGHT encoded as little endian - 32bits
        bw_((diffMat.rows_ >> 0) & 255, 8);
        bw_((diffMat.rows_ >> 8) & 255, 8);
        bw_((diffMat.rows_ >> 16) & 255, 8);
        bw_((diffMat.rows_ >> 24) & 255, 8);
        //Number of items in the following Huffman table - 9bits
        bw_(codes.size(), 9);
        //NumElement couples (symbol, size) - (unsigned 9 bits integer, unsigned 5 bits integer)
        map<uint16_t, Code> codeMap{};

        for (const auto &code: codes) {
            bw_(code.sym, 9);
            bw_(code.size, 5);
            codeMap[code.sym] = code;
        }
        //diffMat encoded with canonical Huffman

        for (uint16_t &sym: diffMat.data_) {
            Code c = codeMap[sym];
            bw_(c.code, c.size);
        }
    }

    static Mat<uint8_t> diffMatPam(Mat<uint8_t> &mat) {
        Mat<uint8_t> diffMat(mat.rows_, mat.cols_);
        diffMat(0, 0) = mat(0, 0);
        for (int r = 1; r < mat.rows_; ++r) {

            diffMat(r, 0) = ((int) mat(r, 0) - (int) mat(r - 1, 0)) / 2 + 127;
        }

        for (int r = 0; r < mat.rows_; ++r) {
            for (int c = 1; c < mat.cols_; ++c) {
                diffMat(r, c) = ((int) mat(r, c) - (int) mat(r, c - 1)) / 2 + 127;
            }
        }

        return diffMat;
    }

    static bool cmpNode(Node &lhs, Node &rhs) {
        if (lhs.c.prob == rhs.c.prob) {
            return lhs.c.sym < rhs.c.sym;
        }
        return lhs.c.prob > rhs.c.prob;
    }

    static bool cmpCode(Code &lhs, Code &rhs) {
        if (lhs.size == rhs.size) {
            return lhs.sym < rhs.sym;
        }
        return lhs.size < rhs.size;
    }
};

struct HuffDiffDecoder {
    BitReader br_;
    map<pair<size_t, uint64_t>, Code> cipher{};

    explicit HuffDiffDecoder(istream &is) : br_(is) {}

    void decode(ostream &os) {
        string magicNumber{};
        for (int i = 0; i < 8; ++i) {
            magicNumber += static_cast<char>(br_(8));
        }
        if (magicNumber != "HUFFDIFF") {
            perror("The input file is not a file encoded as HUFFDIF");
            exit(1);
        }

        size_t cols = (size_t) br_(8) << 0 | (size_t) br_(8) << 8 | (size_t) br_(8) << 16 | (size_t) br_(8) << 24;
        size_t rows = (size_t) br_(8) << 0 | (size_t) br_(8) << 8 | (size_t) br_(8) << 16 | (size_t) br_(8) << 24;

        size_t nCodes = br_(9);
        vector<pair<uint16_t, size_t>> huffTable;
        for (int i = 0; i < nCodes; ++i) {
            uint16_t symbol = br_(9);
            size_t size = br_(5);
            huffTable.emplace_back(symbol, size);
        }
        std::sort(huffTable.begin(), huffTable.end(),
                  [](pair<uint16_t, size_t> &lhs, pair<uint16_t, size_t> &rhs) {
                      if (lhs.second == rhs.second) {
                          return lhs.first < rhs.first;
                      }
                      return lhs.second < rhs.second;
                  });
        uint64_t code = 0;
        size_t len = 0;
        for (const auto &entry: huffTable) {
            code = code << (entry.second - len);
            cipher[{entry.second, code}] = Code(entry.first, entry.second, 0, code);
            code++;
            len = entry.second;
        }

        Mat<uint16_t> diffMat(rows, cols);
        for (int i = 0; i < diffMat.size(); ++i) {
            code = 0;
            len = 0;
            while (true) {
                code = (code << 1) | br_(1);
                len++;
                if (cipher.contains({len, code})) {
                    diffMat.data_[i] = cipher[{len, code}].sym;
                    break;
                }
            }
        }
        Mat<uint8_t> decodedMat(rows, cols);
        decodedMat(0, 0) = diffMat(0, 0) - 255;
        for (int i = 1; i < rows; ++i) {
            decodedMat(i, 0) = diffMat(i, 0) + decodedMat(i - 1, 0) - 255;
        }
        for (int r = 0; r < rows; ++r) {
            for (int c = 1; c < cols; ++c) {
                decodedMat(r, c) = diffMat(r, c) + decodedMat(r, c - 1) - 255;
            }
        }
        PamHelper::dumpPamGray(os, decodedMat);
    }
};

struct Entropy {

    template<typename T>
    static double entropy(Mat<T> &data) {
        map<uint16_t, size_t> freq{};
        size_t numElem = data.size() * 3;
        for (int i = 0; i < data.size(); ++i) {

            if (freq.contains(data.data_[i])) {
                freq[data.data_[i]]++;
            } else {
                freq[data.data_[i]] = 1;
            }
        }
        double entropy = 0;
        for (auto &[key, value]: freq) {
            double prob = (double) value / (double) numElem;
            entropy += prob * log2(prob);
        }
        entropy = -entropy;
        cout << "Entropy is: " << entropy << endl;
        return 0.0;
    }
};


int main(int argc, char **argv) {
    if (argc != 4) {
        perror("Error in the number of parameters passed, just 3 are required!!\n");
        return EXIT_FAILURE;
    }
    if (string{argv[1]} == "c") {
        if (!string{argv[2]}.contains(".pam")) {
            perror("The first file passed as parameters is not a .pam file!\n");
            return EXIT_FAILURE;
        }
        ifstream is(argv[2], ios::binary);
        if (is.fail()) {
            perror("Error while opening the input file\n");
            return EXIT_FAILURE;
        }
        Mat<uint8_t> img = PamHelper::loadPamGray(is);
        is.close();
//        Entropy::entropy(img);



        ofstream os = ofstream(argv[3], ios::binary | ios::trunc);
        if (os.fail()) {
            perror("Error while opening the output file\n");
            return EXIT_FAILURE;
        }
        HuffDiffEncoder hf_(img, os);
        hf_.algoritm();
        hf_.encode();
//        Entropy::entropy(hf_.diffMat);

        return EXIT_SUCCESS;
    } else if (string{argv[1]} == "d") {

        ifstream is(argv[2], ios::binary);
        if (is.fail()) {
            perror("Error while opening the input file\n");
            return EXIT_FAILURE;
        }
        HuffDiffDecoder hf_(is);
        ofstream os(argv[3], ios::binary);
        if (os.fail()) {
            perror("Error while opening the output file\n");
            return EXIT_FAILURE;
        }
        hf_.decode(os);

        return EXIT_SUCCESS;
    }
    perror("The first parameters must be 'c' for compression or 'd' for decompression\n");


    return EXIT_FAILURE;
}