#include "AD590_Driver.hpp"

float AD590::getTemperature() {
    float result = 0;

    AFEC0_ChannelsEnable(AFEC_CH0_MASK);
    AFEC0_ChannelsEnable(AFEC_CH1_MASK);

    AFEC0_ConversionStart();

    if(AFEC0_ChannelResultIsReady(AFEC_CH0)){
        result = result = AFEC0_ChannelResultGet(AFEC_CH0);
    }
    else if(AFEC0_ChannelResultIsReady(AFEC_CH1)){
        result = result = AFEC0_ChannelResultGet(AFEC_CH1);
    }

    return result;
}
