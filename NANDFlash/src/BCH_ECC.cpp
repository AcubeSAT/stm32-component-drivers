#include "BCH_ECC.h"
#include <cstring>

etl::array<uint16_t, BCH_ECC::GF_SIZE + 1> BCH_ECC::logTable;
etl::array<uint16_t, BCH_ECC::GF_SIZE + 1> BCH_ECC::antilogTable;
etl::array<uint16_t, BCH_ECC::PARITY_BITS + 1> BCH_ECC::generatorPoly;
uint8_t BCH_ECC::generatorDegree = 0;
bool BCH_ECC::initialized = false;

etl::expected<void, BCH_ECC::BCHError> BCH_ECC::initialize() {
    if (initialized) {
        return {};
    }

    uint16_t alpha = 1;
    for (uint16_t i = 0; i < GF_SIZE; ++i) {
        antilogTable[i] = alpha;
        logTable[alpha] = i;

        alpha <<= 1;
        if (alpha & (1 << GF_BITS)) {
            alpha ^= PRIMITIVE_POLY;
        }
    }

    logTable[0] = GF_SIZE;
    antilogTable[GF_SIZE] = 0;

    buildGeneratorPoly();

    initialized = true;
    return {};
}

void BCH_ECC::buildGeneratorPoly() {
    generatorPoly.fill(0);
    generatorPoly[0] = 1;
    generatorDegree = 0;

    etl::array<bool, GF_SIZE> includedRoots;
    includedRoots.fill(false);

    for (uint8_t i = 1; i <= 2 * ERROR_CAPABILITY; ++i) {
        if (includedRoots[i]) continue;

        etl::array<uint16_t, GF_BITS + 1> minPoly;
        minPoly.fill(0);
        minPoly[0] = 1;
        uint8_t minPolyDegree = 0;

        uint16_t root = i;
        for (uint8_t j = 0; j < GF_BITS; ++j) {
            if (root >= GF_SIZE) break;

            includedRoots[root] = true;

            etl::array<uint16_t, GF_BITS + 1> temp = minPoly;
            for (int k = minPolyDegree; k >= 0; --k) {
                if (temp[k] != 0) {
                    uint16_t alpha_root = antilogTable[root];
                    minPoly[k + 1] = gfAdd(minPoly[k + 1], temp[k]);
                    minPoly[k] = gfMul(temp[k], alpha_root);
                }
            }
            minPolyDegree++;

            root = (root * 2) % GF_SIZE;
        }

        etl::array<uint16_t, PARITY_BITS + 1> newGen;
        newGen.fill(0);

        for (uint8_t a = 0; a <= generatorDegree; ++a) {
            for (uint8_t b = 0; b <= minPolyDegree; ++b) {
                if (generatorPoly[a] != 0 && minPoly[b] != 0) {
                    newGen[a + b] = gfAdd(newGen[a + b], gfMul(generatorPoly[a], minPoly[b]));
                }
            }
        }

        generatorDegree += minPolyDegree;
        for (uint8_t idx = 0; idx <= generatorDegree; ++idx) {
            generatorPoly[idx] = newGen[idx];
        }
    }

    for (uint8_t i = 1; i <= 2 * ERROR_CAPABILITY; ++i) {
        uint16_t alpha_i = antilogTable[i];
        uint16_t eval = 0;
        for (uint8_t j = 0; j <= generatorDegree; ++j) {
            uint16_t term = gfMul(generatorPoly[j], gfPow(alpha_i, j));
            eval = gfAdd(eval, term);
        }
    }
}

uint16_t BCH_ECC::gfMul(uint16_t a, uint16_t b) {
    if (a == 0 || b == 0) return 0;
    uint16_t logSum = (logTable[a] + logTable[b]);
    if (logSum >= GF_SIZE) logSum -= GF_SIZE;
    return antilogTable[logSum];
}

uint16_t BCH_ECC::gfDiv(uint16_t a, uint16_t b) {
    if (b == 0) return 0;
    if (a == 0) return 0;
    int16_t logDiff = static_cast<int16_t>(logTable[a]) - static_cast<int16_t>(logTable[b]);
    if (logDiff < 0) logDiff += GF_SIZE;
    return antilogTable[logDiff];
}

