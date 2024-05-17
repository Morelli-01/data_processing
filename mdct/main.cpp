#include <iostream>
#include <fstream>
#include <vector>
#include "string"
#include "array"
#include "cstdlib"
#include "cstdint"
#include "cmath"
#include "algorithm"
#include "map"
#include "chrono"

#define N 1024
#define N2 (N*2)
using namespace std;
using namespace std::chrono;

vector<int16_t> read_bytes(string file_name) {
    vector<int16_t> v{};
    ifstream is(file_name, ios::binary);
    if (is.fail()) return v;
    int16_t tmp;
    is.read(reinterpret_cast<char *>(&tmp), 2);
    while (is.good()) {
        v.push_back(tmp);
        is.read(reinterpret_cast<char *>(&tmp), 2);
    }
    is.close();
    return v;
}

void dump_audio(string file_name, vector<int16_t> &v) {
    ofstream os(file_name, ios::binary | ios::trunc);
    if (os.fail()) {
        cout << "error while dumping audio file\n";
        return;
    }
    os.write(reinterpret_cast<const char *>(v.data()), v.size() * 2);
    os.close();
}

struct frequency {
    array<size_t, 65536> freq{};
    size_t nSym{};

    void compute_freq(vector<int16_t> &v) {
        freq = array<size_t, 65536>{};
        nSym = v.size();
        for (auto &item: v) {
            freq[item + 32768]++;
        }
    }

    double compute_entropy() {
        double h = 0;
        for (auto &item: freq) {
            if (item == 0) continue;
            h -= (double) item / nSym * log2((double) item / nSym);
        }
        return h;
    }
};

struct MDCT {
    array<double, N2> weights{};
    vector<double> cosines;

    MDCT() : cosines(N * N2) {
        double size = 2 * N;
        for (int n = 0; n < size; ++n) {
            weights[n] = sin((M_PI / size) * (n + 0.5));
            for (int k = 0; k < N; ++k) {
                cosines[n * N + k] = cos((M_PI / N) * (n + 0.5 + N / 2) * (k + 0.5));
            }
        }
    }

    vector<int32_t> operator()(vector<int16_t> &v) {

        int nWin = v.size() / N;
        vector<int32_t> coeff((nWin - 1) * N);

        for (int win = 0; win < nWin - 1; ++win) {
//            cout << "win: " << win << endl;
            for (int k = 0; k < N; ++k) {
                for (int n = 0; n < N2; ++n) {
                    coeff[win * N + k] += v[win * N + n] * weights[n] * cosines[n * N + k];
                }
            }
        }

        return coeff;
    }

    vector<int16_t> inverse(vector<int32_t> &v) {
        int nWin = v.size() / N;
        vector<int16_t> coeff((nWin - 1) * N);
        for (int win = 0; win < nWin - 1; ++win) {
            for (int n = 0; n < N2; ++n) {
                for (int k = 0; k < N; ++k) {
//                    auto tmp = coeff[win * N + n];
//                    auto tmp1 = weights[n];
//                    auto tmp2 = v[win * N + k];
//                    auto tmp4 = cosines[n * N + k];

                    coeff[win * N + n] += v[win * N + k] * cosines[n * N + k];
                }
            }
        }

        for (int win = 0; win < nWin - 1; ++win) {
            for (int i = 0; i < N; ++i) {
                coeff[win * N + i] *= (double) 2 / N * weights[i];
            }

        }

        return coeff;
    }

    static void dump_coeff(string file_name, vector<int32_t> &v) {
        ofstream os(file_name, ios::binary | ios::trunc);
        if (os.fail()) {
            cout << "error while dumping coeff file\n";
            return;
        }
        os.write(reinterpret_cast<const char *>(v.data()), v.size() * sizeof(int32_t));
        os.close();
    }

    static vector<int32_t> parse_coeff(string file_name) {
        vector<int32_t> v{};
        ifstream is(file_name, ios::binary);
        if (is.fail()) return v;
        int16_t tmp;
        is.read(reinterpret_cast<char *>(&tmp), sizeof(int32_t));
        while (is.good()) {
            v.push_back(tmp);
            is.read(reinterpret_cast<char *>(&tmp), sizeof(int32_t));
        }
        is.close();
        return v;
    }

};

void compute_qtz_err(string file_name) {
    vector v = read_bytes(file_name);
    frequency stats{};
    stats.compute_freq(v);
    cout << stats.compute_entropy() << endl;

    vector<int16_t> qtz{};
    for (auto &item: v) {
        qtz.push_back(item / 2600);
    }

    stats.compute_freq(qtz);
    cout << stats.compute_entropy() << endl;


    for (auto &item: qtz) {
        item *= 2600;
    }
    dump_audio("../output_qt.raw", qtz);

    vector<int16_t> err{};
    for (int i = 0; i < v.size(); ++i) {
        err.push_back(v[i] - qtz[i]);
    }
    dump_audio("../error_qt.raw", err);
}


int main() {
    vector v = read_bytes("/home/nicola/Desktop/data_processing/mdct/test.raw");

    v.insert(v.begin(), 1024, 0);
    while (v.size() % 1024 != 0) {
        v.push_back(0);
    }
    v.insert(v.end(), 1024, 0);
//
    MDCT mdct{};

    auto start = steady_clock::now();

    auto coeff = mdct(v);

    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << "Elapsed time to compute MDCT coefficient was :" << elapsed_ms.count() << "ms" << endl;
//
//    MDCT::dump_coeff("../coeff.bin", coeff);

//    auto coeff = MDCT::parse_coeff("../coeff.bin");
//
    start = steady_clock::now();

    auto v_rec = mdct.inverse(coeff);
    dump_audio("../reconstructed.raw", v);

    stop = steady_clock::now();
    elapsed_ms = stop - start;
    cout << "Elapsed time to execute reconstruct was :" << elapsed_ms.count() << "ms" << endl;


    return EXIT_SUCCESS;
}
