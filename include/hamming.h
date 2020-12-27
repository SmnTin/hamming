#ifndef HAMMING_HAMMING_H
#define HAMMING_HAMMING_H

#include <cstdint>

typedef uint8_t word_t;

void generate_tables();

word_t encode_block(word_t block);
word_t compute_syndrome_vector(word_t word);
word_t compute_error(word_t syndrome_vector);
word_t decode_block(word_t word);

#endif //HAMMING_HAMMING_H
