#include <etl/span.h>
#include "NANDFlash.h"


MT29F_Errno MT29F::resetNAND() {
    sendCommand(RESET);
    sendCommand(READ_STATUS);
    if (waitTimeout()) {
        return MT29F_Errno::TIMEOUT;
    }
    return MT29F_Errno::NONE;
}

/**
 * @brief Read Specific device info
 *
 * @param id 8-byte array to store the return data.
 * Returns 5 bytes containing, manufacturers ID, device configuration and part-specific information.
 * @return MT29F_Errno::NONE on SUCCESS
 */
MT29F_Errno MT29F::readNANDID(etl::span<uint8_t, 8> id) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    sendCommand(READID);
    sendAddress(READ_MODE);
    if (waitTimeout()) {
        return MT29F_Errno::TIMEOUT;
    }
    for (uint8_t i = 0; i < 8; i++) {
        id[i] = readData();
    }
    return MT29F_Errno::NONE;
}

/**
 * @brief Read Specific device info
 *
 * @param id 8-byte array to store the return data.
 * Returns 4-byte ONFI identifier code.
 * @return MT29F_Errno::NONE on SUCCESS
 */
MT29F_Errno MT29F::readONFIID(etl::span<uint8_t, 8> onfi_id) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    sendCommand(READID);
    sendAddress(READ_ONFI_CONFIRM);
    if (waitTimeout()) {
        return MT29F_Errno::TIMEOUT;
    }
    for (uint8_t i = 0; i < 8; i++) {
        onfi_id[i] = readData();
    }
    return MT29F_Errno::NONE;
}

bool MT29F::isNANDAlive() {
    uint8_t id[8] = {};
    const uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
    readNANDID(id);
    if (std::equal(std::begin(valid_id), std::end(valid_id), id)) {
        return true;
    } else return false;
}

bool MT29F::waitTimeout() {
    const uint32_t start = xTaskGetTickCount();
    while ((PIO_PinRead(nandReadyBusyPin) == 0)) {
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return true;
        }
    }
    return false;
}

void MT29F::errorHandler(MT29F_Errno error) {
    switch (error)
    {
    case MT29F_Errno::TIMEOUT:
        // Increase timeout counter and reset?
        break;
    case MT29F_Errno::ADDRESS_OUT_OF_BOUNDS:
        // Inform user
        break;
    case MT29F_Errno::BUSY_IO:
        // Queue task for later execution?
        break;
    case MT29F_Errno::BUSY_ARRAY:
        // Same as above
        break;
    case MT29F_Errno::FAIL_PREVIOUS:
        //Inform user
        break;
    case MT29F_Errno::FAIL_RECENT:
        //Inform user & retry ?
        break;
    case MT29F_Errno::NOT_READY:
        //Check for status later
        break;
    default:
        break;
    }
}


bool MT29F::setAddress(Address *address, uint8_t LUN, uint64_t position) {
    if(position>MaxAllowedAddress){
        return false;
    }
    const uint16_t column = position % PageSizeBytes;
    const uint8_t page = (position / PageSizeBytes) % PagesPerBlock;
    const uint16_t block = position / BlockSizeBytes;

    address->col1 = column & 0xff;
    address->col2 = (column & 0x3f00) >> 8;
    address->row1 = (page & 0x7f) | ((block & 0x01) << 7);
    address->row2 = (block >> 1) & 0xff;
    address->row3 = ((block >> 9) & 0x07) | ((LUN & 0x01) << 3);

    return true;
}

MT29F_Errno MT29F::writeNAND(uint8_t LUN, uint64_t position, uint8_t data) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    Address writeAddress{};
    if(!setAddress(&writeAddress, LUN, position)){
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    sendData(data);
    sendCommand(PAGE_PROGRAM_CONFIRM);

    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }

    if(!getARDYstatus() || !getRDYstatus()){
        return MT29F_Errno::NOT_READY;
    }
    if(getFAILstatus()){
        return MT29F_Errno::FAIL_RECENT;
    }

    //FAIL STATUS is 0
    return MT29F_Errno::NONE;
}

MT29F_Errno MT29F::writeNAND(uint8_t LUN, uint64_t position, etl::span<const uint8_t> data) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    if(position+data.size()>MaxAllowedAddress){
        // The selected address cannot accomodate for sequentially writing the whole data length
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }
    uint8_t start_page = (position / PageSizeBytes) % PagesPerBlock;
    Address writeAddress{};
    if(!setAddress(&writeAddress, LUN, position)){
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }
    for(size_t i=0; i<data.size(); i++){
        if( (((position+i)/PageSizeBytes)%PagesPerBlock) != start_page ){
            // Next address is located in new page, NAND module should reload its contents in its cache
            // Finish previous operation by saving the already transferred bytes
            // start new transaction with updated addresses and page info
            sendCommand(PAGE_PROGRAM_CONFIRM);
            if(waitTimeout()){
                return MT29F_Errno::TIMEOUT;
            }
            if(!setAddress(&writeAddress, LUN, position+i)){
                return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
            }
            start_page = (position+i) / PageSizeBytes;
            sendCommand(PAGE_PROGRAM);
            sendAddress(writeAddress.col1);
            sendAddress(writeAddress.col2);
            sendAddress(writeAddress.row1);
            sendAddress(writeAddress.row2);
            sendAddress(writeAddress.row3);
            if(waitTimeout()){
                return MT29F_Errno::TIMEOUT;
            }
        }
        sendData(data[i]);
        if(waitTimeout()){
            return MT29F_Errno::TIMEOUT;
        }
    }
    sendCommand(PAGE_PROGRAM_CONFIRM);

    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }

    if(!getARDYstatus() || !getRDYstatus()){
        return MT29F_Errno::NOT_READY;
    }
    if(getFAILstatus()){
        return MT29F_Errno::FAIL_RECENT;
    }

    //FAIL STATUS is 0
    return MT29F_Errno::NONE;
}

