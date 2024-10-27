#include "InternalFlash/inc/InternalFlash.hpp"

InternalFlash::InternalFlash() {
  EFC_Initialize();
}

EFC_ERROR InternalFlash::QuadWordWrite(uint32_t* data, uint32_t address) {
    EFC_QuadWordWrite(data, address);

    while (EFC_IsBusy());

    return EFC_ErrorGet();
}

EFC_ERROR InternalFlash::PageWrite(uint32_t* data, uint32_t address) {
    EFC_PageWrite(data, address);

    while (EFC_IsBusy());

    return EFC_ErrorGet();
}

EFC_ERROR InternalFlash::Read(uint32_t* data, uint32_t length, uint32_t address) {
    EFC_Read(data, length, address);

    while (EFC_IsBusy());

    return EFC_ErrorGet();
}

EFC_ERROR InternalFlash::SectorErase(uint32_t address) {
    EFC_SectorErase(address);

    while (EFC_IsBusy());

    return EFC_ErrorGet();
}