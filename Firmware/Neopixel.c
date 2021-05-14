#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "inc/hw_udma.h"
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
#include "driverlib/udma.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"

#include "pinConfig.h"
#include "Neopixel.h"

#include "TimeHandler.h"

#define transferSize 1024

// Gamma correction cuz our eyes are weird
const uint8_t GammaConversion[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

//  UDMA Control Table (must be 1024-byte aligned)
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t DMAcontroltable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(DMAcontroltable, 1024)
uint8_t DMAcontroltable[1024];
#else
uint8_t DMAcontroltable[1024] __attribute__ ((aligned(1024)));
#endif

// Global RGB values set by app
extern uint8_t primaryR;
extern uint8_t primaryG;
extern uint8_t primaryB;
extern uint8_t secondaryR;
extern uint8_t secondaryG;
extern uint8_t secondaryB;

// Current blade RGB colors
uint8_t bladeRed1;
uint8_t bladeGreen1;
uint8_t bladeBlue1;
uint8_t bladeRed2;
uint8_t bladeGreen2;
uint8_t bladeBlue2;
uint8_t bladePattern = 0;
uint16_t bladeEffectDelay;

// Store current and next Neopixel colors
uint32_t bladeColors[NEOPIXEL_COUNT] = {0}; // 24-bit RGB
int Neopixel_Update_Flag = 0;

// Variables for extend and retract
uint8_t hidePixels[NEOPIXEL_COUNT] = {0};   // 1 = off, 0 = normal
uint8_t movementMode = NO_MOTION;           // NO_MOTION, EXTEND_MODE, RETRACT_MODE

// LED Colors, 24-bit color: GGGGGGGG RRRRRRRR BBBBBBBB
int bytesBuffered;
int channel;
uint8_t data[NEOPIXEL_COUNT*24 + 1024 + 50*24];

extern void Neopixel_DMAInterrupt() {
    SSIIntClear(SSI1_BASE, SSI_DMATX);
//    uint8_t mode1 = uDMAChannelModeGet(UDMA_CH25_SSI1TX | UDMA_PRI_SELECT);
//    uint8_t mode2 = uDMAChannelModeGet(UDMA_CH25_SSI1TX | UDMA_ALT_SELECT);

    int bytesRemaining = sizeof(data) - bytesBuffered;
    int bytesToSend = bytesRemaining;
    if(bytesToSend > transferSize) bytesToSend = transferSize;

    if(bytesToSend > 0) {
        if(channel == 0) {
            channel = 1;
            //  update primary
            uDMAChannelTransferSet(UDMA_CHANNEL_SSI1TX | UDMA_PRI_SELECT,
                                   UDMA_MODE_PINGPONG, data+bytesBuffered, (void *)(SSI1_BASE + SSI_O_DR), bytesToSend);
            uDMAChannelEnable(UDMA_CHANNEL_SSI1TX);
        } else {
            channel = 0;
            //  update secondary
            uDMAChannelTransferSet(UDMA_CHANNEL_SSI1TX | UDMA_ALT_SELECT,
                                   UDMA_MODE_PINGPONG, data+bytesBuffered, (void *)(SSI1_BASE + SSI_O_DR), bytesToSend);
            uDMAChannelEnable(UDMA_CHANNEL_SSI1TX);
        }
        bytesBuffered += bytesToSend;
    }
    else {
//        if(mode1 == mode2 && mode1 == UDMA_MODE_STOP) {
            //  both primary and alternate are complete
            uDMAChannelDisable(UDMA_CHANNEL_SSI1TX);
            GPIOPinTypeGPIOOutput(Neopixel_b, Neopixel_p);
            GPIOPinWrite(Neopixel_b, Neopixel_p, 0);
//        }
    }
}

void Neopixel_initialize() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1)) {}

    GPIOPinConfigure(GPIO_PF1_SSI1TX);
    GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinConfigure(GPIO_PF2_SSI1CLK);
    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);

    SSIDMAEnable(SSI1_BASE, SSI_DMA_TX);
    SSIIntEnable(SSI1_BASE, SSI_DMATX);
//    IntPrioritySet(SSI1_BASE, 1);
    SSIIntRegister(SSI1_BASE, Neopixel_DMAInterrupt);

    SSIConfigSetExpClk(SSI1_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, 80000000/12, 8);
