#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "tm4c123gh6pm.h"

#include "SoundFont.h"
#include "pinConfig.h"
#include "AmINyanCat.h"
#include "Audio.h"


#define AUDIO_BUFFER_SIZE 512

#ifndef NYANCAT
#define AUDIO_FREQUENCY 16000
#else
#define AUDIO_FREQUENCY 8000
#endif

int32_t playbackIndex[NUMBER_OF_FONTS];
uint8_t playbackLoop[NUMBER_OF_FONTS];
uint8_t playbackVolume[NUMBER_OF_FONTS];

//  input: [0, 4095]
//  vout: ((2.048 * input) / 4095) * 2
void Audio_sendData(uint16_t number) {
    // Set header. [15] ignore/write, [14] don't care, [13] vref/internal, [12] active/shutdown
    number = (0b1 << 12) | (number & 0xFFF); // set bit 12 and clear 13-15

    // Pull CS low and send SPI command
    GPIOPinWrite(CS_DAC_b, CS_DAC_p, 0);
    SSIDataPut(SSI0_BASE, number);
    while(SSIBusy(SSI0_BASE));
    GPIOPinWrite(CS_DAC_b, CS_DAC_p, CS_DAC_p);
}

extern void Audio_timerInterrupt() {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    int sum = 0;
    for(uint8_t i = 0; i < NUMBER_OF_FONTS; i++) {
        //  decide whether to play this sound or not
        int32_t index = playbackIndex[i];
        if(index == -1) continue;   //  not currently playing this sound

        //  calculate voltage level to add for this sound (centered around 0)
        int32_t currentLevel = FontPtrs[i][index];
        currentLevel = (playbackVolume[i] * (currentLevel-128));
        sum += currentLevel;

        //  update index for this sound
        index += 1;
        if(frameSizes[i] == index) {
            //  end of sound reached, determine whether to loop or stop
            if(playbackLoop[i]) playbackIndex[i] = 0;
            else playbackIndex[i] = -1;
        } else playbackIndex[i] = index;
    }

    sum *= 6;    //  rescale for max 3 things at once
    sum /= 100;  //  rescale, since volume for each has been scaled by 100
    sum += 1434;    //  center around 2048*70%
    if(sum > 2867) sum = 2867;  //  cap at 4095*70%
    if(sum < 0) sum = 0;

    //  play voltage
    Audio_sendData(sum);
}

void Audio_initialize(void) {
    // Initialize SSI0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)) {}

    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinTypeSSI(MOSI_b, MOSI_p);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinTypeSSI(SCK_b, SCK_p);

    GPIOPinTypeGPIOOutput(CS_DAC_b, CS_DAC_p);
    GPIOPinWrite(CS_DAC_b, CS_DAC_p, 0);

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 10000000, 16); // DAC settling time = 4.5 us
    SSIEnable(SSI0_BASE);

    //  initialize timer0 to 16kHz
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet()/AUDIO_FREQUENCY);
    TimerControlEvent(TIMER1_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);

    TimerIntRegister(TIMER1_BASE, TIMER_A, Audio_timerInterrupt);
    IntPrioritySet(INT_TIMER1A, 3);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER1_BASE, TIMER_A);
}

//  will overwrite currently playing music
//  volume: 100 = 100.0%, 0 = 0%
//  loop: 1 = playback loop, 0 = no playback loop
void Audio_play(uint8_t fontID, uint8_t volume, uint8_t loop) {
    playbackIndex[fontID] = 0;
    playbackVolume[fontID] = volume;
    playbackLoop[fontID] = loop;
}

int Audio_isPlaying(uint8_t fontID) {
    return playbackIndex[fontID] != -1;
}

void Audio_setVolume(uint8_t fontID, uint8_t volume) {
    playbackVolume[fontID] = volume;
}

void Audio_stop(uint8_t fontID) {
    playbackIndex[fontID] = -1;
}
