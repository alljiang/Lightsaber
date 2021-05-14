/*
 * EEPROM.h
 *
 *  Created on: May 4, 2021
 *      Author: Allen
 */

#ifndef EEPROM_H_
#define EEPROM_H_

extern void EEPROM_initialize();

void EEPROM_setPrimaryColors(uint8_t R, uint8_t G, uint8_t B);
void EEPROM_setSecondaryColors(uint8_t R, uint8_t G, uint8_t B);
void EEPROM_setSoundFontIndex(uint8_t index);
void EEPROM_setBladeEffectIndex(uint8_t index);

uint32_t EEPROM_readPrimaryR();
uint32_t EEPROM_readPrimaryG();
uint32_t EEPROM_readPrimaryB();
uint32_t EEPROM_readSecondaryR();
uint32_t EEPROM_readSecondaryG();
uint32_t EEPROM_readSecondaryB();
uint32_t EEPROM_readSoundFontIndex();
uint32_t EEPROM_readBladeEffectIndex();

#endif /* EEPROM_H_ */
