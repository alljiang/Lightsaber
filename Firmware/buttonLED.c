
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
//#include "drivers/buttons.h"
#include "driverlib/timer.h"
#include "timerHandler.h"
#include "tm4c123gh6pm.h"

#define BUTTON_LED_OFF -2
#define BUTTON_LED_IDLE -1
#define BUTTON_LED_SOLID 0
#define BUTTON_LED_BLINK 1
#define BUTTON_LED_PULSE 2
#define BUTTON_LED_RAINBOW 3

int8_t CONFIG_BUTTON_LED_MODE = 0;

uint8_t CONFIG_BUTTON_LED_SOLID_R = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_G = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_B = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_A = 0;

uint8_t CONFIG_BUTTON_LED_BLINK_R = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_G = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_B = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_A = 0;
uint32_t CONFIG_BUTTON_LED_BLINK_ONPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_BLINK_OFFPERIOD = 0;

uint8_t CONFIG_BUTTON_LED_PULSE_R = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_G = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_B = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_LOWALPHA = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_HIGHALPHA = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_TOPPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD= 0;
uint32_t CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD = 0;

bool blink_isOn = false;
long blink_lastToggleMillis = 0;

int pulse_state = 0;
long pulse_lastStateMillis = 0;

void button_rgb(int redPercent, int greenPercent, int bluePercent, int brightnessPercent) {
    int genPeriod = 10000;

    int redPeriod = (genPeriod / 100 / 100) * redPercent * brightnessPercent / 3;
    int bluePeriod = (genPeriod / 100 / 100) * bluePercent * brightnessPercent / 3 + redPeriod;
    int greenPeriod = genPeriod - ((genPeriod / 100 / 100) * greenPercent * brightnessPercent / 3);

    if(redPeriod <= 0) redPeriod = 1;
    if(redPeriod >= genPeriod) redPeriod = genPeriod - 1;
    if(greenPeriod <= 0) greenPeriod = 1;
    if(greenPeriod >= genPeriod) greenPeriod = genPeriod - 1;
    if(bluePeriod <= 0) bluePeriod = 1;
    if(bluePeriod >= genPeriod) bluePeriod = genPeriod - 1;

    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, genPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, genPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, genPeriod);

    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, redPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, greenPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, bluePeriod);

    PWM0_INVERT_R |= 0b1001000;
    PWMOutputInvert(PWM0_BASE, PWM_OUT_6 | PWM_OUT_5, true);
    PWMOutputInvert(PWM0_BASE, PWM_OUT_3, false);

    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_5_BIT | PWM_OUT_6_BIT, true);
}

void ButtonLED_setSolid(int r, int g, int b, int alpha) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_SOLID;

    CONFIG_BUTTON_LED_SOLID_R = r;
    CONFIG_BUTTON_LED_SOLID_G = g;
    CONFIG_BUTTON_LED_SOLID_B = b;
    CONFIG_BUTTON_LED_SOLID_A = alpha;
}

void ButtonLED_setBlink(int r, int g, int b, int alpha, int onPeriod, int offPeriod) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_BLINK;

    CONFIG_BUTTON_LED_BLINK_R = r;
    CONFIG_BUTTON_LED_BLINK_G = g;
    CONFIG_BUTTON_LED_BLINK_B = b;
    CONFIG_BUTTON_LED_BLINK_A = alpha;
    CONFIG_BUTTON_LED_BLINK_ONPERIOD = onPeriod;
    CONFIG_BUTTON_LED_BLINK_OFFPERIOD = offPeriod;

    blink_isOn = false;
    blink_lastToggleMillis = 0;
}

void ButtonLED_setPulse(int r, int g, int b, int lowAlpha, int highAlpha, int rampUpPeriod, int topPeriod, int rampDownPeriod, int bottomPeriod) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_PULSE;

    CONFIG_BUTTON_LED_PULSE_R = r;
    CONFIG_BUTTON_LED_PULSE_G = g;
    CONFIG_BUTTON_LED_PULSE_B = b;
    CONFIG_BUTTON_LED_PULSE_LOWALPHA = lowAlpha;
    CONFIG_BUTTON_LED_PULSE_HIGHALPHA = highAlpha;
    CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD = rampUpPeriod;
    CONFIG_BUTTON_LED_PULSE_TOPPERIOD = topPeriod;
    CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD= rampDownPeriod;
    CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD = bottomPeriod;

    pulse_state = 0;
    pulse_lastStateMillis = millis();
}

void buttonLEDHandler() {

    if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_IDLE) {}
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_OFF) {
        button_rgb(0,0,0,0);
        CONFIG_BUTTON_LED_MODE = BUTTON_LED_IDLE;
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_SOLID) {
        button_rgb(CONFIG_BUTTON_LED_SOLID_R, CONFIG_BUTTON_LED_SOLID_G, CONFIG_BUTTON_LED_SOLID_B, CONFIG_BUTTON_LED_SOLID_A);
        CONFIG_BUTTON_LED_MODE = BUTTON_LED_IDLE;
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_BLINK) {
        if(blink_isOn) {
            if(millis() - blink_lastToggleMillis >= CONFIG_BUTTON_LED_BLINK_ONPERIOD) {
                //  turn off
                button_rgb(0,0,0,0);
                blink_lastToggleMillis = millis();
                blink_isOn = false;
            }
        } else {
            if(millis() - blink_lastToggleMillis >= CONFIG_BUTTON_LED_BLINK_OFFPERIOD) {
                //  turn on
                button_rgb(CONFIG_BUTTON_LED_BLINK_R, CONFIG_BUTTON_LED_BLINK_G, CONFIG_BUTTON_LED_BLINK_B, CONFIG_BUTTON_LED_BLINK_A);
                blink_lastToggleMillis = millis();
                blink_isOn = true;
            }
        }
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_PULSE) {
        if(pulse_state == 0) {
            //  bottom
            if(millis() >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD) {
                pulse_state = 1;
                pulse_lastStateMillis = millis();
            }
            else {
                button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B, CONFIG_BUTTON_LED_PULSE_LOWALPHA);
            }
        }
        if(pulse_state == 1) {
            //  ramp up
            if(millis() >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD) {
                pulse_state = 2;
                pulse_lastStateMillis = millis();
            }
            else {
                uint8_t alpha = (CONFIG_BUTTON_LED_PULSE_HIGHALPHA - CONFIG_BUTTON_LED_PULSE_LOWALPHA) * (millis() - pulse_lastStateMillis) / CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD + CONFIG_BUTTON_LED_PULSE_LOWALPHA;
                button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B,  alpha);
            }
        }
        if(pulse_state == 2) {
            //  top
            if(millis() >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_TOPPERIOD) {
                pulse_state = 3;
                pulse_lastStateMillis = millis();
            }
            else {
                button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B, CONFIG_BUTTON_LED_PULSE_HIGHALPHA);
            }
        }
        if(pulse_state == 3) {
            //  ramp down
            if(millis() >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD) {
                pulse_state = 0;
                pulse_lastStateMillis = millis();
            }
            else {
                uint8_t alpha = CONFIG_BUTTON_LED_PULSE_HIGHALPHA - (CONFIG_BUTTON_LED_PULSE_HIGHALPHA - CONFIG_BUTTON_LED_PULSE_LOWALPHA) * (millis() - pulse_lastStateMillis) / CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD;
                button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B,  alpha);
            }
        }
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_RAINBOW) {

    }

}
