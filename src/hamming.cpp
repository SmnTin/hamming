#include "hamming.h"

#include <cstddef>

typedef uint8_t bit_t;

static const size_t block_size = 4;
static const size_t syn_vec_size = 3;
static const size_t word_size = 7;

static const size_t blocks_num = (1u << block_size);
static const size_t syn_vecs_num = (1u << syn_vec_size);
static const size_t words_num = (1u << word_size);

// At the max level of the compiler optimizations
// loop is unrolled automatically.
static inline word_t pack_n_bits(const bit_t bits[], size_t n) {
    word_t result = 0;
    for (size_t i = 0; i < n; ++i)
        result |= bits[i] << i;
    return result;
}

static inline void unpack_n_bits(bit_t bits[], word_t packed, size_t n) {
    for (size_t i = 0; i < n; ++i)
        bits[i] = (packed >> i) & 1u;
}

static bit_t xor_bits_table[0x100];

static void gen_xor_bits_table() {
    for (unsigned i = 0; i <= 0xFF; ++i)
        xor_bits_table[i] = __builtin_parity(i);
}

static inline bit_t xor_bits(word_t word) {
    return xor_bits_table[word];
}

static word_t gen_block_encoding(word_t block) {
    /**
     * Matrix multiplication over a field of 2:
     *
     * Input:
     *
     *     (*)
     * p = (*)
     *     (*)
     *     (*)
     *
     *     (1 0 1 1)
     *     (1 1 0 1)
     *     (0 0 0 1)
     * G = (1 1 1 0)
     *     (0 0 1 0)
     *     (0 1 0 0)
     *     (1 0 0 0)
     *
     * Result = G * p
     */

    // Reversed order due to bit packing
    static const word_t matrix[word_size] = {
        0b1000,
        0b0100,
        0b0010,
        0b1110,
        0b0001,
        0b1101,
        0b1011
    };

    bit_t unpacked[word_size];

    for (size_t i = 0; i < word_size; ++i)
        unpacked[i] = xor_bits(block & matrix[i]);

    return pack_n_bits(unpacked, word_size);
}

static word_t block_encoding_table[blocks_num];

static void gen_block_encoding_table() {
    for (uint8_t block = 0; block < blocks_num; ++block)
        block_encoding_table[block] = gen_block_encoding(block);
}

word_t encode_block(word_t word) {
    return block_encoding_table[word];
}

static word_t gen_syndrome_vector(word_t word) {
    /**
     * Matrix multiplication over a field of 2:
     *
     *     (1 0 1 0 1 0 1)
     * H = (0 1 1 0 0 1 1)
     *     (0 0 0 1 1 1 1)
     *
     * Result = H * p
     */

    // Original order preserved
    static const word_t matrix[word_size] = {
            0b1010101,
            0b0110011,
            0b0001111
    };

    bit_t unpacked[syn_vec_size];
    for (size_t i = 0; i < syn_vec_size; ++i)
        unpacked[i] = xor_bits(word & matrix[i]);

    return pack_n_bits(unpacked, syn_vec_size);
}

static word_t syndrome_vector_table[words_num];

static void gen_syndrome_vector_table() {
    for (uint8_t word = 0; word < words_num; ++word)
        syndrome_vector_table[word] = gen_syndrome_vector(word);
}

word_t compute_syndrome_vector(word_t word) {
    return syndrome_vector_table[word];
}

word_t compute_error(word_t syndrome_vector) {
    if (syndrome_vector == 0)
        return 0;
    return (1u << (word_size - syndrome_vector));
}

static word_t gen_word_decoding(word_t word) {
    word_t syndrome_vector = compute_syndrome_vector(word);
    word_t error = compute_error(syndrome_vector);

    word_t corrected = word ^ error;

    bit_t unpacked_word[word_size];
    unpack_n_bits(unpacked_word, corrected, word_size);

    /**
     * Note that 4-th, 2-nd, 1-st and 0-th rows
     * of G form an identity matrix.
     *
     *     (1 0 1 1) | 6
     *     (1 1 0 1) | 5
     *     (0 0 0 1) | 4
     * G = (1 1 1 0) | 3
     *     (0 0 1 0) | 2
     *     (0 1 0 0) | 1
     *     (1 0 0 0) | 0
     */
    bit_t unpacked_block[block_size];
    unpacked_block[0] = unpacked_word[4];
    unpacked_block[1] = unpacked_word[2];
    unpacked_block[2] = unpacked_word[1];
    unpacked_block[3] = unpacked_word[0];

    return pack_n_bits(unpacked_block, block_size);
}

static word_t word_decoding_table[words_num];

static void gen_word_decoding_table() {
    for (word_t word = 0; word < words_num; ++word)
        word_decoding_table[word] = gen_word_decoding(word);
}

word_t decode_block(word_t word) {
    return word_decoding_table[word];
}

void generate_tables() {
    gen_xor_bits_table();
    gen_block_encoding_table();
    gen_syndrome_vector_table();
    gen_word_decoding_table();
}
