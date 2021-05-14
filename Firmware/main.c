
#include <PLL.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
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

#include "Audio.h"
#include "Button.h"
#include "EEPROM.h"
#include "ESP.h"
#include "IMU.h"
#include "Neopixel.h"
#include "Power.h"
#include "Wifi.h"
#include "Utils.h"

#include "pinConfig.h"
#include "OTFConfig.h"
#include "TimeHandler.h"

extern int Neopixel_Update_Flag;

uint32_t batteryVoltage = 0;

//  OTF Config saved here
volatile uint8_t primaryR;
volatile uint8_t primaryG;
volatile uint8_t primaryB;
volatile uint8_t secondaryR;
volatile uint8_t secondaryG;
volatile uint8_t secondaryB;
volatile uint8_t fontIndex;
volatile uint8_t bladeEffectIndex;

//  1000 Hz Handler
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    millisHandler();
}

void SysTickIntHandler(void) {
}

void setupPins() {

    // Unlock PF0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE+GPIO_O_CR) |= GPIO_PIN_0;

    // Enable Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
}

void setupInterrupts() {
    IntMasterDisable();
    //  Systick
//    SysTickPeriodSet(10);
//    SysTickPeriodSet(SysCtlClockGet() / 2);
//    SysTickIntEnable();
//    SysTickEnable();

    //  Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000); // 1000 Hz

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER0_BASE, TIMER_A);

    IntMasterEnable();
}

uint32_t last_ButtonLEDHandler = 0;
uint32_t last_VoltageHandler = 0;
uint32_t last_NeopixelHandler = 0;
uint32_t last_IMUHandler = 0;
uint32_t last_EEPROMHandler = 0;
uint32_t last_WifiHandler = 0;
uint32_t last_WifiInfoHandler = 0;
bool connectedToPhone = false;

void handlerLoop() {
    long time = millis();

    Neopixel_handler();

    if(time - last_ButtonLEDHandler >= 10) {
        last_ButtonLEDHandler = time;
        Button_Handler();
    }

    if(time - last_VoltageHandler >= 1000) {
        last_VoltageHandler = time;

//        batteryVoltage = Power_getBatteryVoltage(); //  scaled by 1000
//        if(x < 3400) {
//            Power_latchOff();
//        }
    }

    if(time - last_IMUHandler >= 2) {
        last_IMUHandler = time;
        IMU_update();
    }

    if(time - last_EEPROMHandler >= 1000) {
        last_EEPROMHandler = time;

        //  periodically update saved values
        EEPROM_setPrimaryColors(primaryR, primaryG, primaryB);
        EEPROM_setSecondaryColors(secondaryR, secondaryG, secondaryB);
        EEPROM_setSoundFontIndex(fontIndex);
        EEPROM_setBladeEffectIndex(bladeEffectIndex);
    }

    if(connectedToPhone) {
        if(time - last_WifiHandler >= 30) {
            last_WifiHandler = time;
            Wifi_handler();
        }
        if(time - last_WifiInfoHandler >= 1000) {
            last_WifiInfoHandler = time;
            Wifi_sendVoltage(batteryVoltage);
            Wifi_sendTemperature(IMU_getTemperature());
        }
    } else {
        //  try to connect to TCP client
        if(time - last_WifiHandler >= 1000) {
            last_WifiHandler = time;
            connectedToPhone = ESP_TryConnect();

        }
    }
}

volatile uint32_t lastStateChange = 0;
volatile uint8_t state = 0;
volatile uint32_t lastControlLoop = 0;
volatile uint32_t lastSwing = 0;
volatile uint32_t lastBtnUpdate = 0;