//    SSIConfigSetExpClk(SSI1_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, 8000000, 8);
    SSIEnable(SSI1_BASE);

    //  initialize DMA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);

    uDMAEnable();
    IntEnable(INT_UDMA);
    IntPrioritySet(INT_UDMA, 2);
    uDMAControlBaseSet(&DMAcontroltable[0]);

    uDMAChannelAssign(UDMA_CH25_SSI1TX);
    uDMAChannelAttributeDisable(UDMA_CHANNEL_SSI1TX, UDMA_ATTR_ALL);
    uDMAChannelAttributeEnable(UDMA_CHANNEL_SSI1TX, UDMA_ATTR_HIGH_PRIORITY);

    uDMAChannelControlSet(UDMA_CHANNEL_SSI1TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                            UDMA_DST_INC_NONE | UDMA_ARB_1024);
    uDMAChannelControlSet(UDMA_CHANNEL_SSI1TX | UDMA_ALT_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                            UDMA_DST_INC_NONE | UDMA_ARB_1024);

    //  set data buffer to a known state
     Neopixel_setSolid(0, 0, 0);

    //  clear data table
//    for(int i = 0; i < sizeof(data); i++) data[i] = 0;
}

uint32_t volatile prevUpdate = 0;
void Neopixel_update() {
    // Make sure previous update was more than 5 ms ago, otherwise throw out
    uint32_t time = millis();
    if (time - prevUpdate < 6)
        return;
    prevUpdate = time;

    for(int i = 0; i < 1024; i++) data[i] = 0;
    for(int i = 20*24; i > 0; i--) {
        int x = sizeof(data)-i;
        data[x] = 0;
    }

    channel = 0;

    GPIOPinConfigure(GPIO_PF1_SSI1TX);
    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);
    bytesBuffered = transferSize*2;

    uDMAChannelControlSet(UDMA_CHANNEL_SSI1TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                            UDMA_DST_INC_NONE | UDMA_ARB_1024);
    uDMAChannelControlSet(UDMA_CHANNEL_SSI1TX | UDMA_ALT_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                            UDMA_DST_INC_NONE | UDMA_ARB_1024);
    uDMAChannelTransferSet(UDMA_CHANNEL_SSI1TX | UDMA_PRI_SELECT,
                           UDMA_MODE_PINGPONG, data, (void *)(SSI1_BASE + SSI_O_DR), transferSize);
    uDMAChannelTransferSet(UDMA_CHANNEL_SSI1TX | UDMA_ALT_SELECT,
                           UDMA_MODE_PINGPONG, data+transferSize, (void *)(SSI1_BASE + SSI_O_DR), transferSize);
    uDMAChannelEnable(UDMA_CHANNEL_SSI1TX);
}

void Neopixel_setPixel(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b) {
    if (pixel > NEOPIXEL_COUNT) return;
    // Hide pixels for extend and retract motions
    if (hidePixels[pixel]) {
        r = 0; g = 0; b = 0;
    }
    // Account for gamma correction
    if (GAMMA_CORRECTION) {
        r = GammaConversion[r]; g = GammaConversion[g]; b = GammaConversion[b];
    }

    uint32_t grb = (g << 16) | (r << 8) | b;
    // Prepare data[] for transmit
    for (int8_t i = 23; i >= 0; i--) {
        volatile uint8_t convert = 0b11100000;
        if ((grb >> i) & 0x1) convert = 0b11111110;
        data[pixel*24 + (23-i) + 1024] = convert;
    }
}

// ===============================================
//           -=-=-=  E X T E N D  =-=-=-
// ===============================================
uint8_t extendingNeopixel = 0;
uint8_t extendEffectDelay = 10; // millis 10
void Neopixel_setExtend(void) {
    movementMode = EXTEND_MODE;
    extendingNeopixel = 0;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        hidePixels[i] = 1;
    }
}
void Neopixel_doExtend(void) {
    if (extendingNeopixel >= NEOPIXEL_COUNT)
        movementMode = NO_MOTION;
    extendingNeopixel += 2;
    hidePixels[extendingNeopixel-1] = 0;
    hidePixels[extendingNeopixel-2] = 0;
    for (int i = 0; i < extendingNeopixel; i+=2) {
        Neopixel_setPixel(i, get_red(bladeColors[i]), get_green(bladeColors[i]), get_blue(bladeColors[i]));
        if (i+1 < NEOPIXEL_COUNT)
            Neopixel_setPixel(i+1, get_red(bladeColors[i+1]), get_green(bladeColors[i+1]), get_blue(bladeColors[i+1]));
    }
    for (int i = extendingNeopixel; i < NEOPIXEL_COUNT; i+=2) {
        Neopixel_setPixel(i, 0, 0, 0);
        if (i+1 < NEOPIXEL_COUNT)
            Neopixel_setPixel(i+1, 0, 0, 0);
    }

}