uint16_t BCH_ECC::gfInv(uint16_t a) {
    if (a == 0) return 0;
    return antilogTable[GF_SIZE - logTable[a]];
}

uint16_t BCH_ECC::gfPow(uint16_t base, uint16_t exp) {
    if (base == 0) return 0;
    if (exp == 0) return 1;
    uint32_t logResult = (static_cast<uint32_t>(logTable[base]) * exp) % GF_SIZE;
    return antilogTable[logResult];
}

etl::expected<void, BCH_ECC::BCHError> BCH_ECC::encode(etl::span<const uint8_t> data,
                                                       etl::span<uint8_t> parity) {
    if (!initialized) {
        auto init_result = initialize();
        if (!init_result) {
            return etl::unexpected(init_result.error());
        }
    }

    if (data.size() != DATA_BYTES_PER_CODEWORD || parity.size() != PARITY_BYTES_PER_CODEWORD) {
        return etl::unexpected(BCHError::INVALID_PARAMETER);
    }

    etl::array<uint8_t, 123> paddedData;
    std::memcpy(paddedData.data(), data.data(), DATA_BYTES_PER_CODEWORD);
    paddedData[DATA_BYTES_PER_CODEWORD] = 0;

    std::memset(parity.data(), 0, parity.size());
    polyDivMod(etl::span<const uint8_t>(paddedData.data(), 123), parity);

    return {};
}

void BCH_ECC::polyDivMod(etl::span<const uint8_t> data, etl::span<uint8_t> remainder) {
    etl::array<uint8_t, 128> shifted;
    shifted.fill(0);

    for (size_t i = 0; i < data.size() * 8; ++i) {
        if (getBit(data, i)) {
            setBit(etl::span<uint8_t>(shifted), i);
        }
    }

    for (int i = data.size() * 8 - 1; i >= static_cast<int>(PARITY_BITS); --i) {
        if (getBit(etl::span<const uint8_t>(shifted), i)) {
            for (uint8_t j = 0; j <= generatorDegree; ++j) {
                if (generatorPoly[j] != 0) {
                    toggleBit(etl::span<uint8_t>(shifted), i - PARITY_BITS + j);
                }
            }
        }
    }

    for (uint8_t i = 0; i < PARITY_BITS; ++i) {
        if (getBit(etl::span<const uint8_t>(shifted), i)) {
            setBit(remainder, i);
        }
    }
}

etl::expected<uint8_t, BCH_ECC::BCHError> BCH_ECC::decode(etl::span<uint8_t> data,
                                                          etl::span<const uint8_t> parity) {
    if (!initialized) {
        auto init_result = initialize();
        if (!init_result) {
            return etl::unexpected(init_result.error());
        }
    }

    if (data.size() != DATA_BYTES_PER_CODEWORD || parity.size() != PARITY_BYTES_PER_CODEWORD) {
        return etl::unexpected(BCHError::INVALID_PARAMETER);
    }

    etl::array<uint16_t, 2 * ERROR_CAPABILITY> syndrome;
    calculateSyndrome(data, parity, syndrome);

    bool hasErrors = false;
    for (uint8_t i = 0; i < 2 * ERROR_CAPABILITY; ++i) {
        if (syndrome[i] != 0) {
            hasErrors = true;
            break;
        }
    }

    if (!hasErrors) {
        return 0;
    }

    etl::array<uint16_t, ERROR_CAPABILITY + 1> errorLocator;
    errorLocator.fill(0);
    uint8_t degree = berlekampMassey(syndrome, errorLocator);

    if (degree > ERROR_CAPABILITY) {
        return etl::unexpected(BCHError::TOO_MANY_ERRORS);
    }

    etl::array<uint16_t, ERROR_CAPABILITY> errorPositions;
    uint8_t numErrors = chienSearch(errorLocator, degree, errorPositions);

    if (numErrors != degree) {
        return etl::unexpected(BCHError::LOCATOR_ERROR);
    }

    correctErrors(data, etl::span<const uint16_t>(errorPositions.data(), numErrors), numErrors);

    return numErrors;
}

