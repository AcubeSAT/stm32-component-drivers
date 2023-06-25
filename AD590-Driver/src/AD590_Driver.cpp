#include "AD590_Driver.hpp"

float AD590::getTemperature() {
    uint16_t data = 0;
    float result = 0;

    AFEC0_ChannelsEnable(AFEC_CH0_MASK);
    AFEC0_ChannelsEnable(AFEC_CH1_MASK);

    AFEC0_ConversionStart();

    while(!AFEC0_ChannelResultIsReady(AFEC_CH0) || !AFEC0_ChannelResultIsReady(AFEC_CH1))
    {
    };

    if(AFEC0_ChannelResultIsReady(AFEC_CH0)){
        if (AFEC0_ChannelResultGet(AFEC_CH0) != NULL){
            data =  AFEC0_ChannelResultGet(AFEC_CH0);
        }
    }
    else if(AFEC0_ChannelResultIsReady(AFEC_CH1)){
        if (AFEC0_ChannelResultGet(AFEC_CH1) != NULL){
            data =  AFEC0_ChannelResultGet(AFEC_CH1);
        }
    }

    uint8_t upperByte = (data >> 8) & 0x1F;
    uint8_t lowerByte = data & 0xFF;

    if ((upperByte & 0x10) != 0) {
        upperByte &= 0x0F;
        result = -(256 - (upperByte * 16.0f + lowerByte / 16.0f));
    } else {
        result = upperByte * 16.0f + lowerByte / 16.0f;
    }

    return result;
}