// ===============================================
//           -=-=-=  R E T R A C T  =-=-=-
// ===============================================
volatile int16_t retractingNeopixel;
uint8_t retractEffectDelay = 8; // millis 8
void Neopixel_setRetract(void) {
    movementMode = RETRACT_MODE;
    retractingNeopixel = NEOPIXEL_COUNT;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        hidePixels[extendingNeopixel] = 0;
    }
}
void Neopixel_doRetract(void) {
    retractingNeopixel -= 2;
    if (retractingNeopixel >= 0)
        hidePixels[retractingNeopixel] = 1;
    hidePixels[retractingNeopixel+1] = 1;
    for (int i = 0; i < retractingNeopixel; i++) {
        if (i >= 0)
            Neopixel_setPixel(i, get_red(bladeColors[i]), get_green(bladeColors[i]), get_blue(bladeColors[i]));
        Neopixel_setPixel(i+1, get_red(bladeColors[i+1]), get_green(bladeColors[i+1]), get_blue(bladeColors[i+1]));
    }
    for(int i = retractingNeopixel; i < NEOPIXEL_COUNT; i++) {
        if (i >= 0)
            Neopixel_setPixel(i, 0, 0, 0);
        Neopixel_setPixel(i+1, 0, 0, 0);
    }
    if (retractingNeopixel <= 0) {
        hidePixels[0] = 1;
        movementMode = NO_MOTION;
    }
}

// ===============================================
//            -=-=-=  O F F  =-=-=-
// ===============================================
void Neopixel_setOff(void) {
    bladePattern = SOLID_PATTERN;
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        Neopixel_setPixel(i, 0, 0, 0);
    }
    bladeEffectDelay = 1000; // millis
}

// ===============================================
//           -=-=-=  S O L I D  =-=-=-
// ===============================================
void Neopixel_setSolid(uint8_t r, uint8_t g, uint8_t b) {
    bladePattern = SOLID_PATTERN;
    bladeRed1 = r; bladeGreen1 = g; bladeBlue1 = b;
    bladeEffectDelay = 80; // millis
}
volatile uint32_t lprev = 0;
void Neopixel_doSolid(void) {
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        bladeColors[i] = rgb_to_int(bladeRed1, bladeGreen1, bladeBlue1);
        Neopixel_setPixel(i, bladeRed1, bladeGreen1, bladeBlue1);
    }
    uint32_t time = millis();
    lprev = time;
}

// ===============================================
//            -=-=-=  S P A R K  =-=-=-
// ===============================================
#define INVERSE_SPARK_FREQ 50 // higher = less sparks. recommend 50

void Neopixel_setSpark(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
    bladePattern = SPARK_PATTERN;
    bladeRed1 = r1; bladeGreen1 = g1; bladeBlue1 = b1;
    bladeRed2 = r2; bladeGreen2 = g2; bladeBlue2 = b2;
    bladeEffectDelay = 60; // millis
}

void Neopixel_doSpark(void) {
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        int spark = rand() % INVERSE_SPARK_FREQ;
        if (spark < 1) {
            // Set pixel to secondary color
            bladeColors[i] = rgb_to_int(bladeRed2, bladeGreen2, bladeBlue2);
            Neopixel_setPixel(i, bladeRed2, bladeGreen2, bladeBlue2);
        } else {
            // Set pixel to primary color
            bladeColors[i] = rgb_to_int(bladeRed1, bladeGreen1, bladeBlue1);
            Neopixel_setPixel(i, bladeRed1, bladeGreen1, bladeBlue1);
        }
    }
}

