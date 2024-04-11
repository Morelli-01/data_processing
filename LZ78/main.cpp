#include <iostream>
#include <fstream>

using namespace std;

bool lz78encode(const std::string &input_filename, const std::string &output_filename, int maxbits) {
    if (maxbits < 1 or maxbits > 30) {
        return false;
    }
    ifstream is(input_filename, ios::binary);
    ifstream os(output_filename, ios::binary);
    if (is.fail() or os.fail()) {
        return false;
    }

    return true;
}


int main() {
    return lz78encode("/home/nicola/Desktop/data_processing/LZ78/test1.txt",
                      "/home/nicola/Desktop/data_processing/LZ78/output_test1.bin", 30);

}
