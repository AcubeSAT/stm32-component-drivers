#include "NANDFlash.h"

//uint8_t MT29F::initialize(){
//
//    uint8_t data;
//    while(1) {
//        if (PIO_PinRead(PIO_PIN_PA12)) {
//            sendCommand(0xFF);
//            while (1) {
//                if (PIO_PinRead(PIO_PIN_PA12)) {
//                    sendCommand(0x70);
//                    break;
//                }
//            }
//            PIO_PinWrite(NCS, false);
//            for(int i = 0; i < 10; i++){
//                data = *((volatile uint8_t *) dataRegister);
//            }
//            break;
//        }
//    }
//    return data;
//}


void MT29F::READ_ID() {
    uint8_t buffer[5];
    nandSendCommand(0x90);
    nandSendAddress(0x00);

    for(int i=0; i<5; i++) {
        buffer[i] = nandReadData();
    }
 return ;
}
