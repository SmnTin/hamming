#include "gtest/gtest.h"

#include "hamming.h"

class hamming_tests : public ::testing::Test {
protected:
    void SetUp() override {
        generate_tables();
    }
};

static word_t flip_bit(word_t word, size_t bit_pos) {
    return word ^ (1u << bit_pos);
}

TEST_F(hamming_tests, test_syndrome_vec_for_non_distorted_words) {
    for (word_t block = 0; block <= 0xF; ++block)
        EXPECT_EQ(0, compute_syndrome_vector(encode_block(block)));
}

TEST_F(hamming_tests, test_syndrome_vec_for_distorted_words) {
    for (word_t block = 0; block <= 0xF; ++block)
        for (size_t bit_pos = 0; bit_pos < 7; ++bit_pos)
            EXPECT_EQ(7 - bit_pos, compute_syndrome_vector(
                    flip_bit(encode_block(block), bit_pos)));
}

TEST_F(hamming_tests, test_block_non_distorted_encoding_and_decoding) {
    for (word_t block = 0; block <= 0xF; ++block)
        for (size_t bit_pos = 0; bit_pos < 7; ++bit_pos)
            EXPECT_EQ(block, decode_block(flip_bit(encode_block(block), bit_pos)));
}

TEST_F(hamming_tests, test_block_distorted_encoding_and_decoding) {
    for (word_t block = 0; block <= 0xF; ++block)
        EXPECT_EQ(block, decode_block(encode_block(block)));
}