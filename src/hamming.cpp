#include "hamming.h"

typedef uint8_t bit_t;

static const size_t block_size = 4;
static const size_t syn_vec_size = 3;
static const size_t word_size = 7;

static const size_t blocks_num = (1u << block_size);
static const size_t words_num = (1u << word_size);

// At the max level of the compiler optimizations
// loop is unrolled automatically.
static inline uint8_t pack_n_bits(const bit_t bits[], size_t n) {
    uint8_t result = 0;
    for (size_t i = 0; i < n; ++i)
        result |= bits[i] << i;
    return result;
}

static inline void unpack_n_bits(bit_t bits[], uint8_t packed, size_t n) {
    for (size_t i = 0; i < n; ++i)
        bits[i] = (packed >> i) & 1u;
}

static bit_t xor_bits_table[0x100];

static void gen_xor_bits_table() {
    for (unsigned i = 0; i <= 0xFF; ++i)
        xor_bits_table[i] = __builtin_parity(i);
}

static inline bit_t xor_bits(uint8_t word) {
    return xor_bits_table[word];
}

static uint8_t gen_block_encoding(uint8_t block) {
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
    static const uint8_t matrix[word_size] = {
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

static uint8_t block_encoding_table[blocks_num];

static void gen_block_encoding_table() {
    for (uint8_t block = 0; block < blocks_num; ++block)
        block_encoding_table[block] = gen_block_encoding(block);
}

uint8_t encode_block(uint8_t word) {
    return block_encoding_table[word];
}

static uint8_t gen_syndrome_vector(uint8_t word) {
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
    static const uint8_t matrix[word_size] = {
            0b1010101,
            0b0110011,
            0b0001111
    };

    bit_t unpacked[syn_vec_size];
    for (size_t i = 0; i < syn_vec_size; ++i)
        unpacked[i] = xor_bits(word & matrix[i]);

    return pack_n_bits(unpacked, syn_vec_size);
}

static uint8_t syndrome_vector_table[words_num];

static void gen_syndrome_vector_table() {
    for (uint8_t word = 0; word < words_num; ++word)
        syndrome_vector_table[word] = gen_syndrome_vector(word);
}

uint8_t compute_syndrome_vector(uint8_t word) {
    return syndrome_vector_table[word];
}

uint8_t compute_error(uint8_t syndrome_vector) {
    if (syndrome_vector == 0)
        return 0;
    return (1u << (word_size - syndrome_vector));
}

static uint8_t gen_word_decoding(uint8_t word) {
    uint8_t syndrome_vector = compute_syndrome_vector(word);
    uint8_t error = compute_error(syndrome_vector);

    uint8_t corrected = word ^error;

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

static uint8_t word_decoding_table[words_num];

static void gen_word_decoding_table() {
    for (uint8_t word = 0; word < words_num; ++word)
        word_decoding_table[word] = gen_word_decoding(word);
}

uint8_t decode_block(uint8_t word) {
    return word_decoding_table[word];
}

void generate_tables() {
    gen_xor_bits_table();
    gen_block_encoding_table();
    gen_syndrome_vector_table();
    gen_word_decoding_table();
}

static const size_t bits_in_byte = 8;

static inline void put_word(data_t &data, size_t &bit_shift, uint8_t word) {
    if (bit_shift == 0) {
        data.push_back(word);
        bit_shift = word_size;
    } else {
        data.back() |= (word << bit_shift);
        if (bit_shift + word_size > bits_in_byte)
            data.push_back(word >> (bits_in_byte - bit_shift));
        bit_shift += word_size;
        bit_shift %= bits_in_byte;
    }
}

data_t encode_data(const data_t &data) {
    data_t encoded;

    size_t bit_shift = 0;
    for (uint8_t byte : data) {
        uint8_t block1 = byte >> block_size;
        uint8_t block2 = byte & 0xF;

        put_word(encoded, bit_shift, encode_block(block1));
        put_word(encoded, bit_shift, encode_block(block2));
    }

    return encoded;
}

static inline uint8_t get_word(const data_t &data, size_t &byte_shift, size_t &bit_shift) {
    uint8_t word = data[byte_shift] >> bit_shift;
    if (bit_shift + word_size > bits_in_byte)
        word |= data[byte_shift + 1] << (bits_in_byte - bit_shift);

    byte_shift += (bit_shift + word_size) / bits_in_byte;
    bit_shift = (bit_shift + word_size) % bits_in_byte;
    word = word & (words_num - 1);

    return word;
}

static inline uint8_t merge_blocks(uint8_t block1, uint8_t block2) {
    return ((block1 << block_size) | block2) & UINT8_MAX;
}

/**
 * You may notice that when we put (7 * 8 - 1)
 * 7-bit words into 8-bit bytes, after parsing
 * we get 7 * 8 words. But this is never a case
 * because (7 * 8 - 1) is odd. Each byte
 * of original message is split into two blocks
 * thus number of resulting words is even.
 * So if [encoded] is a result of [encode_data]
 * we will never run into such a problem.
 */
data_t decode_data(const data_t &encoded) {
    data_t words;

    size_t byte_shift = 0;
    size_t bit_shift = 0;

    // Reserve a bit more than necessary
    words.reserve(encoded.size());

    while (byte_shift + (bit_shift + word_size - 1) / bits_in_byte < encoded.size())
        words.push_back(get_word(encoded, byte_shift, bit_shift));

    data_t decoded;
    decoded.reserve(words.size() / 2);
    for (size_t i = 0; i < words.size(); i += 2) {
        uint8_t block1 = decode_block(words[i]);
        uint8_t block2 = decode_block(words[i + 1]);

        decoded.push_back(merge_blocks(block1, block2));
    }

    return decoded;
}

data_t encode_string(const std::string &str) {
    auto *raw_data = reinterpret_cast<const uint8_t *>(str.data());
    data_t data(raw_data, raw_data + str.size());
    return encode_data(data);
}

std::string decode_string(const data_t &encoded) {
    data_t decoded = decode_data(encoded);
    if (decoded.empty()) {
        return std::string();
    } else {
        auto *raw_str = reinterpret_cast<const char *>(decoded.data());
        // Terminating null byte is not preserved
        return std::string(raw_str, raw_str + decoded.size());
    }
}
