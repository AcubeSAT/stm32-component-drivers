#include "HAL_TRNG.h"
#include "device.h"

/**
 * TRNG peripheral ID is 57 (datasheet Table 56-1). PMC_PCER1 only covers
 * peripheral IDs 32-63, so the enable bit is (57 - 32) = bit 25.
 */
static constexpr uint32_t TrngPeripheralId = 57U;
static constexpr uint32_t PmcPcer1FirstPeripheralId = 32U;
static constexpr uint32_t TrngPmcPcer1Bit = TrngPeripheralId - PmcPcer1FirstPeripheralId;

/**
 * The TRNG produces a new value every 84 peripheral clock cycles (datasheet
 * Section 56.5). Worst case, MCK is running at its 12 MHz reset default
 * (Section 31.1), so a value is guaranteed within ~7us. 2,000,000 polling
 * iterations is a big margin on top of that - still ~1000x the worst case
 * even assuming the device's fastest clock (300 MHz) and an unrealistically
 * tight 1-cycle loop body - enough to absorb bus wait states, jitter, and
 * whatever overhead the compiler ends up generating for the loop.
 */
static constexpr uint32_t TrngTimeoutIterations = 2000000U;

void trngEnable() {
    PMC_REGS->PMC_PCER1 = (1U << TrngPmcPcer1Bit);
    TRNG_REGS->TRNG_CR = TRNG_CR_ENABLE_Msk | TRNG_CR_KEY_PASSWD;
}

TrngError trngWaitReady() {
    for (uint32_t i = 0; i < TrngTimeoutIterations; i++) {
        if ((TRNG_REGS->TRNG_ISR & TRNG_ISR_DATRDY_Msk) != 0) {
            return TrngError::OK;
        }
    }
    return TrngError::TIMEOUT;
}

uint32_t trngRead() {
    return TRNG_REGS->TRNG_ODATA;
}

void trngDisable() {
    TRNG_REGS->TRNG_CR = TRNG_CR_KEY_PASSWD;
}

TrngResult trngGenerate() {
    trngEnable();

    const TrngError err = trngWaitReady();
    if (err != TrngError::OK) {
        trngDisable();
        return TrngResult{err, 0};
    }

    const uint32_t value = trngRead();
    trngDisable();
    return TrngResult{TrngError::OK, value};
}