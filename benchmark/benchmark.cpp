#include "hamming.h"

#include <chrono>
#include <iostream>

int main() {
    srand(time(0));

    generate_tables();

    const size_t data_size = 1e7;
    data_t data(data_size);
    for (uint8_t &byte : data)
        byte = rand() % 256;

    auto start = std::chrono::high_resolution_clock::now();

    auto encoded = encode_data(data);
    auto decoded = decode_data(data);

    auto end = std::chrono::high_resolution_clock::now();

    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Encoded and decoded 10^7 bytes in " << millis << " ms." << std::endl;
}