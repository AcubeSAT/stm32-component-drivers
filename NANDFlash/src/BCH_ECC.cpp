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

    // Generate logarithm and antilogarithm tables for GF(2^10)
    // Using primitive polynomial 0x409 = x^10 + x^3 + 1
    logTable.fill(0);
    antilogTable.fill(0);

    uint16_t element = 1;
    for (uint16_t i = 0; i < GF_SIZE; ++i) {
        antilogTable[i] = element;
        logTable[element] = i;

        element <<= 1;
        if (element >= (1 << GF_BITS)) {
            element ^= PRIMITIVE_POLY;
        }
    }

    logTable[0] = GF_SIZE;
    antilogTable[GF_SIZE] = 0;

    buildGeneratorPoly();

    initialized = true;
    return {};
}

void BCH_ECC::buildGeneratorPoly() {
    // Build BCH generator polynomial as LCM of minimal polynomials
    // For BCH(1023, 983, 4), generator has roots at α^1, α^2, ..., α^8
    generatorPoly.fill(0);
    generatorPoly[0] = 1;  // Start with g(x) = 1
    generatorDegree = 0;

    etl::array<bool, GF_SIZE> processedRoots;
    processedRoots.fill(false);

    for (uint16_t rootPower = 1; rootPower <= 2 * ERROR_CAPABILITY; ++rootPower) {
        if (processedRoots[rootPower]) {
            continue;
        }

        // Find minimal polynomial for α^rootPower
        etl::array<uint16_t, GF_BITS + 1> minPoly;
        minPoly.fill(0);
        minPoly[0] = 1;  // Start with m(x) = 1
        uint8_t minPolyDegree = 0;

        // Find all conjugates: α^rootPower, α^(2*rootPower), α^(4*rootPower), ...
        etl::array<bool, GF_SIZE> visited;
        visited.fill(false);
        etl::array<uint16_t, GF_BITS> conjugates;
        uint8_t conjugateCount = 0;

        uint16_t current = rootPower;
        while (!visited[current] && conjugateCount < GF_BITS) {
            visited[current] = true;
            processedRoots[current] = true;
            conjugates[conjugateCount++] = gfPow(2, current);  // α^current
            current = (current * 2) % GF_SIZE;  // Next conjugate power
        }

        // Build minimal polynomial as product of (x - conjugate)
        for (uint8_t i = 0; i < conjugateCount; ++i) {
            for (int j = minPolyDegree + 1; j > 0; --j) {
                uint16_t term = gfMul(conjugates[i], minPoly[j]);
                minPoly[j] = gfAdd(minPoly[j - 1], term);
            }
            minPoly[0] = gfMul(conjugates[i], minPoly[0]);
            minPolyDegree++;
        }

        // Multiply generator polynomial by this minimal polynomial
        etl::array<uint16_t, PARITY_BITS + 1> newGen;
        newGen.fill(0);

        for (uint8_t a = 0; a <= generatorDegree; ++a) {
            for (uint8_t b = 0; b <= minPolyDegree; ++b) {
                if (a + b <= PARITY_BITS) {
                    newGen[a + b] = gfAdd(newGen[a + b], gfMul(generatorPoly[a], minPoly[b]));
                }
            }
        }

        generatorDegree += minPolyDegree;
        generatorPoly = newGen;
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
    if (data.size() != DATA_BYTES_PER_CODEWORD || parity.size() != PARITY_BYTES_PER_CODEWORD) {
        return etl::unexpected(BCHError::INVALID_PARAMETER);
    }

    etl::bitset<DATA_BITS> dataBits;
    bytesToBitset(data, dataBits);

    // Systematic BCH encoding: compute parity as D(x) * x^40 mod g(x)
    etl::bitset<PARITY_BITS> parityBits = calculateParity(dataBits);

    bitsetToBytes(parityBits, parity);

    return {};
}

etl::bitset<BCH_ECC::PARITY_BITS> BCH_ECC::calculateParity(const etl::bitset<DATA_BITS>& dataBits) {
    etl::bitset<CODEWORD_BITS> workPoly;

    for (size_t i = 0; i < DATA_BITS; ++i) {
        workPoly[i + PARITY_BITS] = dataBits[i];
    }

    // Polynomial division in GF(2) - process from high degree to low
    for (int i = DATA_BITS + PARITY_BITS - 1; i >= static_cast<int>(PARITY_BITS); --i) {
        if (workPoly[i]) {
            for (uint8_t j = 0; j <= generatorDegree; ++j) {
                if (generatorPoly[j] != 0) {
                    workPoly.flip(i - PARITY_BITS + j);
                }
            }
        }
    }

    etl::bitset<PARITY_BITS> parityBits;
    for (size_t i = 0; i < PARITY_BITS; ++i) {
        parityBits[i] = workPoly[i];
    }

    return parityBits;
}

void BCH_ECC::bytesToBitset(etl::span<const uint8_t> bytes, etl::bitset<DATA_BITS>& bits) {
    bits.reset();

    for (size_t byteIdx = 0; byteIdx < bytes.size() && byteIdx * 8 < DATA_BITS; ++byteIdx) {
        uint8_t byte = bytes[byteIdx];
        for (size_t bitIdx = 0; bitIdx < 8 && (byteIdx * 8 + bitIdx) < DATA_BITS; ++bitIdx) {
            bits[byteIdx * 8 + bitIdx] = (byte >> bitIdx) & 1;
        }
    }
}

void BCH_ECC::bitsetToBytes(const etl::bitset<PARITY_BITS>& bits, etl::span<uint8_t> bytes) {
    std::memset(bytes.data(), 0, bytes.size());

    for (size_t i = 0; i < PARITY_BITS && i / 8 < bytes.size(); ++i) {
        if (bits[i]) {
            bytes[i / 8] |= (1 << (i % 8));
        }
    }
}

etl::expected<uint8_t, BCH_ECC::BCHError> BCH_ECC::decode(etl::span<uint8_t> data,
                                                          etl::span<const uint8_t> parity) {
    if (data.size() != DATA_BYTES_PER_CODEWORD || parity.size() != PARITY_BYTES_PER_CODEWORD) {
        return etl::unexpected(BCHError::INVALID_PARAMETER);
    }

    etl::bitset<CODEWORD_BITS> receivedCodeword;
    bytesToCodeword(parity, data, receivedCodeword);

    etl::array<uint16_t, 2 * ERROR_CAPABILITY> syndromes;
    calculateSyndromes(receivedCodeword, syndromes);

    bool hasErrors = false;
    for (uint8_t i = 0; i < 2 * ERROR_CAPABILITY; ++i) {
        if (syndromes[i] != 0) {
            hasErrors = true;
            break;
        }
    }

    if (!hasErrors) {
        return 0;
    }

    etl::array<uint16_t, 2 * ERROR_CAPABILITY + 1> errorLocator;
    errorLocator.fill(0);
    uint8_t errorCount = berlekampMassey(syndromes, errorLocator);

    if (errorCount == 0) {
        return etl::unexpected(BCHError::LOCATOR_ERROR);
    }

    if (errorCount > ERROR_CAPABILITY) {
        return etl::unexpected(BCHError::TOO_MANY_ERRORS);
    }

    etl::array<uint16_t, ERROR_CAPABILITY> errorPositions;
    uint8_t foundErrors = chienSearch(errorLocator, errorCount, errorPositions);

    if (foundErrors != errorCount) {
        return etl::unexpected(BCHError::LOCATOR_ERROR);
    }

    for (uint8_t i = 0; i < foundErrors; ++i) {
        uint16_t pos = errorPositions[i];
        if (pos < CODEWORD_BITS) {
            receivedCodeword[pos] = receivedCodeword[pos] ^ 1;  // Flip bit
        }
    }

    codewordToBytes(receivedCodeword, data);

    return foundErrors;
}

void BCH_ECC::calculateSyndromes(const etl::bitset<CODEWORD_BITS>& receivedCodeword,
                                 etl::span<uint16_t> syndromes) {
    for (uint8_t j = 0; j < 2 * ERROR_CAPABILITY; ++j) {
        uint16_t syndrome = 0;
        uint16_t alphaPower = 1;  // Start with α^0 = 1
        uint16_t alphaRoot = gfPow(2, j + 1);

        // Evaluate polynomial: Σ(i=0 to 1022) r_i * α^((j+1)*i)
        for (uint16_t i = 0; i < CODEWORD_BITS; ++i) {
            if (receivedCodeword[i]) {
                syndrome = gfAdd(syndrome, alphaPower);
            }
            alphaPower = gfMul(alphaPower, alphaRoot);
        }

        syndromes[j] = syndrome;
    }
}

void BCH_ECC::bytesToCodeword(etl::span<const uint8_t> parity, etl::span<const uint8_t> data,
                               etl::bitset<CODEWORD_BITS>& codeword) {
    codeword.reset();

    for (size_t i = 0; i < PARITY_BITS && i / 8 < parity.size(); ++i) {
        codeword[i] = (parity[i / 8] >> (i % 8)) & 1;
    }

    for (size_t i = 0; i < DATA_BITS && i / 8 < data.size(); ++i) {
        codeword[PARITY_BITS + i] = (data[i / 8] >> (i % 8)) & 1;
    }
}

void BCH_ECC::codewordToBytes(const etl::bitset<CODEWORD_BITS>& codeword, etl::span<uint8_t> data) {
    std::memset(data.data(), 0, data.size());

    for (size_t i = 0; i < DATA_BITS && i / 8 < data.size(); ++i) {
        if (codeword[PARITY_BITS + i]) {
            data[i / 8] |= (1 << (i % 8));
        }
    }
}

uint8_t BCH_ECC::berlekampMassey(etl::span<const uint16_t> syndromes,
                                 etl::span<uint16_t> errorLocator) {
    etl::array<uint16_t, 2 * ERROR_CAPABILITY + 1> C;  // Current connection polynomial
    etl::array<uint16_t, 2 * ERROR_CAPABILITY + 1> B;  // Previous connection polynomial

    C.fill(0);
    B.fill(0);
    C[0] = 1;  // C(x) = 1
    B[0] = 1;  // B(x) = 1

    uint8_t L = 0;      // Current degree of C(x)
    uint8_t m = 1;      // Shift amount
    uint16_t b = 1;     // Discrepancy value when L was last updated

    for (uint8_t n = 0; n < 2 * ERROR_CAPABILITY; ++n) {
        // Calculate discrepancy d_n
        uint16_t d = syndromes[n];
        for (uint8_t i = 1; i <= L; ++i) {
            d = gfAdd(d, gfMul(C[i], syndromes[n - i]));
        }

        if (d == 0) {
            m++;
            continue;
        }

        etl::array<uint16_t, 2 * ERROR_CAPABILITY + 1> T = C;

        // C(x) = C(x) - (d/b) * x^m * B(x)
        uint16_t coeff = gfDiv(d, b);
        for (size_t i = 0; i < B.size() && i + m < C.size(); ++i) {
            if (B[i] != 0) {
                C[i + m] = gfAdd(C[i + m], gfMul(coeff, B[i]));
            }
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

    for (size_t i = 0; i < errorLocator.size() && i <= L; ++i) {
        errorLocator[i] = C[i];
    }

    return L;
}

uint8_t BCH_ECC::chienSearch(etl::span<const uint16_t> errorLocator,
                             uint8_t errorCount,
                             etl::span<uint16_t> errorPositions) {
    // If error is at position j, then α^(-j) should be a root
    uint8_t foundErrors = 0;

    uint8_t polyDegree = 0;
    for (int i = errorLocator.size() - 1; i >= 0; --i) {
        if (errorLocator[i] != 0) {
            polyDegree = i;
            break;
        }
    }

    for (uint16_t j = 0; j < CODEWORD_BITS; ++j) {
        uint16_t alphaInvJ = gfPow(2, (GF_SIZE - j) % GF_SIZE);
        uint16_t evaluation = 0;
        uint16_t xPower = 1;

        for (uint8_t i = 0; i <= polyDegree; ++i) {
            evaluation = gfAdd(evaluation, gfMul(errorLocator[i], xPower));
            xPower = gfMul(xPower, alphaInvJ);
        }

        if (evaluation == 0) {
            errorPositions[foundErrors++] = j;

            if (foundErrors >= errorCount) {
                break;
            }
        }
    }

    return foundErrors;
}