// ===============================================
//           -=-=-=  F L A M E  =-=-=-
// ===============================================
#define FLAME_COOLING1 50                   // how fast primary flame cools down. suggested = 50
#define FLAME_COOLING2 75                   // how fast secondary flame cools down. suggested = 75
#define MAX_HEAT 200                        // best to leave this alone
#define SpawnFlameArea (NEOPIXEL_COUNT/8);  // largest Neopixel that a flame can start from
uint8_t heatLevel[NEOPIXEL_COUNT] = {0};   // number from 0 to MAX_HEAT to indicate temperature
uint8_t heatLevel2[NEOPIXEL_COUNT] = {0};  // number from 0 to MAX_HEAT to indicate temperature

void Neopixel_setFlame(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
    bladePattern = FLAME_PATTERN;
    bladeRed1 = r1; bladeGreen1 = g1; bladeBlue1 = b1;
    bladeRed2 = r2; bladeGreen2 = g2; bladeBlue2 = b2;
    bladeEffectDelay = 40; // millis
}

void Neopixel_doFlame(void) {
    // Ignite new flames near bottom
    if (rand() % 10 < 11) { // Determine % time spawns new flames. Higher right num = more often
        // Primary color
        uint8_t ignite = 0;
        if (rand() % 10 < 3)
            ignite = rand() % SpawnFlameArea;
        // Set two LEDs to high brightness
        uint16_t newHeat = heatLevel[ignite] + rand() % (MAX_HEAT/2) + MAX_HEAT/2;
        uint16_t newHeat2 = heatLevel[ignite+1] + rand() % (MAX_HEAT/2) + MAX_HEAT/2;
        heatLevel[ignite] = newHeat > MAX_HEAT? MAX_HEAT : newHeat;
        heatLevel[ignite+1] = newHeat2 > MAX_HEAT? MAX_HEAT : newHeat2;
    }
    if (rand() % 10 < 5) { // Determine % time spawns new flames. Higher right num = more often
        uint16_t ignite = 0;
        if (rand() % 10 < 2)
            ignite = rand() % SpawnFlameArea;
        // Set two LEDs to high brightness
        int newHeat = heatLevel2[ignite] + rand() % (MAX_HEAT/2) + MAX_HEAT/2;
        int newHeat2 = heatLevel2[ignite+1] + rand() % (MAX_HEAT/2) + MAX_HEAT/2;
        heatLevel2[ignite] = newHeat > MAX_HEAT? MAX_HEAT : newHeat;
        heatLevel2[ignite+1] = newHeat2 > MAX_HEAT? MAX_HEAT : newHeat2;
    }
    // Heat drifts up and diffuses
    for (uint8_t i = NEOPIXEL_COUNT - 1; i >= 2; i--) {
        heatLevel[i] = (heatLevel[i-1] + heatLevel[i-2]*2) / 3;     // primary color
        heatLevel2[i] = (heatLevel2[i-1] + heatLevel2[i-2]*2) / 3;  // secondary color
    }
    // Cool down every cell
    for (uint8_t i = 0; i < NEOPIXEL_COUNT; i++) {
        uint8_t cooling = 2*(rand() % FLAME_COOLING1) / i + 1;
        heatLevel[i] = cooling > heatLevel[i] ? 0 : heatLevel[i] - cooling;     // primary color
        cooling = 2*(rand() % FLAME_COOLING2) / i + 1;
        heatLevel2[i] = cooling > heatLevel2[i] ? 0 : heatLevel2[i] - cooling;  // secondary color
    }
    // Update Neopixels
    for (uint8_t i = 0; i < NEOPIXEL_COUNT; i++) {
        uint16_t red1, green1, blue1;
        red1 = bladeRed1*heatLevel[i]/MAX_HEAT; green1 = bladeGreen1*heatLevel[i]/MAX_HEAT; blue1 = bladeBlue1*heatLevel[i]/MAX_HEAT;
        uint16_t red2, green2, blue2;
        red2 = bladeRed2*heatLevel2[i]/MAX_HEAT; green2 = bladeGreen2*heatLevel2[i]/MAX_HEAT; blue2 = bladeBlue2*heatLevel2[i]/MAX_HEAT;
        // set pixel
//        uint16_t heatlvl2 = heatLevel2[i]/MAX_HEAT;
//        uint16_t red, green, blue;
//        if (heatlvl2 > 10) {
//            red = red2*heatlvl2 + red1*(1-heatlvl2);
//            green = green2*heatlvl2 + green1*(1-heatlvl2);
//            blue = blue2*heatlvl2 + blue1*(1-heatlvl2);
//            Neopixel_setPixel(i, red, green, blue);
//        } else {
//            Neopixel_setPixel(i, red1, green1, blue1);
//        }
        if (100*heatLevel2[i]/MAX_HEAT > 5) {
            bladeColors[i] = rgb_to_int(red2, green2, blue2);
            Neopixel_setPixel(i, red2, green2, blue2);
        } else {
            bladeColors[i] = rgb_to_int(red1, green1, blue1);
            Neopixel_setPixel(i, red1, green1, blue1);
        }
    }
}

