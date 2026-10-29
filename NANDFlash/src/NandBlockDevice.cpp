#include "NandBlockDevice.hpp"

#include <etl/algorithm.h>

NandBlockDevice::NandBlockDevice(etl::span<const DieConfig> dieConfigs, YieldDelegate yieldMs,
                                 BusLockDelegate lockBus, BusLockDelegate unlockBus)
    : dieCount{(dieConfigs.size() >= 1U and dieConfigs.size() <= MaxDieCount) ? dieConfigs.size() : 0U},
      hasMismatchedBusDelegates{lockBus.is_valid() != unlockBus.is_valid()},
      lockBus{lockBus},
      unlockBus{unlockBus} {
    for (size_t die = 0U; die < dieCount; die++) {
        dies[die].emplace(dieConfigs[die].chipSelect, dieConfigs[die].readyBusyPin,
                          dieConfigs[die].writeProtectPin, yieldMs);
    }

    progSpare[MarkerOffset] = NandEcc::ErasedByte;
    etl::fill_n(progSpare.begin() + VersionOffset, NandEcc::VersionCopyCount, NandEcc::OobLayoutVersion);
}

etl::expected<void, BlockDeviceError> NandBlockDevice::initialize() {
    if (dieCount == 0U) {
        return etl::unexpected(BlockDeviceError::INVALID_DIE_COUNT);
    }

    if (hasMismatchedBusDelegates) {
        return etl::unexpected(BlockDeviceError::MISMATCHED_BUS_DELEGATES);
    }

    if (isInitialized) {
        return {};
    }

    for (size_t die = 0U; die < dieCount; die++) {
        BusGuard lock{*this};

        if (auto initResult = dies[die]->initialize();
            not initResult.has_value() and initResult.error() != NANDErrorCode::ALREADY_INITIALIZED) {
            return etl::unexpected(BlockDeviceError::DRIVER_ERROR);
        }
    }

    isInitialized = true;

    return {};
}

etl::expected<void, BlockDeviceError> NandBlockDevice::reset() {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    for (size_t die = 0U; die < dieCount; die++) {
        BusGuard lock{*this};

        if (auto resetResult = dies[die]->reset(); not resetResult.has_value()) {
            return etl::unexpected(BlockDeviceError::DRIVER_ERROR);
        }
    }

    return {};
}

etl::expected<void, BlockDeviceError> NandBlockDevice::validateSuperblockAnchors() const {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    for (uint16_t anchorBlock = 0U; anchorBlock <= 1U; anchorBlock++) {
        const auto IsBad = dies[0]->isBlockBad(anchorBlock);

        if (not IsBad.has_value()) {
            return etl::unexpected(BlockDeviceError::ANCHOR_QUERY_FAILED);
        }

        if (IsBad.value()) {
            return etl::unexpected(BlockDeviceError::ANCHOR_BLOCK_BAD);
        }
    }

    return {};
}

etl::expected<uint32_t, BlockDeviceError> NandBlockDevice::badBlockCount(size_t dieIndex) const {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    if (dieIndex >= dieCount) {
        return etl::unexpected(BlockDeviceError::OUT_OF_BOUNDS);
    }

    uint32_t count = 0U;

    for (uint32_t block = 0U; block < BlocksPerDie; block++) {
        const auto IsBad = dies[dieIndex]->isBlockBad(static_cast<uint16_t>(block));

        if (not IsBad.has_value()) {
            return etl::unexpected(BlockDeviceError::DRIVER_ERROR);
        }

        if (IsBad.value()) {
            count++;
        }
    }

    return count;
}


/* ============= Page / block operations ============= */

