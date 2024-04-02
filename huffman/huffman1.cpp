//
// Created by nicola on 28/03/2024.
//


#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <list>
#include <cstdint>
#include <map>
#include <array>
#include "iostream"

using std::cout;
using std::endl;


int error(const std::string &reason) {
    cout << reason << endl;
    return EXIT_FAILURE;
}

class BitWriter {
    std::ostream &os_;
    uint8_t buffer_{};
    size_t n_ = 0;

    std::ostream &writebit(uint64_t b) {
        buffer_ = buffer_ << 1 | (static_cast<char>(1) & b);
        n_++;
        if (n_ == 8) {
            os_.put(static_cast<char>(buffer_));
            buffer_ = 0;
            n_ = 0;
        }
        return os_;
    }


public:
    explicit BitWriter(std::ostream &os_) : os_(os_) {}

    std::ostream &operator()(uint64_t x, int numbits) {
        for (int i = numbits - 1; i >= 0; i--) {
            writebit((x >> i) & 1);
        }
        return os_;
    }

    ~BitWriter() {
        flush();
    }

    std::ostream &flush() {
        while (n_ > 0) {
            writebit(0);
        }
        return os_;
    }

};

class BitReader {
    std::istream &is_;
    uint8_t buffer_{};
    size_t n_ = 0;

    uint8_t readbit() {
        uint8_t bit = (buffer_ >> (n_ - 1)) & 1;
        n_--;
        if (n_ == 0) {
            buffer_ = is_.get();
            n_ = 8;
        }

        return bit;
    }

public:
    explicit BitReader(std::istream &is) : is_(is) {
        buffer_ = is_.get();
        n_ = 8;
    }

    uint64_t operator()(size_t numbits) {
        uint64_t value = 0;
        for (int i = 0; i < numbits; ++i) {
            value = (value << 1) | readbit();
        }
        return value;
    }

};

struct Code {
    int code;
    size_t n;
    unsigned char c;
};

struct Node {
    double value;
    unsigned char c;
    Node *rhs;
    Node *lhs;
};

class frequencies {
public:
    std::array<int, 256> data_{};
    std::vector<std::pair<unsigned char, double>> pdf_{};
    std::vector<Node *> tree{};
    std::map<unsigned char, Code *> codes;
    int count = 0;


    std::ifstream &read_bytes(std::ifstream &is) {
        unsigned char c;
        while ((c = is.get()) != 255) {
            data_[c]++;
            count++;
        }
        return is;
    }

    void compute_pdf() {
        for (int i = 0; i < 256; i++) {
            if (data_[i] == 0) {
                continue;
            }
            pdf_.emplace_back(i, static_cast<double>(data_[i]) / count);
        }
        std::sort(pdf_.begin(), pdf_.end(), doubleCmp);
    }

    void compute_tree() {
        for (const auto &x: pdf_) {
            tree.push_back(new Node(x.second, x.first, nullptr, nullptr));
        }
        while (tree.size() > 1) {
            Node *e1 = tree.back();
            tree.pop_back();
            Node *e2 = tree.back();
            tree.pop_back();
            tree.push_back(new Node(e1->value + e2->value, ' ', e1, e2));
            for (int i = tree.size() - 1; i > 0; i--) {
                if (tree.at(i)->value > tree.at(i - 1)->value) {
                    std::swap(tree.at(i), tree.at(i - 1));
                }
            }
        }
    }

    void recursive(const Node &node, int sol = 0, int n = -1) {
        n++;
        if (node.lhs == nullptr and node.rhs == nullptr) {
            codes.insert({node.c, new Code(sol, n)});
            return;
        }
        if (node.rhs != nullptr) {
            recursive(*(node.rhs), ((sol << 1) | 0), n);
        }
        if (node.lhs != nullptr) {
            recursive(*(node.lhs), ((sol << 1) | 1), n);
        }
    }

    static bool doubleCmp(const std::pair<unsigned char, double> &lhs, const std::pair<unsigned char, double> &rhs) {
        return lhs.second > rhs.second;
    }
};

class HuffmanWriter {
    const std::map<unsigned char, Code *> &huffmanTable_;
    BitWriter bw_;
    std::istream &is_;

public:
    explicit HuffmanWriter(std::ostream &os_, const std::map<unsigned char, Code *> &huffmanTable_, std::istream &is_)
            : bw_(os_), huffmanTable_(
            huffmanTable_), is_(is_) {}


    void operator()(int nSym) {
        for (const auto &x: "HUFFMAN1") {
            if (x == '\0')continue;
            bw_(x, 8);
        }
        bw_(static_cast<char>(huffmanTable_.size()), 8);
        for (const auto &x: huffmanTable_) {
            bw_(x.first, 8);
            bw_(x.second->n, 5);
            bw_(x.second->code, x.second->n);
        }
        bw_(nSym, 32);
        unsigned char tmp;
        while ((tmp = is_.get()) != 255) {
            auto code = *(huffmanTable_.at(tmp));
            bw_(code.code, code.n);

        }
    }
};

class HuffmanReader {
    BitReader br_;
    std::ostream &os_;

public:
    explicit HuffmanReader(std::istream &is_, std::ostream &os_) : br_(is_), os_(os_) {}

    void operator()() {
        for (int i = 0; i < 8; ++i) {
            char tmp;
            tmp = br_(8);
            cout << tmp;
        }

        int tableEntries = br_(8);
        std::map<std::pair<int, int>, Code *> huffmanTable{};

        for (int i = 0; i < tableEntries; ++i) {
            unsigned char c = br_(8);
            int len = br_(5);
            int code = br_(len);
            huffmanTable[{code, len}] = new Code(code, len, c);
        }

        uint32_t numSym = br_(32);
        for (int i = 0; i < numSym; ++i) {
            bool isCode = false;
            int c = 0;
            int l = 0;
            while (!isCode) {
                uint8_t bit = br_(1);
                c = (c << 1) | bit;
                l++;
                if (huffmanTable.contains({c, l})) {
                    os_.put(huffmanTable[{c, l}]->c);
//                        cout << huffmanTable[{c, l}]->c;
                    isCode = true;
                }
            }
        }

    }


};


//template<typename T>
//std::ostream &operator<<(std::ostream &os, std::array<T, 256> &data) {
//    for (int i = 0; i < 256; i++) {
//        os << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(i);
//        os << ": " << std::dec << data[i] << endl;
//    }
//    return os;
//}


int main(int argc, char **argv) {
    if (argc != 4) {
//        auto err = std::format("{ parameters were passed while just 3 a required", argc - 1);
        return error("Too much/too few parameters were passed while just 3 a required");
    }
    std::ifstream is(argv[2], std::ios::binary);
    if (!is.good()) {
        return error("Error encountered while opening input file");
    }
    std::ofstream os(argv[3], std::ios::binary);
    if (!os.good()) {
        return error("Error encountered while opening output file");
    }

    if (std::string(argv[1]) == "c") {
        frequencies stats{};
        stats.read_bytes(is);
        is.close();
        stats.compute_pdf();
        stats.compute_tree();
        stats.recursive(*stats.tree[0]);

        std::ifstream is(argv[2], std::ios::binary);
        if (!is.good()) {
            return error("Error encountered while opening input file");
        }
        HuffmanWriter hf_(os, stats.codes, is);
        hf_(stats.count);


    } else if (std::string(argv[1]) == "d") {
        HuffmanReader hr_(is, os);
        hr_();
    }
    return EXIT_SUCCESS;
}