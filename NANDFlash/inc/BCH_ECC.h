#pragma once

#include <etl/array.h>
#include <etl/span.h>
#include <etl/expected.h>
#include <cstdint>

/**
 * @brief BCH(1023, 983, 4) Error Correction Code
 *
 * @details This class implements a BCH error correction code with the following parameters:
 *          - n = 1023 bits (codeword length)
 *          - k = 983 bits (data length)
 *          - t = 4 (error correction capability - corrects up to 4 bit errors)
 *          - Galois Field: GF(2^10)
 *          - Primitive Polynomial: x^10 + x^3 + 1 = 0x409
 *
 * @note It implements the Berlekamp-Massey algorithm for error locator polynomial
 *       computation and Chien search for error position finding.
 *       For NAND flash integration:
 *          - 68 codewords per 8KB page (122 bytes data + 5 bytes parity each)
 *          - Total ECC storage: 340 bytes
 */
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
    static constexpr uint8_t CODEWORDS_PER_PAGE = 68;
    static constexpr uint16_t TOTAL_ECC_BYTES = 340;

    /**
     * @brief Error codes returned by BCH operations
     */
    enum class BCHError : uint8_t {
        SUCCESS = 0,                /*!< Operation completed successfully */
        TOO_MANY_ERRORS,            /*!< More than 4 errors detected (uncorrectable) */
        INVALID_PARAMETER,          /*!< Invalid input parameters */
        LOCATOR_ERROR               /*!< Error in error locator polynomial computation */
    };

    /**
     * @brief Initialize BCH codec lookup tables
     *
     * @note Must be called once before any encode/decode operations.
     *       Generates logarithm and antilogarithm tables for GF(2^10) arithmetic
     *       and builds the generator polynomial for BCH(1023, 983, 4).
     *
     * @return Success or error code
     * @retval BCHError::SUCCESS Initialization completed successfully
     */
    static etl::expected<void, BCHError> initialize();

    /**
     * @brief Encode data using BCH(1023, 983, 4) systematic encoding
     *
     * @param data Input data span (must be exactly 122 bytes)
     * @param parity Output parity span (must be exactly 5 bytes)
     *
     * @return Success or error code
     * @retval BCHError::SUCCESS Encoding completed successfully
     * @retval BCHError::INVALID_PARAMETER Invalid input size
     */
    static etl::expected<void, BCHError> encode(etl::span<const uint8_t> data,
                                                etl::span<uint8_t> parity);

    /**
     * @brief Decode and correct errors in BCH-encoded data
     *
     * @param data Input/output data span (must be exactly 122 bytes)
     * @param parity Input parity span (must be exactly 5 bytes)
     *
     * @return Number of corrected errors or error code
     * @retval 0-4 Number of bit errors that were successfully corrected
     * @retval BCHError::TOO_MANY_ERRORS More than 4 errors detected
     * @retval BCHError::INVALID_PARAMETER Invalid input size
     * @retval BCHError::LOCATOR_ERROR Error in syndrome computation
     */
    static etl::expected<uint8_t, BCHError> decode(etl::span<uint8_t> data,
                                                   etl::span<const uint8_t> parity);

