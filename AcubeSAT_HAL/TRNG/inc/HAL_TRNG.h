#pragma once

#include <stdint.h>

enum class TrngError : uint32_t {
    OK = 0,
    TIMEOUT,
};

struct TrngResult {
    TrngError error;
    uint32_t  value;
};

TrngResult trngGenerate();
void trngEnable();
TrngError trngWaitReady();
uint32_t trngRead();
void trngDisable();