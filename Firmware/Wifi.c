
#include <stdint.h>
#include <stdbool.h>
#include "Wifi.h"
#include "ESP.h"
#include "OTFConfig.h"

#define readBufferSize 100

char transmitBuffer[15];
uint8_t buildBuffer[15];
volatile char readBuffer[readBufferSize];
uint8_t readBufferIndex;
uint8_t buildBufferIndex;
volatile uint8_t readBufferLength;
volatile int bytesReading = -1;

void Wifi_initialize() {
    bytesReading = -1;
    readBufferIndex = 0;
    readBufferLength = 0;
}

void Wifi_readIntoBuffer() {
    volatile int length = 0;
    volatile char* read = ESP_GetString(&length);

    for(int i = 0; i < length; i++) {
        readBuffer[(readBufferIndex + i) % readBufferSize] = read[i];
    }
    readBufferLength += length;
}

int lastReading = 0;
void Wifi_handler() {
    Wifi_readIntoBuffer();

    if(bytesReading == 0) {
        if(buildBuffer[0] == 1) {
            primaryR = buildBuffer[1];
            primaryG = buildBuffer[2];
            primaryB = buildBuffer[3];
            secondaryR = buildBuffer[4];
            secondaryG = buildBuffer[5];
            secondaryB = buildBuffer[6];
        } else if(buildBuffer[0] == 2) {
            fontIndex = buildBuffer[1];
        } else if(buildBuffer[0] == 3) {
            bladeEffectIndex = buildBuffer[1];
        } else if(buildBuffer[0] == 4) {
            Wifi_sendSettings(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB, fontIndex, bladeEffectIndex);
        }
        bytesReading = -1;
    } else if(bytesReading > 0) {
        //  in middle of reading packet
        while(bytesReading > 0 && readBufferLength > 0) {
            buildBuffer[buildBufferIndex++] = readBuffer[readBufferIndex];
            readBufferIndex = (readBufferIndex + 1) % readBufferSize;
            bytesReading--; readBufferLength--;

            lastReading = millis();
        }

        if(millis() - lastReading > 500) bytesReading = -1;
    } else {
        //  awaiting header
        if(readBufferLength > 0) {
            //  read header
            buildBuffer[0] = readBuffer[readBufferIndex];
            buildBufferIndex = 1;
            readBufferIndex = (readBufferIndex + 1) % readBufferSize;

            if(buildBuffer[0] == 1) bytesReading = 6;
            else if(buildBuffer[0] == 2) bytesReading = 1;
            else if(buildBuffer[0] == 3) bytesReading = 1;
            else if(buildBuffer[0] == 4) bytesReading = 0;
            else readBufferLength = 0;  //  throw away

            lastReading = millis();
        }
    }

}

void Wifi_sendSettings(uint8_t primary_R, uint8_t primary_G, uint8_t primary_B,
                       uint8_t secondary_R, uint8_t secondary_G, uint8_t secondary_B,
                       uint8_t fontIndex, uint8_t bladeEffectIndex) {
    transmitBuffer[0] = 12;
    transmitBuffer[1] = primary_R;
    transmitBuffer[2] = primary_G;
    transmitBuffer[3] = primary_B;
    transmitBuffer[4] = secondary_R;
    transmitBuffer[5] = secondary_G;
    transmitBuffer[6] = secondary_B;
    transmitBuffer[7] = fontIndex;
    transmitBuffer[8] = bladeEffectIndex;
    ESP_SendStupidString(transmitBuffer, 9);
}

//  batteryVoltage input must be scaled by 1000
void Wifi_sendVoltage(int batteryVoltage) {
    uint8_t top8 = (batteryVoltage >> 8) & 0xFF;
    uint8_t bottom8 = (batteryVoltage >> 0) & 0xFF;

    transmitBuffer[0] = 15;
    transmitBuffer[1] = top8;
    transmitBuffer[2] = bottom8;
    ESP_SendStupidString(transmitBuffer, 3);
}

//  temperature input must be scaled by 100
void Wifi_sendTemperature(int temperature) {
    uint8_t top8 = (temperature >> 8) & 0xFF;
    uint8_t bottom8 = (temperature >> 0) & 0xFF;

    transmitBuffer[0] = 14;
    transmitBuffer[1] = top8;
    transmitBuffer[2] = bottom8;
    ESP_SendStupidString(transmitBuffer, 3);
}
