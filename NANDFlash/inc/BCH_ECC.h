#pragma once

#include <etl/array.h>
#include <etl/span.h>
#include <etl/expected.h>
#include <cstdint>

class BCH_ECC {
public:
    static constexpr uint16_t GF_BITS = 10;
    static constexpr uint16_t GF_SIZE = 1023;
    static constexpr uint16_t CODEWORD_BITS = 1023;
    static constexpr uint16_t DATA_BITS = 983;
    static constexpr uint16_t PARITY_BITS = 40;
    static constexpr uint8_t ERROR_CAPABILITY = 4;
    static constexpr uint16_t PRIMITIVE_POLY = 0x409;

    static constexpr uint16_t DATA_BYTES_PER_CODEWORD = 122;
    static constexpr uint16_t PARITY_BYTES_PER_CODEWORD = 5;
    static constexpr uint8_t CODEWORDS_PER_PAGE = 67;
    static constexpr uint16_t TOTAL_ECC_BYTES = 335;

    enum class BCHError : uint8_t {
        SUCCESS = 0,
        TOO_MANY_ERRORS,
        INVALID_PARAMETER,
        LOCATOR_ERROR
    };

    static etl::expected<void, BCHError> initialize();

    static etl::expected<void, BCHError> encode(etl::span<const uint8_t> data,
                                                etl::span<uint8_t> parity);

    static etl::expected<uint8_t, BCHError> decode(etl::span<uint8_t> data,
                                                   etl::span<const uint8_t> parity);

private:
    static constexpr uint16_t gfAdd(uint16_t a, uint16_t b) {
        return a ^ b;
    }

    static uint16_t gfMul(uint16_t a, uint16_t b);
    static uint16_t gfDiv(uint16_t a, uint16_t b);
    static uint16_t gfInv(uint16_t a);
    static uint16_t gfPow(uint16_t base, uint16_t exp);

    static void buildGeneratorPoly();

    static void polyDivMod(etl::span<const uint8_t> data, etl::span<uint8_t> remainder);

    static inline uint8_t getBit(etl::span<const uint8_t> data, uint16_t bitPos) {
        return (data[bitPos / 8] >> (7 - bitPos % 8)) & 1;
    }

    static inline void setBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] |= (1 << (7 - bitPos % 8));
    }

    static inline void clearBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] &= ~(1 << (7 - bitPos % 8));
    }

    static inline void toggleBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] ^= (1 << (7 - bitPos % 8));
    }

    static void calculateSyndrome(etl::span<const uint8_t> data,
                                  etl::span<const uint8_t> parity,
                                  etl::span<uint16_t> syndrome);

    static uint8_t berlekampMassey(etl::span<const uint16_t> syndrome,
                                   etl::span<uint16_t> errorLocator);

    static uint8_t chienSearch(etl::span<const uint16_t> errorLocator,
                              uint8_t degree,
                              etl::span<uint16_t> errorPositions);

    static void correctErrors(etl::span<uint8_t> data,
                              etl::span<const uint16_t> positions,
                              uint8_t numErrors);

    static etl::array<uint16_t, GF_SIZE + 1> logTable;
    static etl::array<uint16_t, GF_SIZE + 1> antilogTable;
    static etl::array<uint16_t, PARITY_BITS + 1> generatorPoly;
    static uint8_t generatorDegree;
    static bool initialized;
};