void controlLoop() {
    uint32_t time = millis();

    if(time - lastControlLoop < 5) return;
    lastControlLoop = time;
    uint32_t stateRuntime = time - lastStateChange;

    if(state == 0) {
        static bool startupTriggered = false;
        //  startup - must hold button for 1 second and release to finish startup and enter idle
        if(time > 1000) {
            Power_latchOn();
            if(!startupTriggered) Button_setBlink(100,100,100,100, 100, 100);
            startupTriggered = true;
            if(!Button_isPressed()) {   //  on button release, move to next state
                state = 1;
                Button_setPulse(50,50,50, 0, 20, 1000, 1000, 800, 700);
            }
        }
    } else if(state == 1) {
        //  idle - ready for button to start up
        static int8_t lastChargeMode = -1;

        if(time - lastBtnUpdate > 100) {
            lastBtnUpdate = time;

            uint8_t chargeMode = Power_isCharging();
            if(chargeMode != lastChargeMode) {
                lastChargeMode = chargeMode;

                if(chargeMode == NO_CHARGE) {
                    Button_setPulse(50,50,50, 0, 20, 1000, 1000, 800, 700);
                } else if(chargeMode == CHARGING) {
                    Button_setPulse(100,0,0, 0, 100, 500, 500, 400, 300);
                } else if(chargeMode == FULL_CHARGE) {
                    Button_setPulse(0,100,0, 0, 100, 1000, 1000, 800, 700);
                }
            }
        }

        if(Button_isPressed()) {
            state = 2;
            lastChargeMode = -1;
            lastStateChange = time;
        }
    } else if(state == 2) {
        //  button pressed in idle - check for blade on or power off
        if(stateRuntime > 3000) {    //  power off
            //  pressed long enough, show indicator for turn-off
            static bool powerOffTriggered = false;
            if(!powerOffTriggered) {
                powerOffTriggered = true;
                Button_setBlink(100,0,0,100, 100, 100);
            }
            if(!Button_isPressed()) {
                Power_latchOff();   //  kill on release
                state = 1;  //  if we get to this point, it means we are on USB power
            }
        } else {    //  blade on
            if(!Button_isPressed()) {
                //  button released, decide what to do
                lastStateChange = time;
                state = 10;
                // set blade effect/color
                if(bladeEffectIndex == 0) Neopixel_setSolid(primaryR, primaryG, primaryB);
                else if(bladeEffectIndex == 1) Neopixel_setUnstable(primaryR, primaryG, primaryB);
                else if(bladeEffectIndex == 2) Neopixel_setSpark(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB);
                else if(bladeEffectIndex == 3) Neopixel_setFlame(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB);
                else if(bladeEffectIndex == 4) Neopixel_setEnergetic(primaryR, primaryG, primaryB);
                else if(bladeEffectIndex == 5) Neopixel_setRainbow(0);
                else if(bladeEffectIndex == 6) Neopixel_setRainbowPulse();
                else if(bladeEffectIndex == 7) Neopixel_setRainbowStrobe();
                Neopixel_setExtend();
#ifndef NYANCAT
                if(fontIndex == 0) {
                    Audio_play(FONTID_A_EXTEND, 200, 0);
                } else if(fontIndex == 1) {
                    Audio_play(FONTID_B_EXTEND, 200, 0);
                }
#else
                Audio_play(FONTID_NYAN_HUM, 100, 1);
                Audio_stop(FONTID_NYAN_WAVE);
#endif
                Button_setBlink(primaryR*100/255, primaryG*100/255, primaryB*100/255, 100, 100, 100);
            }
        }

    } else if(state == 10) {
        //  blade extending

        //  TODO Start blade extension effect

#ifndef NYANCAT
        if(stateRuntime > 500 && (!Audio_isPlaying(FONTID_A_HUM)&&!Audio_isPlaying(FONTID_B_HUM))) {
            //  partially done with extension, start background hum
            if(fontIndex == 0) {
                Audio_play(FONTID_A_HUM, 150, 1);
                Audio_play(FONTID_A_SWING1H, 0, 1);
                Audio_play(FONTID_A_SWING1L, 0, 1);
                Audio_play(FONTID_A_SWING2H, 0, 1);
                Audio_play(FONTID_A_SWING2L, 0, 1);
            } else if(fontIndex == 1) {
                Audio_play(FONTID_B_HUM, 150, 1);
                Audio_play(FONTID_B_SWING1H, 0, 1);
                Audio_play(FONTID_B_SWING1L, 0, 1);
                Audio_play(FONTID_B_SWING2H, 0, 1);
                Audio_play(FONTID_B_SWING2L, 0, 1);
            }
        }
#else
#endif

        if(stateRuntime > 1000) {
            state = 11;
            lastStateChange = time;
        }

    } else if(state == 11) {
        //  blade idle

#ifndef NYANCAT
        // set blade effect/color
        if(bladeEffectIndex == 0) Neopixel_setSolid(primaryR, primaryG, primaryB);
        else if(bladeEffectIndex == 1) Neopixel_setUnstable(primaryR, primaryG, primaryB);
        else if(bladeEffectIndex == 2) Neopixel_setSpark(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB);
        else if(bladeEffectIndex == 3) Neopixel_setFlame(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB);
        else if(bladeEffectIndex == 4) Neopixel_setEnergetic(primaryR, primaryG, primaryB);
        else if(bladeEffectIndex == 5) Neopixel_setRainbow(0);
        else if(bladeEffectIndex == 6) Neopixel_setRainbowPulse();
        else if(bladeEffectIndex == 7) Neopixel_setRainbowStrobe();
#else
        if(stateRuntime >= 2350 && !Audio_isPlaying(FONTID_NYAN_WAVE)) {
            Audio_play(FONTID_NYAN_WAVE, 0, 1);
        }
        Neopixel_setRainbow(1);
#endif

        // set button effect/color
        if(time - lastBtnUpdate > 100) {
            lastBtnUpdate = time;
            if(bladeEffectIndex == 4 || bladeEffectIndex == 5 || bladeEffectIndex == 6) {
                Button_setSolid(100,100,100,100);
            }  else {
                Button_setSolid(primaryR*100/255, primaryG*100/255, primaryB*100/255, 100);
            }
        }

        //  ==============  SMOOTHSWING ===============
        rand(); //  just adds a bit more randomness to our collection
        int16_t humAudioVolume = 0;
        int16_t waveAudioVolume1 = 0;
        int16_t waveAudioVolume2 = 0;

#ifndef NYANCAT
        uint16_t maxVolumeSwingSpeed = 720;     //  dps
        uint16_t maxSwingVolume = 250;
        uint32_t swingActivateVelocity = 120;     //  dps
        uint32_t swingDeactivateVelocity = 60;     //  dps
        uint8_t transitionRegion1 = 45;    // degrees, must be < 180
        uint8_t transitionRegion2 = 150;    // degrees, must be < 180. Typically much larger than region1
        int16_t humVolume = 150;
        uint8_t minimumHumDuck = 25;    //  volume, out of 100
#else
        uint16_t maxVolumeSwingSpeed = 360;     //  dps
        uint16_t maxSwingVolume = 250;
        uint32_t swingActivateVelocity = 120;     //  dps
        uint32_t swingDeactivateVelocity = 60;     //  dps
        uint8_t transitionRegion1 = 45;    // degrees, must be < 180
        uint8_t transitionRegion2 = 150;    // degrees, must be < 180. Typically much larger than region1
        int16_t humVolume = 100;
        uint8_t minimumHumDuck = 25;    //  volume, out of 100
#endif

        // calculate swing strength
        uint32_t angularVelocity = IMU_getAngularVelocity();    // dps, scaled by 100
        uint32_t swingStrength;
        if(angularVelocity/100 > maxVolumeSwingSpeed) swingStrength = maxSwingVolume;
        else swingStrength =  angularVelocity * maxSwingVolume / 100 / maxVolumeSwingSpeed;     //  out of max swing volume
        swingStrength = sqrtInt(swingStrength*swingStrength*swingStrength)*maxSwingVolume / sqrtInt(maxSwingVolume*maxSwingVolume*maxSwingVolume);  // nonlinearity, basically to the power of 3/2

        static int8_t smoothswing_state = 0;
        static uint8_t waveAudioSelection = 0;
        static uint16_t transitionPoint1 = 0;    //  beginning of transition region 1
        static uint16_t transitionPoint2 = 0;    //  beginning of transition region 2
        static uint8_t loopingPair = 0;
        static uint32_t lastAccumulationTime = 0;
        static int accumulatedSwingAngle = 0;    //  degrees, scaled by 1000. Calculated with simple integration

        if(smoothswing_state == 0) {
            //  Not currently swinging

            humAudioVolume = humVolume;
            waveAudioVolume1 = 0;
            waveAudioVolume2 = 0;

            if(angularVelocity/100 >= swingActivateVelocity) {
                //  Swing started
                loopingPair = rand() % 2;
                waveAudioSelection = rand() % 2;

                uint8_t largestTransition = transitionRegion1 > transitionRegion2 ? transitionRegion1 : transitionRegion2;
                transitionPoint1 = rand() % (180 - largestTransition);
                transitionPoint2 = transitionPoint1 + 180;

                lastAccumulationTime = time;
                accumulatedSwingAngle = 0;

                smoothswing_state = 1;
            }
        } else {
            //  Currently swinging
            //  Accumulate swing angle
            accumulatedSwingAngle += angularVelocity * (time - lastAccumulationTime) / 2 / 100;
            accumulatedSwingAngle %= 360*1000;
            lastAccumulationTime = time;

            //  allow state to feed directly into the next state for a faster response
            if(smoothswing_state == 1) {
                //  Before transition region 1
                waveAudioVolume1 = waveAudioSelection ? 0 : 100;
                waveAudioVolume2 = waveAudioSelection ? 100 : 0;

                //  Check if in transition region 1 yet. Add an additional qualifier to account for when angle < 360 and right after transition 2
                if(accumulatedSwingAngle/1000 >= transitionPoint1 && accumulatedSwingAngle/1000 < transitionPoint2) {
                    smoothswing_state = 2;
                }
            }
            if(smoothswing_state == 2) {
                //  Transition Region 1
                int8_t fallingVolume = 100 - ( 100*(accumulatedSwingAngle/1000-transitionPoint1) / transitionRegion1 );
                int8_t risingVolume = 100*(accumulatedSwingAngle/1000-transitionPoint1) / transitionRegion1;

                waveAudioVolume1 = waveAudioSelection ? risingVolume : fallingVolume;
                waveAudioVolume2 = waveAudioSelection ? fallingVolume : risingVolume;

                if(accumulatedSwingAngle/1000 >= transitionPoint1 + transitionRegion1) {
                    smoothswing_state = 3;
                }
            }
            if(smoothswing_state == 3) {
                //  Between transition region 1 and 2
                waveAudioVolume1 = waveAudioSelection ? 100 : 0;
                waveAudioVolume2 = waveAudioSelection ? 0 : 100;

                //  Check if in transition region 2 yet
                if(accumulatedSwingAngle/1000 >= transitionPoint2) {
                    smoothswing_state = 4;
                }
            }
            if(smoothswing_state == 4) {
                //  Transition Region 2
                int8_t fallingVolume = 100 - ( 100*(accumulatedSwingAngle/1000-transitionPoint2) / transitionRegion2 );
                int8_t risingVolume = 100*(accumulatedSwingAngle/1000-transitionPoint2) / transitionRegion2;

                waveAudioVolume1 = waveAudioSelection ? fallingVolume : risingVolume;
                waveAudioVolume2 = waveAudioSelection ? risingVolume : fallingVolume;

                if(accumulatedSwingAngle/1000 >= transitionPoint2 + transitionRegion2) {
                    smoothswing_state = 1;
                }
            }

            //  modulate volume based on swing strength
            humAudioVolume = humVolume - swingStrength; if(humAudioVolume < minimumHumDuck) humAudioVolume = minimumHumDuck;
            waveAudioVolume1 = waveAudioVolume1 * swingStrength / 100;
            waveAudioVolume2 = waveAudioVolume2 * swingStrength / 100;

            //  check if still swinging
            if(angularVelocity/100 < swingDeactivateVelocity) {
                smoothswing_state = 0;
            }
        }

#ifndef NYANCAT
        if(fontIndex == 0) {
            Audio_setVolume(FONTID_A_HUM, humAudioVolume);
            Audio_setVolume(FONTID_A_SWING1H, loopingPair == 0 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_A_SWING1L, loopingPair == 0 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_A_SWING2H, loopingPair == 1 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_A_SWING2L, loopingPair == 1 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_B_HUM, humAudioVolume);
            Audio_setVolume(FONTID_B_SWING1H, loopingPair == 0 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_B_SWING1L, loopingPair == 0 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_B_SWING2H, loopingPair == 1 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_B_SWING2L, loopingPair == 1 ? waveAudioVolume2 : 0);
        } else if(fontIndex == 1) {
            Audio_setVolume(FONTID_B_HUM, humAudioVolume);
            Audio_setVolume(FONTID_B_SWING1H, loopingPair == 0 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_B_SWING1L, loopingPair == 0 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_B_SWING2H, loopingPair == 1 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_B_SWING2L, loopingPair == 1 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_A_HUM, humAudioVolume);
            Audio_setVolume(FONTID_A_SWING1H, loopingPair == 0 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_A_SWING1L, loopingPair == 0 ? waveAudioVolume2 : 0);
            Audio_setVolume(FONTID_A_SWING2H, loopingPair == 1 ? waveAudioVolume1 : 0);
            Audio_setVolume(FONTID_A_SWING2L, loopingPair == 1 ? waveAudioVolume2 : 0);
        }
#else
        int volume = 0;
        if(angularVelocity > 36000) volume = 200;
        else if(angularVelocity > 18000) volume = 100;
        Audio_setVolume(FONTID_NYAN_WAVE, volume + 30);
#endif

        if(Button_isPressed()) {
            state = 12;
            lastStateChange = time;
        }

    } else if(state == 12) {
        //  button pressed in blade mode
        if(!Button_isPressed()) {
            //  blade retracting
            Neopixel_setRetract();
            state = 19;
            lastStateChange = time;
#ifndef NYANCAT
            if(fontIndex == 0) {
                Audio_play(FONTID_A_RETRACT, 200, 0);
            } else if(fontIndex == 1){
                Audio_play(FONTID_B_RETRACT, 200, 0);
            }
#else
#endif
            Button_setBlink(primaryR*100/255, primaryG*100/255, primaryB*100/255, 100, 100, 100);
        }
    } else if(state == 19) {
        //  blade retract
        if(stateRuntime > 300) {
#ifndef NYANCAT
            Audio_stop(FONTID_A_HUM);
            Audio_stop(FONTID_A_SWING1H);
            Audio_stop(FONTID_A_SWING1L);
            Audio_stop(FONTID_A_SWING2H);
            Audio_stop(FONTID_A_SWING2L);
            Audio_stop(FONTID_B_HUM);
            Audio_stop(FONTID_B_SWING1H);
            Audio_stop(FONTID_B_SWING1L);
            Audio_stop(FONTID_B_SWING2H);
            Audio_stop(FONTID_B_SWING2L);
#else
            Audio_stop(FONTID_NYAN_HUM);
            Audio_stop(FONTID_NYAN_WAVE);
#endif
        }
        if(stateRuntime > 800) {
            state = 1;
            lastStateChange = time;
            Button_setPulse(50,50,50, 0, 20, 1000, 1000, 800, 700);
        }
    }
}

int main(void) {
    PLL_Init(Bus80MHz);

    setupInterrupts();

    setupPins();

    Power_initialize();
    IMU_initialize();
    Neopixel_initialize();
    Audio_initialize();
    Wifi_initialize();
    EEPROM_initialize();
    Button_initialize();

    while(millis() < 300) {}      //  we need a delay so that the ESP-01 can power up!
    ESP_Init();
    fontIndex = 1;
    connectedToPhone = false;

    Button_setSolid(100,0,0,100);
//    Neopixel_setRainbow(false);
//    Neopixel_setExtend();
//    uint32_t pr = millis();
//    uint8_t done = 0;
    while(1) {
        handlerLoop();
//        Neopixel_handler();
//        if (millis() - pr > 4000 && !done) {
//            Neopixel_setRetract();
//            done = 1;
//        }
        controlLoop();
    }
}