// ===============================================
//      -=-=-=  R A I N B O W  =-=-=-
// ===============================================
uint8_t Rainbow_Increments;
uint32_t FirstRainbowState = 0;  // used to determine next color
void Neopixel_setRainbow(uint8_t nyan_flag) {
    bladePattern = RAINBOW_PATTERN;
    if (nyan_flag) {
        Rainbow_Increments = 10;
        bladeEffectDelay = 8; // millis
    } else {
        Rainbow_Increments = 40;
        bladeEffectDelay = 20; // millis
    }
}

void Neopixel_doRainbow(void) {
    uint32_t color1, color2;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        uint32_t RainbowState = (FirstRainbowState + i) % (6*Rainbow_Increments);
        if (RainbowState < Rainbow_Increments) { // red -> orange
            color1 = RED_COLOR;
            color2 = ORANGE_COLOR;
        } else if (RainbowState < 2*Rainbow_Increments) { // orange -> yellow
            color1 = ORANGE_COLOR;
            color2 = YELLOW_COLOR;
        } else if  (RainbowState < 3*Rainbow_Increments) { // yellow -> green
            color1 = YELLOW_COLOR;
            color2 = GREEN_COLOR;
        } else if (RainbowState < 4*Rainbow_Increments) { // green -> blue
            color1 = GREEN_COLOR;
            color2 = BLUE_COLOR;
        } else if (RainbowState < 5*Rainbow_Increments) { // blue -> purple
            color1 = BLUE_COLOR;
            color2 = PURPLE_COLOR;
        } else if (RainbowState < 6*Rainbow_Increments) { // purple -> red
            color1 = PURPLE_COLOR;
            color2 = RED_COLOR;
        }

        // Calculate RGB from the two transition colors
        uint16_t step = RainbowState % Rainbow_Increments;
        int16_t red = get_red(color1) - ((int16_t) get_red(color1) - (int16_t) get_red(color2))*step/Rainbow_Increments;
        int16_t green = get_green(color1) - ((int16_t) get_green(color1) - (int16_t) get_green(color2))*step/Rainbow_Increments;
        int16_t blue = get_blue(color1) - ((int16_t) get_blue(color1) - (int16_t) get_blue(color2))*step/Rainbow_Increments;

        bladeColors[NEOPIXEL_COUNT-1-i] = rgb_to_int(red, green, blue);
        Neopixel_setPixel(NEOPIXEL_COUNT-1-i, red, green, blue);
    }
    if (++FirstRainbowState >= 6*Rainbow_Increments) {
        FirstRainbowState = 0;
    }
}

// ===============================================
// -=-=-=  R A I N B O W     P U L S E  =-=-=-
// ===============================================
#define RAINBOW_PULSE_INCREMENTS 30
uint32_t RainbowPulseState = 0;  // used to determine next color
void Neopixel_setRainbowPulse(void) {
    bladePattern = RAINBOW_PULSE_PATTERN;
    bladeEffectDelay = 40; // millis
}
void Neopixel_doRainbowPulse(void) {
    uint32_t color1, color2;
    if (RainbowPulseState < RAINBOW_PULSE_INCREMENTS) { // red -> orange
        color1 = RED_COLOR;
        color2 = ORANGE_COLOR;
    } else if (RainbowPulseState < 2*RAINBOW_PULSE_INCREMENTS) { // orange -> yellow
        color1 = ORANGE_COLOR;
        color2 = YELLOW_COLOR;
    } else if  (RainbowPulseState < 3*RAINBOW_PULSE_INCREMENTS) { // yellow -> green
        color1 = YELLOW_COLOR;
        color2 = GREEN_COLOR;
    } else if (RainbowPulseState < 4*RAINBOW_PULSE_INCREMENTS) { // green -> blue
        color1 = GREEN_COLOR;
        color2 = BLUE_COLOR;
    } else if (RainbowPulseState < 5*RAINBOW_PULSE_INCREMENTS) { // blue -> purple
        color1 = BLUE_COLOR;
        color2 = PURPLE_COLOR;
    } else if (RainbowPulseState < 6*RAINBOW_PULSE_INCREMENTS) { // purple -> red
        color1 = PURPLE_COLOR;
        color2 = RED_COLOR;
    }

    // Calculate RGB from the two transition colors
    uint16_t step = RainbowPulseState % RAINBOW_PULSE_INCREMENTS;
    int16_t red = get_red(color1) - ((int16_t) get_red(color1) - (int16_t) get_red(color2))*step/RAINBOW_PULSE_INCREMENTS;
    int16_t green = get_green(color1) - ((int16_t) get_green(color1) - (int16_t) get_green(color2))*step/RAINBOW_PULSE_INCREMENTS;
    int16_t blue = get_blue(color1) - ((int16_t) get_blue(color1) - (int16_t) get_blue(color2))*step/RAINBOW_PULSE_INCREMENTS;

    if (++RainbowPulseState >= 6*RAINBOW_PULSE_INCREMENTS) {
        RainbowPulseState = 0;
    }
    // Set Neopixel colors
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        bladeColors[i] = rgb_to_int(red, green, blue);
        Neopixel_setPixel(i, red, green, blue);
    }
}