etl::expected<void, BlockDeviceError> NandBlockDevice::readPages(uint32_t block, size_t offset, etl::span<uint8_t> buffer) {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    if (not isValidPageRange(block, offset, buffer.size())) {
        return etl::unexpected(BlockDeviceError::INVALID_PARAMETER);
    }

    MT29F& targetDie = die(block);

    const etl::span<const uint8_t, NandEcc::ParityBytesPerPage> Parity{readSpare.data() + ParityOffset,
                                                                       NandEcc::ParityBytesPerPage};
    const etl::span<const uint8_t, NandEcc::VersionCopyCount> Version{readSpare.data() + VersionOffset,
                                                                      NandEcc::VersionCopyCount};

    for (size_t pageIndex = 0U; pageIndex < buffer.size() / PageSize; pageIndex++) {
        const MT29F::NANDAddress Address{0U, localBlock(block), static_cast<uint32_t>((offset / PageSize) + pageIndex), 0U};
        const etl::span<uint8_t, NandEcc::DataBytesPerPage> Data{buffer.data() + (pageIndex * PageSize),
                                                                 NandEcc::DataBytesPerPage};

        {
            BusGuard lock{*this};

            if (auto readResult = targetDie.readPageWithSpare(Address, Data, readSpare); not readResult.has_value()) {
                return etl::unexpected(BlockDeviceError::IO);
            }
        }

        auto correctResult = NandEcc::correct(Data, Parity, Version);
        if (not correctResult.has_value()) {
            if (correctResult.error() == NandEcc::EccError::INCOMPATIBLE_FORMAT) {
                incompatibleFormatReads++;
                return etl::unexpected(BlockDeviceError::INCOMPATIBLE_FORMAT);
            }

            return etl::unexpected(BlockDeviceError::UNCORRECTABLE);
        }

    }

    return {};
}

etl::expected<void, BlockDeviceError> NandBlockDevice::programPages(uint32_t block, size_t offset, etl::span<const uint8_t> data) {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    if (not isValidPageRange(block, offset, data.size())) {
        return etl::unexpected(BlockDeviceError::INVALID_PARAMETER);
    }

    MT29F& targetDie = die(block);

    const etl::span<uint8_t, NandEcc::ParityBytesPerPage> Parity{progSpare.data() + ParityOffset,
                                                                 NandEcc::ParityBytesPerPage};

    for (size_t pageIndex = 0U; pageIndex < data.size() / PageSize; pageIndex++) {
        const MT29F::NANDAddress Address{0U, localBlock(block), static_cast<uint32_t>((offset / PageSize) + pageIndex), 0U};
        const etl::span<const uint8_t, NandEcc::DataBytesPerPage> Data{data.data() + (pageIndex * PageSize),
                                                                       NandEcc::DataBytesPerPage};

        NandEcc::computeParity(Data, Parity);

        BusGuard lock{*this};

        if (auto programResult = targetDie.programPageWithSpare(Address, Data, progSpare); not programResult.has_value()) {
            if ((programResult.error() == NANDErrorCode::PROGRAM_FAILED)
                or (programResult.error() == NANDErrorCode::PROGRAM_TIMEOUT)) {
                static_cast<void>(targetDie.markBadBlock(static_cast<uint16_t>(localBlock(block))));
                return etl::unexpected(BlockDeviceError::BAD_BLOCK);
            }

            return etl::unexpected(BlockDeviceError::IO);
        }
    }

    return {};
}

etl::expected<void, BlockDeviceError> NandBlockDevice::eraseBlock(uint32_t block) {
    if (not isInitialized) {
        return etl::unexpected(BlockDeviceError::NOT_INITIALIZED);
    }

    if (block >= blockCount()) {
        return etl::unexpected(BlockDeviceError::INVALID_PARAMETER);
    }

    MT29F& targetDie = die(block);
    const auto LocalBlock = static_cast<uint16_t>(localBlock(block));

    const auto IsBad = targetDie.isBlockBad(LocalBlock);

    if (not IsBad.has_value()) {
        return etl::unexpected(BlockDeviceError::IO);
    }

    if (IsBad.value()) {
        return etl::unexpected(BlockDeviceError::BAD_BLOCK);
    }

    BusGuard lock{*this};

    if (auto eraseResult = targetDie.eraseBlock(LocalBlock); not eraseResult.has_value()) {
        if ((eraseResult.error() == NANDErrorCode::ERASE_FAILED)
            or (eraseResult.error() == NANDErrorCode::ERASE_TIMEOUT)) {
            static_cast<void>(targetDie.markBadBlock(LocalBlock));
            return etl::unexpected(BlockDeviceError::BAD_BLOCK);
        }

        return etl::unexpected(BlockDeviceError::IO);
    }

    return {};
}