private:
    /**
     * @brief Galois Field addition (XOR operation)
     * 
     * @param a First operand
     * @param b Second operand
     * 
     * @return Sum in GF(2^10)
     */
    static constexpr uint16_t gfAdd(uint16_t a, uint16_t b) {
        return a ^ b;
    }

    /**
     * @brief Galois Field multiplication using lookup tables
     * 
     * @param a First operand
     * @param b Second operand
     * 
     * @return Product in GF(2^10)
     */
    static uint16_t gfMul(uint16_t a, uint16_t b);

    /**
     * @brief Galois Field division using lookup tables
     * 
     * @param a Dividend
     * @param b Divisor (must not be zero)
     * 
     * @return Quotient in GF(2^10)
     */
    static uint16_t gfDiv(uint16_t a, uint16_t b);

    /**
     * @brief Galois Field multiplicative inverse
     * 
     * @param a Input value (must not be zero)
     * 
     * @return Multiplicative inverse in GF(2^10)
     */
    static uint16_t gfInv(uint16_t a);

    /**
     * @brief Galois Field exponentiation
     * 
     * @param base Base value
     * @param exp Exponent
     * 
     * @return base^exp in GF(2^10)
     */
    static uint16_t gfPow(uint16_t base, uint16_t exp);

    /**
     * @brief Build BCH generator polynomial from minimal polynomials
     *
     * @details Constructs the generator polynomial g(x) by computing the LCM of
     *          minimal polynomials for roots α^1, α^2, ..., α^8 in GF(2^10).
     *          The resulting polynomial has degree 40 for BCH(1023, 983, 4).
     */
    static void buildGeneratorPoly();

    /**
     * @brief Polynomial division with remainder for systematic encoding
     * 
     * @param data Input data polynomial (983 bits zero-padded)
     * @param remainder Output remainder polynomial (40 bits parity)
     */
    static void polyDivMod(etl::span<const uint8_t> data, etl::span<uint8_t> remainder);

    /**
     * @brief Get bit value at specified position in byte array
     * 
     * @param data Byte array
     * @param bitPos Bit position (MSB first indexing)
     * 
     * @return Bit value (0 or 1)
     */
    static inline uint8_t getBit(etl::span<const uint8_t> data, uint16_t bitPos) {
        return (data[bitPos / 8] >> (7 - bitPos % 8)) & 1;
    }

    /**
     * @brief Set bit to 1 at specified position in byte array
     * 
     * @param data Byte array
     * @param bitPos Bit position (MSB first indexing)
     */
    static inline void setBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] |= (1 << (7 - bitPos % 8));
    }

    /**
     * @brief Clear bit to 0 at specified position in byte array
     * 
     * @param data Byte array
     * @param bitPos Bit position (MSB first indexing)
     */
    static inline void clearBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] &= ~(1 << (7 - bitPos % 8));
    }

    /**
     * @brief Toggle bit at specified position in byte array
     * 
     * @param data Byte array
     * @param bitPos Bit position (MSB first indexing)
     */
    static inline void toggleBit(etl::span<uint8_t> data, uint16_t bitPos) {
        data[bitPos / 8] ^= (1 << (7 - bitPos % 8));
    }

    /**
     * @brief Calculate syndrome polynomial for error detection
     *
     * @details Evaluates the received polynomial (data + parity) at the roots
     *          α^1, α^2, ..., α^8 to generate syndrome components. Non-zero
     *          syndrome indicates presence of errors.
     *
     * @param data Received data (122 bytes)
     * @param parity Received parity (5 bytes)
     * @param syndrome Output syndrome array (8 elements)
     */
    static void calculateSyndrome(etl::span<const uint8_t> data,
                                  etl::span<const uint8_t> parity,
                                  etl::span<uint16_t> syndrome);

    /**
     * @brief Compute error locator polynomial using Berlekamp-Massey algorithm
     *
     * @param syndrome Input syndrome array (8 elements)
     * @param errorLocator Output error locator polynomial coefficients
     * 
     * @return Degree of error locator polynomial (number of errors detected)
     */
    static uint8_t berlekampMassey(etl::span<const uint16_t> syndrome,
                                   etl::span<uint16_t> errorLocator);

    /**
     * @brief Find error positions using Chien search
     *
     * @param errorLocator Error locator polynomial coefficients
     * @param degree Degree of error locator polynomial
     * @param errorPositions Output array of error positions
     * 
     * @return Number of error positions found
     */
    static uint8_t chienSearch(etl::span<const uint16_t> errorLocator,
                              uint8_t degree,
                              etl::span<uint16_t> errorPositions);

    /**
     * @brief Correct errors at specified positions
     *
     * @param data Data to be corrected (modified in-place)
     * @param positions Array of error positions to correct
     * @param numErrors Number of errors to correct
     */
    static void correctErrors(etl::span<uint8_t> data,
                              etl::span<const uint16_t> positions,
                              uint8_t numErrors);

    static etl::array<uint16_t, GF_SIZE + 1> logTable; /*!<  Logarithm lookup table for GF(2^10) multiplication */

    static etl::array<uint16_t, GF_SIZE + 1> antilogTable; /*!< Antilogarithm lookup table for GF(2^10) multiplication */

    static etl::array<uint16_t, PARITY_BITS + 1> generatorPoly; /*!< BCH generator polynomial coefficients (degree 40) */

    static uint8_t generatorDegree; /*!< Actual degree of generator polynomial */

    static bool initialized; /*!< Initialization state flag */
};