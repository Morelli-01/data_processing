//
// Created by nicola on 17/05/2024.
//

#include "cstdlib"
#include "cstdint"
#include "vector"
#include "string_view"
#include "iostream"
#include "fstream"
#include "ranges"
#include "functional"
#include "algorithm"
#include "error.h"

using namespace std;

template<typename T>
vector<T> readBytes(const string &file_name) {
    ifstream is(file_name, ios::binary);
    is.seekg(0, ios::end);
    streamsize ss = is.tellg();
    is.seekg(0, ios::beg);
    vector<T> data(ss / sizeof(T));
    is.read(reinterpret_cast<char *>(data.data()), ss);
    is.close();
    return data;
}

void computeErrors(){
    auto originalData = readBytes<int16_t>("../test.raw");
    auto reconstructedData = readBytes<int16_t>("../reconstructed.raw");

    vector<int16_t> error(originalData.size());
    ranges::transform(originalData, reconstructedData, error.begin(), minus());

    ofstream os("../error.raw", ios::binary | ios::trunc);
    os.write(reinterpret_cast<const char *>(error.data()), error.size() * sizeof(int16_t));

}

