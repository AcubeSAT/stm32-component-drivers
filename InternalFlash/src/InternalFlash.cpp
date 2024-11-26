#include "InternalFlash.hpp"

FlashDriver::FlashDriver() {
    ;
}

EFC_ERROR FlashDriver::QuadWordWrite(FlashData* data, FlashAddress address) {
    EFC_QuadWordWrite(data, address);

    waitForResponse();

    return EFC_ErrorGet();
}

EFC_ERROR FlashDriver::PageWrite(FlashData* data, FlashAddress address) {
    EFC_PageWrite(data, address);

    waitForResponse();

    return EFC_ErrorGet();
}

EFC_ERROR FlashDriver::Read(FlashData* data, FlashEraseLength length, FlashAddress address) {
    EFC_Read(data, length, address);

    waitForResponse();

    return EFC_ErrorGet();
}

EFC_ERROR FlashDriver::SectorErase(FlashAddress address) {
    EFC_SectorErase(address);

    waitForResponse();

    return EFC_ErrorGet();
}