#include "HAL_TRNG.h"
#include "device.h"       // pulls in Trng.h via the correct inclusion chain

static constexpr uint32_t TrngTimeoutIterations = 1000000;

void trngEnable() {
    PMC_REGS->PMC_PCER1 = (1U << (57U - 32U));
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