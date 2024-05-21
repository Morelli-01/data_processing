//
// Created by nicola on 5/21/24.
//


//
// Created by nicola on 17/05/2024.
//
#include <ranges>
#include <algorithm>
#include "vector"
#include "cstdint"
#include "iostream"
#include "fstream"
#include "chrono"
#include "cmath"
#include "functional"
#include "array"

#define WIN_SIZE 1024
#define DOUBLE_WIN_SIZE (WIN_SIZE*2)
#define M_PI 3.14159265358979323846
#define QTZ_VALUE 100000


using namespace std;
using namespace chrono;

template<typename T>
vector<T> read_bytes(string file_name) {
    ifstream is(file_name, ios::binary);
    is.seekg(0, ios::end);
    std::streamsize fileSize = is.tellg();
    is.seekg(0, ios::beg);
    vector<T> v(fileSize / sizeof(T));
    is.read(reinterpret_cast<char *>(v.data()), fileSize);
    is.close();
    return v;
}


template<typename T>
struct frequency {
    using uT = make_unsigned_t<T>;
    unordered_map<size_t, size_t> freq{};
    size_t nSym{};

    void compute_freq(vector<T> &v) {
        freq = unordered_map<size_t, size_t>{};
        nSym = v.size();
        for (auto &item: v) {
            if (!freq.contains(item)) freq[item] = 0;
            freq[item]++;
        }
    }

    double compute_entropy() {
        double h = 0.0;
        for (auto &item: ranges::views::filter(freq | ranges::views::values,
                                               bind(not_equal_to(), std::placeholders::_1, 0))) {
            h += item * log2((double) item);
        }
        h = log2(nSym) - ((double) 1 / nSym) * h;
        return h;
    }
};

struct MDCT {
    array<double, DOUBLE_WIN_SIZE> weights{};
    vector<double> cosines;
    size_t quantizationValue;


    MDCT(size_t quantizationValue) : cosines(WIN_SIZE * DOUBLE_WIN_SIZE), quantizationValue(quantizationValue) {
        double size = 2 * WIN_SIZE;
        for (int n = 0; n < DOUBLE_WIN_SIZE; ++n) {
            weights[n] = sin((M_PI / size) * (n + 0.5));
            for (int k = 0; k < WIN_SIZE; ++k) {
                cosines[k * DOUBLE_WIN_SIZE + n] = cos((M_PI / WIN_SIZE) * (n + 0.5 + WIN_SIZE / 2) * (k + 0.5));
            }
        }
    }

    vector<int32_t> operator()(vector<int16_t> v) {

        v.insert(v.begin(), WIN_SIZE, 0);
        v.insert(v.end(), WIN_SIZE - v.size() % WIN_SIZE, 0);
        v.insert(v.end(), WIN_SIZE, 0);
        int nWin = v.size() / WIN_SIZE;
        vector<int32_t> coeff{};


        for (int win = 0; win < nWin - 1; ++win) {
            vector<double> coeffWin_ = coeffWin(v, win);
            auto t = coeffWin_ | ranges::views::transform(bind(divides(), std::placeholders::_1, quantizationValue));
            coeff.insert(coeff.end(), std::make_move_iterator(t.begin()), std::make_move_iterator(t.end()));
        }
        return coeff;
    }

    vector<double> coeffWin(vector<int16_t> &data, size_t win) {
        size_t start_index = WIN_SIZE * win;
        vector<double> winCoeff(WIN_SIZE);
        for (int k = 0; k < WIN_SIZE; ++k) {
            for (int n = 0; n < DOUBLE_WIN_SIZE; ++n) {
                winCoeff[k] += data[start_index + n] * weights[n] * cosines[k * DOUBLE_WIN_SIZE + n];
            }
        }
        return winCoeff;
    }
};

struct IMDCT {
    array<double, DOUBLE_WIN_SIZE> weights{};

    vector<double> cosines;
    int quantizationValue_;

    IMDCT(int quantizationValue) : cosines(WIN_SIZE * DOUBLE_WIN_SIZE) {
        quantizationValue_ = quantizationValue;
        double size = 2 * WIN_SIZE;
        for (int n = 0; n < DOUBLE_WIN_SIZE; ++n) {
            weights[n] = sin((M_PI / size) * (n + 0.5));
            for (int k = 0; k < WIN_SIZE; ++k) {
                cosines[n * WIN_SIZE + k] = cos((M_PI / WIN_SIZE) * (n + 0.5 + WIN_SIZE / 2) * (k + 0.5));
//                cosines[k * DOUBLE_WIN_SIZE + n] ;
            }
        }


    }


