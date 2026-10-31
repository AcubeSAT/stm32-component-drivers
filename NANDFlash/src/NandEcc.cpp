#include "NandEcc.hpp"

#include <cstring>
#include <etl/algorithm.h>

namespace NandEcc {

    namespace {

        /**
         * @brief True if every byte of the span is ErasedByte.
         *
         * @details The scan compares 32-bit words rather than bytes because the
         *          cost is per loop iteration. Currently with the code in
         *          flash and no instruction cache every taken branch refetches
         *          the loop body so four times fewer iterations is close to
         *          four times less time. The word is assembled with memcpy because the
         *          span carries no alignment guarantee and a uint32_t* cast
         *          would violate both alignment and strict aliasing. The byte wise
         *          tail handles spans whose length is not a multiple of four
         *          (the 335-byte parity field).
         */
        bool allBytesErased(etl::span<const uint8_t> bytes) {
            constexpr size_t WordBytes = sizeof(uint32_t);
            constexpr uint32_t ErasedWord = 0xFFFFFFFFU;
            static_assert(ErasedByte == 0xFFU, "ErasedWord must be ErasedByte repeated");

            const uint8_t* cursor = bytes.data();
            const uint8_t* const End = cursor + bytes.size();

            for (; static_cast<size_t>(End - cursor) >= WordBytes; cursor += WordBytes) {
                uint32_t word = 0U;
                std::memcpy(&word, cursor, WordBytes);
                if (word != ErasedWord) {
                    return false;
                }
            }

            return etl::all_of(cursor, End, [](uint8_t byte) { return byte == ErasedByte; });
        }

        /**
         * @brief Find the version value with at least VersionAcceptanceThreshold identical copies.
         *
         * @return The accepted value, or no value if no copy reaches the threshold
         */
        etl::expected<uint8_t, EccError> acceptedVersion(etl::span<const uint8_t, VersionCopyCount> version) {
            for (const uint8_t candidate : version) {
                const size_t Matches = etl::count(version.begin(), version.end(), candidate);

                if (Matches >= VersionAcceptanceThreshold) {
                    return candidate;
                }
            }

            return etl::unexpected(EccError::CORRUPT);
        }

    } // namespace

    void computeParity(etl::span<const uint8_t, DataBytesPerPage> data, etl::span<uint8_t, ParityBytesPerPage> parity) {
        static_cast<void>(data);

        etl::fill(parity.begin(), parity.end(), ErasedByte);
    }

    etl::expected<CorrectionResult, EccError> correct(etl::span<uint8_t, DataBytesPerPage> data,
                                                      etl::span<const uint8_t, ParityBytesPerPage> parity,
                                                      etl::span<const uint8_t, VersionCopyCount> version) {
        if (allBytesErased(data) and allBytesErased(parity) and allBytesErased(version)) {
            return CorrectionResult{true, 0U};
        }

        auto accepted = acceptedVersion(version);
        if (not accepted.has_value()) {
            return etl::unexpected(accepted.error());
        }

        if (accepted.value() == OobLayoutVersion) {
            return CorrectionResult{false, 0U};
        }

        if (accepted.value() == ErasedByte) {
            return etl::unexpected(EccError::CORRUPT);
        }

        return etl::unexpected(EccError::INCOMPATIBLE_LAYOUT);
    }

} // namespace NandEcc
