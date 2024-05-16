#include <iostream>
#include <vector>
#include <fstream>
#include "filesystem"
#include "algorithm"
#include "array"
#include "cstdlib"
#include "cstdint"
#include "cmath"
#include "iterator"
#include "chrono"

#define N 1024
#define N2 (N*2)

using namespace std;
using namespace chrono;

template<typename T>
vector<T> read_bytes(string &file_name) {
    ifstream is(file_name, ios::binary);
    if (is.fail()) {
        cout << "Error while trying to open input file\n";
        throw;
    }
    T tmp;
    vector<T> v;
    while (is.good()) {
        is.read(reinterpret_cast<char *>(&tmp), sizeof(T));
        v.push_back(tmp);
    }
    is.close();
    return v;
}

template<typename T>
void dump_file(string file_name, vector<T> data) {
    ofstream os(file_name, ios::binary | ios::trunc);
    if (os.fail()) {
        cout << "error while trying to open output file\n";
        throw;
    }
    os.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(T));
    os.close();
}

template<typename T>
struct Frequencies {
    using uT = make_unsigned_t<T>;
    array<size_t, numeric_limits<uT>().max()> stats{};
    size_t nSym = 0;
//    array<size_t, numeric_limits<make_unsigned_t<T>>().max()> freq{};

    void operator()(vector<T> &v) {
        nSym = v.size();
        stats = array<size_t, numeric_limits<uT>().max()>{};
        auto min = numeric_limits<T>().min();
        for (auto &item: v) {
            stats[item + abs(INT16_MIN)]++;
        }

    }

    void compute_entropy() {
        double h = log2(nSym);
        double tmp = 0;
        for (auto &item: stats) {
            if (item == 0 or item == 1) continue;
            tmp += item * log2(item);
        }
        h -= ((double) 1 / nSym) * tmp;
        cout << "the entropy is: " << h << endl;
    }


};

struct MDCT {
    array<double, N2> weights{};
    array<array<double, N2>, N> cosines{};

    MDCT() {
        for (int n = 0; n < N2; ++n) {
            weights[n] = sin((M_PI / N2) * (n + 0.5));
            for (int k = 0; k < N; ++k) {
                cosines[k][n] = cos((M_PI / N) * (n + 0.2 + N / 2) * (k + 0.5));
            }
        }
    }

    vector<int32_t> operator()(vector<int16_t> v) {
        while (v.size() % 1024) v.push_back(0);
        v.insert(v.begin(), 1024, 0);
        v.insert(v.end(), 1024, 0);
        vector<int32_t> coeff(v.size() - N);

        int nWin = v.size() / 1024 - 1;
        for (int win = 0; win < nWin; ++win) {
            for (int k = 0; k < N; ++k) {
                for (int n = 0; n < N2; ++n) {
                    coeff[win * N + k] += v[win * N + n] * weights[n] * cosines[k][n];
                }
            }
        }

        return coeff;
    }

    vector<int16_t> inverse(vector<int32_t> &coeff) {
        vector<int16_t> rec_v(coeff.size() - 1024);
        int nWin = coeff.size() / 1024 - 1;

        for (int win = 0; win < nWin; ++win) {
            for (int n = 0; n < N2; ++n) {
                for (int k = 0; k < N; ++k) {
                    rec_v[win * N + n] += coeff[win * N + k] * cosines[k][n];
                }
                rec_v[win * N + n] *= (double)2/N * weights[n];
            }

        }


        return rec_v;
    }
};


int main() {
    std::system("ulimit -s 34000");
    string file_name = "../test.raw";
    vector<int16_t> v = read_bytes<int16_t>(file_name);
    Frequencies<int16_t> freq{};
    freq(v);
    freq.compute_entropy();

    for (auto &item: v) {
        item /= 2600;
    }
    freq(v);
    freq.compute_entropy();

    auto start = steady_clock::now();
    MDCT mdct{};
    auto coeff = mdct(v);
//    dump_file("../coeff.bin", coeff);
    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to execute mdct was :" << elapsed_ms.count() << "ms" << endl;

    start = steady_clock::now();
    v = mdct.inverse(coeff);
    stop = steady_clock::now();
    elapsed_ms = stop - start;
    cout << "Elapsed time to execute inverse mdct was :" << elapsed_ms.count() << "ms" << endl;
    dump_file("../reconstructed.raw", v);

    cout << endl;
    return EXIT_SUCCESS;
}