    std::pair<vector<int16_t>, vector<int16_t> > inverseCoeffWin(vector<double> &data, size_t win) {
        size_t start_index = WIN_SIZE * win;
        vector<int16_t> winCoeff1{};
        vector<int16_t> winCoeff2{};
        for (int n = 0; n < WIN_SIZE; ++n) {
            double sum = 0.0;
            for (int k = 0; k < WIN_SIZE; ++k) {
                sum += data[start_index + k] * cosines[n * WIN_SIZE + k];
            }
            winCoeff1.push_back(static_cast<int16_t>(round(sum * (2.0 / WIN_SIZE) * weights[n])));
        }
        for (int n = WIN_SIZE; n < DOUBLE_WIN_SIZE; ++n) {
            double sum = 0.0;
            for (int k = 0; k < WIN_SIZE; ++k) {
                sum += data[start_index + k] * cosines[n * WIN_SIZE + k];
            }
            winCoeff2.push_back(static_cast<int16_t>(round(sum * (2.0 / WIN_SIZE) * weights[n])));
        }
        return {std::move(winCoeff1), std::move(winCoeff2)};
    }


    vector<int16_t> operator()(const vector<int32_t> &data) {
        auto transformed_view =
                data |
                std::ranges::views::transform(std::bind(std::multiplies(), placeholders::_1, quantizationValue_));
        vector<double> v{ranges::begin(transformed_view), ranges::end(transformed_view)};

        int nWin = data.size() / WIN_SIZE;
        vector<int16_t> coeff;
        vector<int16_t> overlapWin{WIN_SIZE, 0};

        for (int win = 0; win < nWin; ++win) {
            auto [invCoeff1, invCoeff2] = inverseCoeffWin(v, win);
            if (win > 0) {
                std::ranges::transform(overlapWin, std::move(invCoeff1), overlapWin.begin(), std::plus());
                coeff.insert(ranges::end(coeff), std::make_move_iterator(overlapWin.begin()),
                             std::make_move_iterator(overlapWin.end()));
            }
            overlapWin = std::move(invCoeff2);

        }

        coeff.insert(coeff.end(), std::make_move_iterator(overlapWin.begin()),
                     std::make_move_iterator(overlapWin.end() - WIN_SIZE));
        return coeff;
    }

};

void computeErrors() {
    auto originalData = read_bytes<int16_t>("/home/nicola/Desktop/data_processing/mdct/test.raw");
    auto reconstructedData = read_bytes<int16_t>("/home/nicola/Desktop/data_processing/mdct/reconstructed.raw");

    vector<int16_t> error(originalData.size());
    ranges::transform(originalData, reconstructedData, error.begin(), minus());

    ofstream os("../error.raw", ios::binary | ios::trunc);
    os.write(reinterpret_cast<const char *>(error.data()), error.size() * sizeof(int16_t));

}

int main(int argc, char **argv) {
    cout << "start\n";
    auto start = steady_clock::now();
    vector file = read_bytes<int16_t>("/home/nicola/Desktop/data_processing/mdct/test.raw");
    frequency<int16_t> statsOriginaFile{};
    statsOriginaFile.compute_freq(file);
    cout << "Entropy of the original file is : " << statsOriginaFile.compute_entropy() << endl;


    MDCT mdct{QTZ_VALUE};
    vector<int32_t> coeff = mdct(file);

    frequency<int32_t> stats{};
    stats.compute_freq(coeff);
    cout << "Entropy with quantization(" << QTZ_VALUE << ") is : " << stats.compute_entropy() << endl;

    IMDCT imdct{QTZ_VALUE};
    vector<int16_t> reconstructed_data = imdct(coeff);
    ofstream os(string{argv[3]}, ios::binary);
    os.write(reinterpret_cast<const char *>(reconstructed_data.data()),
             reconstructed_data.size() * sizeof(int16_t));

    auto stop = steady_clock::now();
    duration<double, std::milli> elapsed_ms = stop - start;
    cout << elapsed_ms.count() << "ms" << endl;

    computeErrors();

    return EXIT_SUCCESS;

}