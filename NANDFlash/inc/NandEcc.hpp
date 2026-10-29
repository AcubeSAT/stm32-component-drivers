#pragma once

#include <cstddef>
#include <cstdint>
#include <etl/expected.h>
#include <etl/span.h>

/**
 * @brief Per page ECC for the MT29F64G08AFAAAWP NAND flash.
 *
 * @details Target code is BCH(1023,983,4) over the 8192B data area:
 *          each codeword protects 983 data bits with 40 parity bits (5 B),
 *          and ceil(65536 / 983) = 67 codewords cover the page, giving 335 parity
 *          bytes per page.
 *
 * @note    Spare area layout version field: one version byte,
 *          written redundantly in VersionCopyCount (4) consecutive spare bytes
 *          (absolute NAND columns 8528-8531, directly after the parity area).
 *          A version field is accepted only if at least
 *          VersionAcceptanceThreshold (3) of the 4 copies are identical.
 *
 * @warning This module is a DUMMY for now.
 * 
 * @ingroup drivers
 */
namespace NandEcc {

    /* ============== BCH(1023,983,4) geometry ============== */

    inline constexpr size_t DataBytesPerPage = 8192U;         /*!< Data area covered by ECC */
    inline constexpr size_t DataBitsPerCodeword = 983U;       /*!< Payload bits per BCH(1023,983,4) codeword */
    inline constexpr size_t ParityBytesPerCodeword = 5U;      /*!< 40 parity bits per codeword */
    inline constexpr size_t CodewordsPerPage = 67U;           /*!< ceil((8192 × 8) / 983) */
    inline constexpr size_t ParityBytesPerPage = CodewordsPerPage * ParityBytesPerCodeword; /*!< 335 B */

    static_assert(CodewordsPerPage == ((DataBytesPerPage * 8U) + DataBitsPerCodeword - 1U) / DataBitsPerCodeword);
    static_assert(ParityBytesPerPage == 335U);

    /* ============== Spare area layout version field ============== */

    inline constexpr uint8_t OobLayoutVersion = 0x01U;        /*!< Current version */
    inline constexpr size_t VersionCopyCount = 4U;            /*!< Redundant copies of the version byte (spare cols 8528-8531) */
    inline constexpr size_t VersionAcceptanceThreshold = 3U;  /*!< Minimum identical copies for the field to be accepted */

    inline constexpr uint8_t ErasedByte = 0xFFU;              /*!< Value of every byte on a cleanly erased page */

    /**
     * @brief Error codes for ECC correction.
     */
    enum class EccError : uint8_t {
        CORRUPT = 1U,          /*!< Uncorrectable page */
        INCOMPATIBLE_FORMAT,   /*!< Valid version field from a different ECC version */
    };

    /**
     * @brief Outcome of a successful correct() call.
     */
    struct CorrectionResult {
        bool isErasedPage;          /*!< true if the page is cleanly erased (all 0xFF) */
        uint32_t correctedBitCount; /*!< Bit flips corrected in place */
    };

    /**
     * @brief Compute per page parity for the data area.
     *
     * @details DUMMY: fills the parity span with 0xFF.
     *
     * @param data Page data area
     * @param[out] parity Parity output
     */
    void computeParity(etl::span<const uint8_t, DataBytesPerPage> data,
                       etl::span<uint8_t, ParityBytesPerPage> parity);

    /**
     * @brief Validate and correct a page read from flash.
     *
     * @param data Page data area
     * @param parity Parity read from the spare area
     * @param version Spare area layout version copies read from the spare area
     *
     * @return CorrectionResult on success, EccError otherwise
     */
    [[nodiscard]] etl::expected<CorrectionResult, EccError> correct(etl::span<uint8_t, DataBytesPerPage> data,
                                                                    etl::span<const uint8_t, ParityBytesPerPage> parity,
                                                                    etl::span<const uint8_t, VersionCopyCount> version);

} // namespace NandEcc
