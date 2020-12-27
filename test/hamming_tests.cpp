#include "gtest/gtest.h"

#include "hamming.h"

class hamming_tests : public ::testing::Test {
protected:
    void SetUp() override {
        generate_tables();
    }
};

static uint8_t flip_bit(uint8_t word, size_t bit_pos) {
    return word ^ (1u << bit_pos);
}

static void flip_random_bit(data_t &data) {
    static size_t bits_in_byte = 8;
    size_t bit_pos = rand() % (data.size() * bits_in_byte);
    uint8_t &byte = data[bit_pos / bits_in_byte];
    byte = flip_bit(byte, bit_pos % bits_in_byte);
}

TEST_F(hamming_tests, test_syndrome_vec_for_non_distorted_words) {
    for (uint8_t block = 0; block <= 0xF; ++block)
        EXPECT_EQ(0, compute_syndrome_vector(encode_block(block)));
}

TEST_F(hamming_tests, test_syndrome_vec_for_distorted_words) {
    for (uint8_t block = 0; block <= 0xF; ++block)
        for (size_t bit_pos = 0; bit_pos < 7; ++bit_pos)
            EXPECT_EQ(7 - bit_pos, compute_syndrome_vector(
                    flip_bit(encode_block(block), bit_pos)));
}

TEST_F(hamming_tests, test_block_non_distorted_encoding_and_decoding) {
    for (uint8_t block = 0; block <= 0xF; ++block)
        for (size_t bit_pos = 0; bit_pos < 7; ++bit_pos)
            EXPECT_EQ(block, decode_block(flip_bit(encode_block(block), bit_pos)));
}

TEST_F(hamming_tests, test_block_distorted_encoding_and_decoding) {
    for (uint8_t block = 0; block <= 0xF; ++block)
        EXPECT_EQ(block, decode_block(encode_block(block)));
}

TEST_F(hamming_tests, test_data_encoding_and_decoding) {
    data_t data {7, 100, 125, 200, 20, 50};
    ASSERT_EQ(data, decode_data(encode_data(data)));
}

static std::string strs[] = {"Push me and the just touch me",
                             "f", "aba", "", "Slim shady",
                             "0123456789012345678901234"};

TEST_F(hamming_tests, test_string_non_distorted_encoding_and_decoding) {
    for (const auto &str : strs)
        EXPECT_EQ(str, decode_string(encode_string(str)));
}

TEST_F(hamming_tests, test_string_distorted_encoding_and_decoding) {
    for (const auto &str : strs) {
        auto encoded = encode_string(str);
        if (!encoded.empty())
            flip_random_bit(encoded);
        EXPECT_EQ(str, decode_string(encoded));
    }
}