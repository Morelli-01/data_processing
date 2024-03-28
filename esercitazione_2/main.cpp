//
// Created by nicola on 21/03/24.
//

#include <string>
#include <fstream>
#include <vector>
#include <bit>
#include "iostream"
#include "chrono"

using std::cout;
using std::endl;
using namespace std::chrono;

class bitwriter {
    uint8_t buffer_{};
    size_t n_ = 0;
    std::ostream &os_;

    void writebit(uint64_t curbit) {
        buffer_ = buffer_ << 1 | (1 & curbit);
        n_++;
        if (n_ == 8) {
            os_.put(static_cast<char>(buffer_));
            n_ = 0;
        }
    }

public:
    explicit bitwriter(std::ostream &os) : os_(os), buffer_() {}

    ~bitwriter() {
        flush();
    }

    std::ostream &operator()(uint64_t x, int numbits) {
        for (int i = numbits - 1; i >= 0; i--) {
            writebit((x >> i) & 1);
        }
        return os_;
    }

    std::ostream &flush(int padbit = 0) {
        while (n_ > 0) {
            writebit(padbit);
        }
        return os_;
    }
};

class bitreader {
    uint8_t buffer_;
    size_t n_ = 0;
    std::istream &is_;

    uint64_t readbit() {
        if (n_ == 0) {
            buffer_ = is_.get();
            n_ = 8;
        }
        n_--;
        return (buffer_ >> n_) & 1;
    }

public:
    bitreader(std::istream &is) : is_(is), buffer_() {}

    ~bitreader() = default;

    uint64_t operator()(int numbits) {
        uint64_t v = 0;
        for (int i = 0; i != numbits; i++) {
            v = (v << 1) | readbit();
        }
        return v;
    }

    bool fail() {
        return is_.fail();
    }
};

class elias_writer {
    std::ostream &os_;


public:
    elias_writer(std::ostream &os) : os_(os) {}

    ~elias_writer() = default;

    static uint64_t encode(const int value) {
        uint64_t encoded_v = 1;
        if (value > 0) {
            encoded_v = (value) * 2 + 1;
        } else if (value < 0) {
            encoded_v = (-value) * 2;
        }
        return encoded_v;

    }

    std::ostream &compress(std::istream &is) {
        int tmp;
        bitwriter bw(os_);
        while (is >> tmp) {
            uint64_t encoded_value = elias_writer::encode(tmp);
            uint64_t numbits = std::bit_width(encoded_value) * 2 - 1;
            bw(encoded_value, static_cast<int>(numbits));
        }
        return os_;
    }
};

class elias_reader {
    std::istream &is_;
public:
    elias_reader(std::istream &is) : is_(is) {}

    ~elias_reader() = default;

    static int decode(const uint64_t value) {
        if (value == 1) {
            return 0;
        }
        if (value % 2 == 0) {
            return -static_cast<int>(value / 2);
        } else {
            return static_cast<int>(value - 1) / 2;
        }
    }

    std::istream &decompress(std::ostream &os) {
        bitreader br(is_);
        while (!br.fail()) {
            int size = 0;
            uint64_t encoded_value = 1;
            while ((br(1) == 0) && (!br.fail())) size++;
            if (!br.fail()) {
                uint64_t tmp = br(size);
                encoded_value = encoded_value << size | tmp;
                os << elias_reader::decode(encoded_value) << '\n';
            }

        }
        return is_;
    }
};

int error(const std::string &reason) {
    cout << reason << endl;
    return EXIT_FAILURE;
}

int compress(const std::string &in_file, const std::string &out_file) {
    std::ifstream is(in_file);
    std::ofstream os(out_file, std::ios::binary);
    if (is.fail() or os.fail()) {
        return error("Error while opening input/output file");
    }
    elias_writer ew(os);
    ew.compress(is);
    return EXIT_SUCCESS;
}

int decompress(const std::string &in_file, const std::string &out_file) {
    std::ifstream is(in_file, std::ios::binary);
    std::ofstream os(out_file);
    elias_reader er(is);
    er.decompress(os);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    auto start = steady_clock::now();
    if (argc != 4) {
        return error("A different number then 3 of parameteres were passed!");
    }
    if (std::string(argv[1]) == "c") {
        return compress(argv[2], argv[3]);
    } else if (std::string(argv[1]) == "d") {
        auto value = decompress(argv[2], argv[3]);
        auto stop = steady_clock::now();
        duration<double, std::milli> elapsed_ms = stop - start;
        cout << "Elapsed time to execute decompression was :" << elapsed_ms.count() << "ms" << endl;
        return value;
    } else {
        return error("Only [c|d] can be passed as first parametere!");
    }
}