#include <iostream>
#include <fstream>
#include "cmath"
#include "algorithm"
#include "vector"
#include "cstdint"

using namespace std;

int main() {
    int duration = 3000;//ms
    int f = 441; //Hz
    int f2 = 550; //Hz
    double A = pow(2, 15) - 1;
    int sampling_freq = 44100;

    vector<int16_t> sample(sampling_freq * duration / 1000);
    for (int i = 0; i < sample.size(); ++i) {
        double t = static_cast<double>(i) / sampling_freq;
        sample[i] = A * sin(2 * M_PI * f2 * t);

    }

    ofstream os("/home/nicola/Desktop/data_processing/audio_intro/raw_audio.raw", ios::binary | ios::trunc);
    os.write(reinterpret_cast<const char *>(sample.data()), sample.size() * 2);

}
