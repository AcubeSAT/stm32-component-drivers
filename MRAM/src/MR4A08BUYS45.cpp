#include <etl/span.h>
#include "MR4A08BUYS45.hpp"

MRAM_Errno MRAM::mramWriteByte(uint32_t dataAddress, const uint8_t &data) {
    uint32_t writeAddress = moduleBaseAddress | dataAddress;
    if( (dataAddress > maxWriteableAddress) || (writeAddress > moduleEndAddress) ){
        return MRAM_Errno::MRAM_ADDRESS_OUT_OF_BOUNDS;
    }
    smcWriteByte(writeAddress, data);
    return MRAM_Errno ::MRAM_NONE;
}

MRAM_Errno MRAM::mramReadByte(uint32_t dataAddress, uint8_t &data) {
    uint32_t readAddress = moduleBaseAddress | dataAddress;
    if( (dataAddress > maxAllowedAddress) || (readAddress > moduleEndAddress) ){
        return MRAM_Errno::MRAM_ADDRESS_OUT_OF_BOUNDS;
    }
    data = smcReadByte(readAddress);
    return MRAM_Errno ::MRAM_NONE;
}

MRAM_Errno MRAM::mramWriteData(uint32_t startAddress, etl::span<const uint8_t> &data){
    MRAM_Errno  error = MRAM_Errno::MRAM_NONE;
    for(size_t i=0;i<data.size();i++){
        error = mramWriteByte((startAddress+i), data[i]);
        if(error!=MRAM_Errno::MRAM_NONE){
            return error;
        }
    }
    return error;
}

MRAM_Errno MRAM::mramReadData(uint32_t startAddress, etl::span<uint8_t> data){
    MRAM_Errno  error = MRAM_Errno::MRAM_NONE;
    for(size_t i=0;i<data.size();i++){
        error = mramReadByte((startAddress+i), data[i]);
        if(error!=MRAM_Errno::MRAM_NONE){
            return error;
        }
    }
    return error;
}

void MRAM::writeID(void){
    for(size_t i=0;i<customIDSize;i++){
        smcWriteByte( (moduleBaseAddress | (customMRAMIDAddress+i)), customID[i] );
    }
}

bool MRAM::checkID(uint8_t* idArray){
    for(size_t i=0;i<customIDSize;i++){
        if(idArray[i]!=customID[i]){
            return false;
        }
    }
    return true;
}

MRAM_Errno MRAM::isMRAMAlive(){
    etl::array<uint8_t, customIDSize> readId{};
    etl::span<uint8_t> readIDspan(readId.data(), readId.size());
    MRAM_Errno error = MRAM_Errno ::MRAM_NONE;
    error = mramReadData(customMRAMIDAddress, readId);
    if(error!=MRAM_Errno::MRAM_NONE){
        return error;
    }
    if(checkID(readId.data())){
        return MRAM_Errno::MRAM_READY;
    }
    // Try writing the id Since it can be the first time booting the device
    writeID();
    error = mramReadData(customMRAMIDAddress, readId);
    if(error!=MRAM_Errno::MRAM_NONE){
        return error;
    }
    if(checkID(readId.data())){
        return MRAM_Errno::MRAM_READY;
    }
    //Couldn't read after write attempt, MRAM is unresponsive
    return MRAM_Errno ::MRAM_NOT_READY;
}

void MRAM::errorHandler(MRAM_Errno error){
    switch (error) {
        case MRAM_Errno::MRAM_TIMEOUT:

            break;
        case MRAM_Errno::MRAM_ADDRESS_OUT_OF_BOUNDS:

            break;
        case MRAM_Errno::MRAM_NOT_READY:

            break;
        default:

            break;
    }
}
