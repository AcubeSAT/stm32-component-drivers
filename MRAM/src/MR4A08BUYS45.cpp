#include "MR4A08BUYS45.hpp"

volatile XDMAC_TRANSFER_EVENT MRAM::lastTransferStatus = XDMAC_TRANSFER_NONE;
volatile bool MRAM::transferPending = false;

bool MRAM::isAddressRangeValid(uint32_t startAddress, size_t size) const {
    if (startAddress > MaxWriteableAddress ||
        size > MaxWriteableAddress ||
        (startAddress + size - 1) > MaxWriteableAddress) {
        return false;
    }

    const uint32_t endAddress = moduleBaseAddress | (startAddress + size - 1);
    return endAddress <= moduleEndAddress;
}

MRAMError MRAM::startCopyFromRAM(void *dest, const void *src, size_t count) {
    if (count == 0) {
        return MRAMError::INVALID_ARGUMENT;
    }

    const uint32_t destOffset = reinterpret_cast<uint32_t>(dest) - moduleBaseAddress;
    if (!isAddressRangeValid(destOffset, count)) {
        return MRAMError::ADDRESS_OUT_OF_BOUNDS;
    }

    if (transferPending) {
        return MRAMError::DMA_IN_PROGRESS;
    }

    transferPending = true;
    const bool started = XDMAC_ChannelTransfer(DmaChannel, src, dest, count);
    if (!started) {
        transferPending = false;
        return MRAMError::DMA_IN_PROGRESS;
    }

    return MRAMError::NONE;
}

MRAMError MRAM::checkDMACompletion() {
    if (transferPending) {
        return MRAMError::DMA_IN_PROGRESS;
    }

    return (lastTransferStatus == XDMAC_TRANSFER_ERROR)
               ? MRAMError::DMA_TRANSFER_ERROR
               : MRAMError::NONE;
}

MRAMError MRAM::isMRAMAlive() {
    // Direct, non-DMA access
    uint8_t readId[CustomIDSize] = {};
    for (size_t i = 0; i < CustomIDSize; i++) {
        uint32_t address = moduleBaseAddress | (CustomMRAMIDAddress + i);
        readId[i] = smcReadByte(address);
    }

    auto matches = [&readId]() -> bool {
        for (size_t i = 0; i < CustomIDSize; i++) {
            if (readId[i] != CustomID[i]) {
                return false;
            }
        }
        return true;
    };

    if (matches()) {
        return MRAMError::READY;
    }

    // Try writing the ID since it might be the first boot.
    for (size_t i = 0; i < CustomIDSize; i++) {
        uint32_t address = moduleBaseAddress | (CustomMRAMIDAddress + i);
        smcWriteByte(address, CustomID[i]);
        readId[i] = smcReadByte(address);
    }

    return matches() ? MRAMError::READY : MRAMError::NOT_READY;
}

namespace {
    constexpr uint32_t SmcPeripheralClockHz = 150'000'000;

    constexpr uint32_t nsToCycles(uint32_t nanoseconds) {
        return static_cast<uint32_t>(
            (static_cast<uint64_t>(nanoseconds) * SmcPeripheralClockHz + 999'999'999ULL)
            / 1'000'000'000ULL);
    }
}

void MRAM::configureSmcTiming() {
    // MR4A08BUYS45 datasheet: Table 11 (read), Table 12 (write, W-controlled).
    constexpr uint32_t readCycle  = nsToCycles(45); // t_AVAV (read cycle time)
    constexpr uint32_t writeCycle = nsToCycles(45); // t_AVAV (write cycle time)
    constexpr uint32_t writePulse = nsToCycles(15); // t_WLWH (write pulse width)
    constexpr uint32_t writeHold  = nsToCycles(12); // t_WHAX (write recovery time)
    constexpr uint32_t writeSetup =
        (writeCycle > writePulse + writeHold) ? (writeCycle - writePulse - writeHold) : 1;

    const uint32_t setupReg =
          SMC_SETUP_NRD_SETUP(0)
        | SMC_SETUP_NCS_RD_SETUP(0)
        | SMC_SETUP_NWE_SETUP(writeSetup)
        | SMC_SETUP_NCS_WR_SETUP(0);

    const uint32_t pulseReg =
          SMC_PULSE_NRD_PULSE(readCycle)
        | SMC_PULSE_NCS_RD_PULSE(readCycle)
        | SMC_PULSE_NWE_PULSE(writePulse)
        | SMC_PULSE_NCS_WR_PULSE(writeCycle);

    const uint32_t cycleReg =
          SMC_CYCLE_NRD_CYCLE(readCycle)
        | SMC_CYCLE_NWE_CYCLE(writeCycle);

    const uint32_t modeReg =
          SMC_MODE_READ_MODE
        | SMC_MODE_WRITE_MODE
        | SMC_MODE_EXNW_MODE(SMC_MODE_EXNW_MODE_DISABLED_Val)
        | SMC_MODE_DBW(SMC_MODE_DBW_BIT_8_Val)
        | SMC_MODE_TDF_CYCLES(1)
        | SMC_MODE_TDF_MODE;

    SMC_REGS->SMC_CS_NUMBER[chipSelectNumber].SMC_SETUP = setupReg;
    SMC_REGS->SMC_CS_NUMBER[chipSelectNumber].SMC_PULSE = pulseReg;
    SMC_REGS->SMC_CS_NUMBER[chipSelectNumber].SMC_CYCLE = cycleReg;
    // SMC_MODE last: writing it triggers SMC's "reload configuration wait
    // state", which is how the new setup/pulse/cycle values take effect.
    SMC_REGS->SMC_CS_NUMBER[chipSelectNumber].SMC_MODE  = modeReg;
}

void MRAM::configureDmaChannel() {
    XDMAC_ChannelCallbackRegister(DmaChannel,
        [](const XDMAC_TRANSFER_EVENT status, uintptr_t context) -> void {
            (void)context;
            lastTransferStatus = status;
            transferPending = false;
        },
        reinterpret_cast<uintptr_t>(nullptr));
}