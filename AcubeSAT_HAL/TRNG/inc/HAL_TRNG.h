#pragma once

#include <stdint.h>

/**
 * @brief Error/status codes returned by the TRNG HAL.
 */
enum class TrngError : uint8_t {
    OK = 0,     /**< Success. */
    TIMEOUT,    /**< No value became ready before trngWaitReady() gave up. */
};

/**
 * @brief Result of a trngGenerate() call.
 *
 * `value` is only meaningful when `error` is TrngError::OK.
 */
struct TrngResult {
    TrngError error;
    uint32_t  value;
};

/**
 * @brief Enable the TRNG peripheral clock and start the generator.
 *
 * Turns on the TRNG's PMC peripheral clock and writes the enable key to
 * TRNG_CR. Once enabled, the TRNG produces a new 32-bit value every 84
 * peripheral clock cycles (datasheet Section 56.5).
 */
void trngEnable();

/**
 * @brief Block until a random value is ready, or time out.
 *
 * Polls TRNG_ISR.DATRDY in a busy loop. See TrngTimeoutIterations in
 * HAL_TRNG.cpp for how the timeout is sized.
 *
 * @return TrngError::OK once a value is ready, TrngError::TIMEOUT otherwise.
 */
TrngError trngWaitReady();

/**
 * @brief Read the most recently generated random value.
 *
 * Only call this after trngWaitReady() returns TrngError::OK.
 *
 * @return The 32-bit value read from TRNG_ODATA.
 */
uint32_t trngRead();

/**
 * @brief Stop the generator by clearing TRNG_CR.ENABLE.
 *
 * Note: this doesn't gate the peripheral clock back off (that would be
 * PMC_PCDR1) - only the ENABLE bit set by trngEnable() is cleared.
 */
void trngDisable();

/**
 * @brief Generate a single random value: enable, wait, read, disable.
 *
 * @return TrngResult{OK, value} on success, or TrngResult{TIMEOUT, 0} if
 *         the TRNG didn't become ready in time.
 */
TrngResult trngGenerate();