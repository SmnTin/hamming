#ifndef HAMMING_HAMMING_H
#define HAMMING_HAMMING_H

#include <cstdint>
#include <vector>
#include <string>

typedef std::vector<uint8_t> data_t;

void generate_tables();

uint8_t encode_block(uint8_t block);
uint8_t compute_syndrome_vector(uint8_t word);
uint8_t compute_error(uint8_t syndrome_vector);
uint8_t decode_block(uint8_t word);


data_t encode_data(const data_t &data);
data_t decode_data(const data_t &encoded);

data_t encode_string(const std::string &str);
std::string decode_string(const data_t &encoded);

#endif //HAMMING_HAMMING_H