// ===============================================
// -=-=-=  R A I N B O W     S T R O B E  =-=-=-
// ===============================================
#define RAINBOW_STROBE_INCREMENTS 2
#define MIN_BRIGHTNESS 100
uint8_t lastStrobeState = 0;
void Neopixel_setRainbowStrobe(void) {
    bladePattern = RAINBOW_STROBE_PATTERN;
    bladeEffectDelay = 30; // millis
}

void Neopixel_doRainbowStrobe(void) {
    uint32_t color1, color2;
    uint8_t strobeState = (rand() % 6) *RAINBOW_STROBE_INCREMENTS;
    while (strobeState == lastStrobeState)
        strobeState = (rand() % 6) *RAINBOW_STROBE_INCREMENTS;
    lastStrobeState = strobeState;
    if (strobeState < RAINBOW_STROBE_INCREMENTS) { // red -> orange
        color1 = RED_COLOR;
        color2 = ORANGE_COLOR;
    } else if (strobeState < 2*RAINBOW_STROBE_INCREMENTS) { // orange -> yellow
        color1 = ORANGE_COLOR;
        color2 = YELLOW_COLOR;
    } else if  (strobeState < 3*RAINBOW_STROBE_INCREMENTS) { // yellow -> green
        color1 = YELLOW_COLOR;
        color2 = GREEN_COLOR;
    } else if (strobeState < 4*RAINBOW_STROBE_INCREMENTS) { // green -> blue
        color1 = GREEN_COLOR;
        color2 = BLUE_COLOR;
    } else if (strobeState < 5*RAINBOW_STROBE_INCREMENTS) { // blue -> purple
        color1 = BLUE_COLOR;
        color2 = PURPLE_COLOR;
    } else if (strobeState < 6*RAINBOW_STROBE_INCREMENTS) { // purple -> red
        color1 = PURPLE_COLOR;
        color2 = RED_COLOR;
    }
    // Calculate RGB from the two transition colors
    uint16_t step = strobeState % RAINBOW_STROBE_INCREMENTS;
    uint8_t red = get_red(color1) - (get_red(color1) - get_red(color2))*step/RAINBOW_STROBE_INCREMENTS;
    uint8_t green = get_green(color1) - (get_green(color1) - get_green(color2))*step/RAINBOW_STROBE_INCREMENTS;
    uint8_t blue = get_blue(color1) - (get_blue(color1) - get_blue(color2))*step/RAINBOW_STROBE_INCREMENTS;
    // Set Neopixel colors
    for(int i = 0; i < NEOPIXEL_COUNT; i++) {
        bladeColors[i] = rgb_to_int(red, green, blue);
        Neopixel_setPixel(i, red, green, blue);
    }
}