void BCH_ECC::calculateSyndrome(etl::span<const uint8_t> data,
                                etl::span<const uint8_t> parity,
                                etl::span<uint16_t> syndrome) {
    etl::array<uint8_t, 123> paddedData;
    std::memcpy(paddedData.data(), data.data(), DATA_BYTES_PER_CODEWORD);
    paddedData[DATA_BYTES_PER_CODEWORD] = 0;

    for (uint8_t i = 0; i < 2 * ERROR_CAPABILITY; ++i) {
        uint16_t alpha_i = antilogTable[i % GF_SIZE];
        uint16_t sum = 0;
        uint16_t alpha_power = 1;

        for (uint16_t bit = 0; bit < DATA_BITS; ++bit) {
            if (getBit(etl::span<const uint8_t>(paddedData), bit)) {
                sum = gfAdd(sum, alpha_power);
            }
            alpha_power = gfMul(alpha_power, alpha_i);
        }

        for (uint16_t bit = 0; bit < PARITY_BITS; ++bit) {
            if (getBit(parity, bit)) {
                sum = gfAdd(sum, alpha_power);
            }
            alpha_power = gfMul(alpha_power, alpha_i);
        }

        syndrome[i] = sum;
    }
}

uint8_t BCH_ECC::berlekampMassey(etl::span<const uint16_t> syndrome,
                                 etl::span<uint16_t> errorLocator) {
    etl::array<uint16_t, ERROR_CAPABILITY + 1> C;
    etl::array<uint16_t, ERROR_CAPABILITY + 1> B;
    C.fill(0);
    B.fill(0);

    C[0] = 1;
    B[0] = 1;
    uint8_t L = 0;
    uint8_t m = 1;
    uint16_t b = 1;

    for (uint8_t n = 0; n < 2 * ERROR_CAPABILITY; ++n) {
        uint16_t d = syndrome[n];
        for (uint8_t i = 1; i <= L; ++i) {
            d = gfAdd(d, gfMul(C[i], syndrome[n - i]));
        }

        if (d == 0) {
            m++;
        } else {
            etl::array<uint16_t, ERROR_CAPABILITY + 1> T = C;
            uint16_t factor = gfDiv(d, b);

            for (uint8_t i = 0; i <= L && (m + i) <= ERROR_CAPABILITY; ++i) {
                C[m + i] = gfAdd(C[m + i], gfMul(factor, B[i]));
            }

            if (2 * L <= n) {
                L = n + 1 - L;
                B = T;
                b = d;
                m = 1;
            } else {
                m++;
            }
        }
    }

    for (uint8_t i = 0; i <= L; ++i) {
        errorLocator[i] = C[i];
    }

    return L;
}

uint8_t BCH_ECC::chienSearch(etl::span<const uint16_t> errorLocator,
                             uint8_t degree,
                             etl::span<uint16_t> errorPositions) {
    uint8_t numErrors = 0;

    for (uint16_t i = 1; i <= CODEWORD_BITS; ++i) {
        uint16_t sum = errorLocator[0];

        for (uint8_t j = 1; j <= degree; ++j) {
            uint16_t alpha_ij = antilogTable[(static_cast<uint32_t>(i) * j) % GF_SIZE];
            sum = gfAdd(sum, gfMul(errorLocator[j], alpha_ij));
        }

        if (sum == 0) {
            errorPositions[numErrors++] = CODEWORD_BITS - i;
            if (numErrors >= ERROR_CAPABILITY) break;
        }
    }

    return numErrors;
}

void BCH_ECC::correctErrors(etl::span<uint8_t> data,
                            etl::span<const uint16_t> positions,
                            uint8_t numErrors) {
    for (uint8_t i = 0; i < numErrors; ++i) {
        uint16_t pos = positions[i];
        if (pos < DATA_BYTES_PER_CODEWORD * 8) {
            toggleBit(data, pos);
        }
    }
}