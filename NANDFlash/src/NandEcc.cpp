#include "NandEcc.hpp"

#include <cstring>
#include <etl/algorithm.h>

namespace NandEcc {

    namespace {

        /**
         * @brief True if every byte of the span is ErasedByte.
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

        return etl::unexpected(EccError::INCOMPATIBLE_FORMAT);
    }

} // namespace NandEcc