// ===============================================
//        -=-=-=  U N S T A B L E  =-=-=-
// ===============================================
uint8_t minUnstableBright = 70; // lowest % brightness from 0 to 100
void Neopixel_setUnstable(uint8_t r, uint8_t g, uint8_t b) {
    bladePattern = UNSTABLE_PATTERN;
    bladeRed1 = r; bladeGreen1 = g; bladeBlue1 = b;
    bladeEffectDelay = 30; // millis
}
void Neopixel_doUnstable(void) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        uint8_t brightness = rand() % (101 - minUnstableBright) + minUnstableBright;
        uint8_t red = bladeRed1*brightness/100;
        uint8_t green = bladeGreen1*brightness/100;
        uint8_t blue = bladeBlue1*brightness/100;
        bladeColors[i] = rgb_to_int(red, green, blue);
        Neopixel_setPixel(i, red, green, blue);
    }
}

// ===============================================
//       -=-=-=  E N E R G E T I C  =-=-=-
// ===============================================
uint8_t energyGrow; // indicate on or off
int16_t energeticState;
uint8_t energy_max_red, energy_max_green, energy_max_blue;
#define energyInc 5 // how many gradients from low to max bright
#define addEnergy 25 // add 0 - 255 to rgb for pulse
void Neopixel_setEnergetic(uint8_t r, uint8_t g, uint8_t b) {
    bladePattern = ENERGETIC_PATTERN;
    bladeRed1 = r*8/10; bladeGreen1 = g*8/10; bladeBlue1 = b*8/10;
    energyGrow = 1;
    energeticState = 0;
    energy_max_red = bladeRed1 + addEnergy > 255 ? 255 : bladeRed1 + addEnergy;
    energy_max_green = bladeGreen1 + addEnergy > 255 ? 255 : bladeGreen1 + addEnergy;
    energy_max_blue = bladeBlue1 + addEnergy > 255 ? 255 : bladeBlue1 + addEnergy;
    bladeEffectDelay = 10; // millis
}
void Neopixel_doEnergetic(void) {
    uint8_t diff_red = (energy_max_red - bladeRed1) * energeticState/energyInc;
    uint8_t diff_green = (energy_max_green - bladeGreen1) * energeticState/energyInc;
    uint8_t diff_blue = (energy_max_blue - bladeBlue1) * energeticState/energyInc;

    if (energyGrow) {
        // increasing in brightness
        for(int i = 0; i < NEOPIXEL_COUNT; i++) {
            bladeColors[i] = rgb_to_int(bladeRed1+diff_red, bladeGreen1+diff_green, bladeBlue1+diff_blue);
            Neopixel_setPixel(i, bladeRed1+diff_red, bladeGreen1+diff_green, bladeBlue1+diff_blue);
        }
        if (energeticState == energyInc)
            energyGrow = 0;
        else
            energeticState++;
    } else {
        // decreasing in brightness
        for(int i = 0; i < NEOPIXEL_COUNT; i++) {
            bladeColors[i] = rgb_to_int(bladeRed1+diff_red, bladeGreen1+diff_green, bladeBlue1+diff_blue);
            Neopixel_setPixel(i, bladeRed1+diff_red, bladeGreen1+diff_green, bladeBlue1+diff_blue);
        }
        if (energeticState == 0)
            energyGrow = 1;
        else
            energeticState--;
    }
}

uint32_t volatile prevTime = 0;
uint32_t volatile moovTime = 0;
void Neopixel_handler() {
    uint32_t time = millis();
    // Movement
    if (movementMode == EXTEND_MODE && time - moovTime > extendEffectDelay) {
        moovTime = time;
        Neopixel_doExtend();
        Neopixel_update();
    } else if (movementMode == RETRACT_MODE && time - moovTime > retractEffectDelay) {
        moovTime = time;
        Neopixel_doRetract();
        Neopixel_update();
    }
    // LED patterns
    if (bladePattern == SOLID_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doSolid();
        Neopixel_update();
    } else if (bladePattern == SPARK_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doSpark();
        Neopixel_update();
    } else if (bladePattern == FLAME_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doFlame();
        Neopixel_update();
    } else if (bladePattern == RAINBOW_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doRainbow();
        Neopixel_update();
    } else if (bladePattern == RAINBOW_PULSE_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doRainbowPulse();
        Neopixel_update();
    } else if (bladePattern == RAINBOW_STROBE_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doRainbowStrobe();
        Neopixel_update();
    } else if (bladePattern == UNSTABLE_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doUnstable();
        Neopixel_update();
    } else if (bladePattern == ENERGETIC_PATTERN && time - prevTime > bladeEffectDelay) {
        prevTime = time;
        Neopixel_doEnergetic();
        Neopixel_update();
    }
}