MT29F_Errno MT29F::readNAND(uint8_t LUN, uint64_t position, uint8_t& data) {
    data = 0;
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    Address readAddress{};
    if(!setAddress(&readAddress, LUN, position)){
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);

    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }

    data = readData();
    return MT29F_Errno::NONE;
}

MT29F_Errno MT29F::readNAND(uint8_t LUN, uint64_t start_position, etl::span<uint8_t> data) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    size_t read_length = data.size();
    if(start_position+data.size()>MaxAllowedAddress){
        // The selected address cannot accomodate for sequentially reading the requested data length
        // truncating the length
        read_length = MaxAllowedAddress - start_position;
    }
    uint8_t start_page = (start_position / PageSizeBytes) % PagesPerBlock;
    Address readAddress{};
    if(!setAddress(&readAddress, LUN, start_position)) {
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);

    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }
    for(size_t i=0; i<read_length; i++){
        if( (((start_position+i)/PageSizeBytes)%PagesPerBlock) != start_page ){
            // Next address is on another page, the NAND module has to load the new page in its cache
            if(!setAddress(&readAddress, LUN, start_position+i)){
                return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
            }
            sendCommand(READ_MODE);
            sendAddress(readAddress.col1);
            sendAddress(readAddress.col2);
            sendAddress(readAddress.row1);
            sendAddress(readAddress.row2);
            sendAddress(readAddress.row3);
            sendCommand(READ_CONFIRM);
            start_page = (start_position+i)/PageSizeBytes;
            if(waitTimeout()){
                return MT29F_Errno::TIMEOUT;
            }
        }
        data[i] = readData();
        if(waitTimeout()){
            return MT29F_Errno::TIMEOUT;
        }
    }
    return MT29F_Errno::NONE;
}

MT29F_Errno MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    if(!getRDYstatus()){
        return MT29F_Errno::BUSY_IO;
    }
    if(block > BlocksPerLUN){
        // Selected block is out of bounds
        return MT29F_Errno::ADDRESS_OUT_OF_BOUNDS;
    }

    const uint8_t row1 = (block & 0x01) << 7;
    const uint8_t row2 = (block >> 1) & 0xff;
    const uint8_t row3 = ((block >> 9) & 0x07) | ((LUN & 0x01) << 3);

    sendCommand(ERASE_BLOCK);
    sendAddress(row1);
    sendAddress(row2);
    sendAddress(row3);
    sendCommand(ERASE_BLOCK_CONFIRM);

    if(waitTimeout()){
        return MT29F_Errno::TIMEOUT;
    }

    if(!getARDYstatus() || !getRDYstatus()){
        return MT29F_Errno::NOT_READY;
    }
    if(getFAILstatus()){
        return MT29F_Errno::FAIL_RECENT;
    }
    //FAIL STATUS is 0
    return MT29F_Errno::NONE;
}



bool MT29F::getRDYstatus(){
    sendCommand(READ_STATUS);
    uint8_t status = readData();

    const uint32_t start = xTaskGetTickCount();
    while((status&MASK_STATUS_RDY) == 0){
        status = readData();
        if((xTaskGetTickCount() - start) > TimeoutCycles){
            break;
        }
    }
    return (status&MASK_STATUS_RDY);
}

bool MT29F::getARDYstatus(){
    sendCommand(READ_STATUS);
    uint8_t status = readData();

    const uint32_t start = xTaskGetTickCount();
    while((status&MASK_STATUS_ARDY) == 0){
        status = readData();
        if((xTaskGetTickCount() - start) > TimeoutCycles){
            break;
        }
    }

    return (status&MASK_STATUS_ARDY);
}

bool MT29F::getFAILCstatus(){
    sendCommand(READ_STATUS);
    uint8_t status = readData();

    const uint32_t start = xTaskGetTickCount();
    while((status&MASK_STATUS_FAIL) != 0){
        status = readData();
        if((xTaskGetTickCount() - start) > TimeoutCycles){
            break;
        }
    }

    return (status&MASK_STATUS_FAIL);

}

bool MT29F::getFAILstatus(){
    sendCommand(READ_STATUS);
    uint8_t status = readData();

    const uint32_t start = xTaskGetTickCount();
    while((status&MASK_STATUS_FAILC) != 0){
        status = readData();
        if((xTaskGetTickCount() - start) > TimeoutCycles){
            break;
        }
    }

    return (status&MASK_STATUS_FAILC);

}
