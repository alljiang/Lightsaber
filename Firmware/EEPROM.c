
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/eeprom.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "pinConfig.h"
#include "OTFConfig.h"

#define EEPROM_MEM_BASE                 0x400

#define EEPROM_MEM_PRIMARY_R            0
#define EEPROM_MEM_PRIMARY_G            1
#define EEPROM_MEM_PRIMARY_B            2
#define EEPROM_MEM_SECONDARY_R          3
#define EEPROM_MEM_SECONDARY_G          4
#define EEPROM_MEM_SECONDARY_B          5
#define EEPROM_MEM_FONT_INDEX           6
#define EEPROM_MEM_BLADE_EFFECT_INDEX   7


uint32_t EEPROM_read(uint32_t address) {
    uint32_t buffer;
    EEPROMRead(&buffer, address, 4);
    return buffer;
}

void EEPROM_write(uint32_t address, uint32_t data) {
    uint32_t copy = data;
    EEPROMProgram(&copy, address, 4);
}

void EEPROM_setPrimaryColors(uint8_t R, uint8_t G, uint8_t B) {
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_R, R);
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_G, G);
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_B, B);
}

void EEPROM_setSecondaryColors(uint8_t R, uint8_t G, uint8_t B) {
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_R, R);
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_G, G);
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_B, B);
}

void EEPROM_setSoundFontIndex(uint8_t index) {
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_FONT_INDEX, index);
}

void EEPROM_setBladeEffectIndex(uint8_t index) {
    EEPROM_write(EEPROM_MEM_BASE + 4*EEPROM_MEM_BLADE_EFFECT_INDEX, index);
}

uint8_t EEPROM_readPrimaryR() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_R); }
uint8_t EEPROM_readPrimaryG() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_G); }
uint32_t EEPROM_readPrimaryB() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_PRIMARY_B); }

uint8_t EEPROM_readSecondaryR() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_R); }
uint8_t EEPROM_readSecondaryG() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_G); }
uint8_t EEPROM_readSecondaryB() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_SECONDARY_B); }

uint8_t EEPROM_readSoundFontIndex() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_FONT_INDEX); }

uint8_t EEPROM_readBladeEffectIndex() { return (uint8_t) EEPROM_read(EEPROM_MEM_BASE + 4*EEPROM_MEM_BLADE_EFFECT_INDEX); }


void EEPROM_initialize() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)) {}

    int ui32EEPROMInit = EEPROMInit();
    if(ui32EEPROMInit != EEPROM_INIT_OK) while(1) {}

    primaryR = EEPROM_readPrimaryR();
    primaryG = EEPROM_readPrimaryG();
    primaryB = EEPROM_readPrimaryB();
    secondaryR = EEPROM_readSecondaryR();
    secondaryG = EEPROM_readSecondaryG();
    secondaryB = EEPROM_readSecondaryB();
    fontIndex = EEPROM_readSoundFontIndex();
    bladeEffectIndex = EEPROM_readBladeEffectIndex();
}
