#include <iostream>
#include <cstdint>
#include <fstream>
#include <bitset>


using namespace std;

class BitReader {
    istream &is_;
    uint8_t buffer_{};
    int len_ = 0;

    uint8_t readBit() {
        if (len_ == 0) {
            buffer_ = is_.get();
            len_ = 8;
        }
        len_--;
        return (buffer_ >> len_) & 1;
    }

public:
    explicit BitReader(istream &is) : is_(is) {}

    uint64_t operator()(int nbit) {
        if (nbit > 64) {
            return 0;
        }

        uint64_t value = 0;

        for (int i = 0; i < nbit; ++i) {
            uint64_t b = readBit();
            value = (value << 1) | b;
        }

        return value;

    }

    bool good() {
        return is_.good();
    }

};

class BitWriter {
    ostream &os_;
    uint8_t buffer_{};
    int len_ = 0;

    ostream &writeBit(uint8_t bit) {
        buffer_ = (buffer_ << 1) | bit;
        len_++;

        if (len_ == 8) {
            os_.put(static_cast<char>(buffer_));
            buffer_ = 0;
            len_ = 0;
        }
        return os_;
    }


public:
    explicit BitWriter(ostream &os) : os_(os) {}

    ostream &operator()(uint64_t value, int nBits) {
        for (int i = nBits - 1; i >= 0; --i) {
            writeBit((value >> i) & 1);
        }

        return os_;
    }
};

int main(int argc, char **argv) {

    if (argc < 3) {
        return EXIT_FAILURE;
    }
    ifstream is(argv[1], ios::binary);
    if (is.fail()) {
        perror("Error creating output file\n<input_file> <output_file>\n");
        return EXIT_FAILURE;
    }

    ofstream os(argv[2], ios::binary | ios::trunc);
    if (os.fail()) {
        perror("Error creating output file\n <input_file> <output_file>\n");
        return EXIT_FAILURE;
    }

    cout << "Please insert encryption key:\n";
    string pwd{};
    cin >> pwd;
    cout << pwd << endl;

    if (pwd.length() * 8 > 64) {
        pwd = pwd.substr(0, 8);
    } else if (pwd.length() * 8 < 64) {
        while (pwd.length() * 8 < 64) {
            pwd += pwd;
        }
    }

    cout << "start encryption\n";

    BitReader br_(is);
    BitWriter bw_(os);
    bitset<64> k = 0;
    bitset<64> pwdB = 0;
    for (int i = 0; i < 8; ++i) {
        pwdB = pwd[8 - 1 - i];
        k = (k << 8) | (pwdB);
//        cout << k << endl;
    }
    while (br_.good()) {
        bitset<64> x = br_(64);
        auto batch = x xor k;
        bw_(batch.to_ullong(), 64);
    }


    cout << "encryption done\n";


    return EXIT_SUCCESS;
}
