//
// Created by nicola on 10/04/2024.
//
#include <cstdlib>
#include <fstream>
#include <array>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <memory>
#include <map>

using namespace std;

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

struct Node {


    double prob;
    uint8_t c;
    unique_ptr<Node> lhs;
    unique_ptr<Node> rhs;
    int code;
    int len;

    Node(double prob, uint8_t c, unique_ptr<Node> lhs, unique_ptr<Node> rhs) : prob(prob), c(c), lhs(std::move(lhs)),
                                                                               rhs(std::move(rhs)) {
        code = 0;
        len = 0;
    }

    Node(Node *node) {

    }
//    Node(Node &other) {
//        cout << "NODE COPY CONSTRUCTOR WAS CALLED !!!!\n";
//    }

    ~Node() {
        cout << "NODE DECONSTRUCTOR WAS CALLED\n";
    }

    static bool cmpNodes(const unique_ptr<Node> &lhs, const unique_ptr<Node> &rhs) {
        return lhs->c < rhs->c;
    }
};


class Frequences {
    array<uint8_t, 256> data_{};
public:

    vector<unique_ptr<Node>> distribution{};
    int totalByte = 0;

    Frequences() {
        cout << "FREQUENCES COSTRUCTOR HAS BEEN CALLED\n";
    }

    Frequences(Frequences &other) {
        cout << "FREQUENCES COPY-COSTRUCTOR HAS BEEN CALLED\n";
    }

    Frequences(Frequences &&other) noexcept {
        data_ = std::move(other.data_);
        distribution = std::move(distribution);
        cout << "FREQUENCES MOVE-COSTRUCTOR HAS BEEN CALLED\n";
    }

    ~Frequences() {
        cout << "FREQUENCES DECONSTRUCTOR WAS CALLED!!!!\n";
    }

    void computeFrequences(istream &is) {
        uint8_t c;
        while ((c = is.get()) != 255) {
            data_[c]++;
            totalByte++;
        }
    }

    void computeDistribution() {
        for (int i = 0; i < 256; ++i) {
            if (data_[i] == 0)continue;
            distribution.emplace_back(new Node(static_cast<double>(data_[i]) / totalByte, static_cast<uint8_t>(i), nullptr,
                                               nullptr));
        }
        std::sort(distribution.begin(), distribution.end(), cmpDistr);


    }


    friend ostream &operator<<(ostream &os, const Frequences &freq) {
        for (int i = 0; i < 256; ++i) {
            if (freq.data_[i] == 0)continue;
            os << std::hex << std::setw(2) << setfill('0') << i;
            os << ": " << std::dec << (int) (freq.data_[i]) << endl;
        }
        os << endl;
        for (const auto &x: freq.distribution) {
            os << std::hex << std::setw(2) << setfill('0') << (int) (x->c);
            os << ": " << std::dec << (x->prob) << endl;
        }

        return os;
    }

    static bool cmpDistr(const unique_ptr<Node> &lhs, const unique_ptr<Node> &rhs) {
        return lhs->prob > rhs->prob;
    }
};

void recursive(unique_ptr<Node> &n, vector<unique_ptr<Node>> &codes, int sol = 0, int len = 0) {
    if (n->lhs == nullptr and n->rhs == nullptr) {
        cout << "Found a leaf\n";
        n->code = sol;
        n->len = len;
//        Node code = Node(n->prob, n->c, nullptr, nullptr);
//        code.code = sol;
//        code.len = len;
        codes.emplace_back(std::move(n));

        return;
    }
    if (n->lhs != nullptr) {
        recursive(n->lhs, codes, (sol << 1) | 0, len + 1);
    }
    if (n->rhs != nullptr) {
        recursive(n->rhs, codes, (sol << 1) | 1, len + 1);
    }


}

void mergeNodes(vector<unique_ptr<Node>> &vec) {
    while (vec.size() > 1) {
        unique_ptr<Node> e1 = std::move(vec.back());
        vec.pop_back();
        unique_ptr<Node> e2 = std::move(vec.back());
        vec.pop_back();
        unique_ptr<Node> nodeMerged = make_unique<Node>(e1->prob + e2->prob, ' ', std::move(e1), std::move(e2));
        auto p = nodeMerged->prob;
        vec.push_back(std::move(nodeMerged));
        for (int i = vec.size() - 2; i >= 0; i--) {
            if (vec[i]->prob < p) {
                std::swap(vec[i], vec[i + 1]);
            } else {
                break;
            }
        }
    }

}

void dumpHuffman1(ostream &os, map<uint8_t, unique_ptr<Node>>  &codes, istream& is, uint32_t nSym) {
    os.write("HUFFMAN1", 8);
    BitWriter br_(os);
    br_(codes.size(), 8);
    for(const auto& [c, code] : codes){
        br_(code ->c, 8);
        br_(code->len, 5);
        br_(code->code, code->len);
    }
    br_(nSym, 32);
    uint8_t tmp;
    while(is.good()){
        tmp = is.get();
        br_(codes[tmp]->c, codes[tmp]->len);
    }

}

int main(int argc, char **argv) {
    if (argc != 4) {
        perror("PARAMETERS!!!!\n");
        return EXIT_FAILURE;
    }
    ifstream is(argv[2], ios::binary);
    ofstream os(argv[3], ios::binary);
    if (is.fail() or os.fail()) {
        perror("ERROR WHILE OPENING INPUT/OUTPUT FILE!!!!\n");
        return EXIT_FAILURE;
    }

    Frequences freq = Frequences();
    freq.computeFrequences(is);
    is.close();

    freq.computeDistribution();
    cout << freq;

    mergeNodes(freq.distribution);
    cout << endl;
    cout << freq;
    vector<unique_ptr<Node>> codes{};
    recursive(freq.distribution[0], codes);
    map<uint8_t, unique_ptr<Node>> codesMap{};
    for (auto &item: codes){
        codesMap.insert({item->c, std::move(item)});
    }
    ifstream is2(argv[2], ios::binary);

    dumpHuffman1(os, codesMap, is2, freq.totalByte);
    cout << endl;

    return EXIT_SUCCESS;
}