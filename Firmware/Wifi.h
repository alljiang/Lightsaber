
#ifndef WIFI_H_
#define WIFI_H_


void Wifi_initialize();

void Wifi_handler();

void Wifi_sendSettings(uint8_t primary_R, uint8_t primaryG, uint8_t primaryB,
                       uint8_t secondary_R, uint8_t secondary_G, uint8_t secondary_B,
                       uint8_t fontIndex, uint8_t bladeEffectIndex);

//  batteryVoltage input must be scaled by 1000
void Wifi_sendVoltage(int batteryVoltage);

//  temperature input must be scaled by 100
void Wifi_sendTemperature(int temperature);


#endif /* WIFI_H_ */
