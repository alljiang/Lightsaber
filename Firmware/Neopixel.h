
#ifndef NEOPIXEL_H_
#define NEOPIXEL_H_

#define NEOPIXEL_COUNT 130

extern void Neopixel_initialize();
extern void Neopixel_setSolid(uint8_t r, uint8_t g, uint8_t b);
extern void Neopixel_sendData(uint32_t values[],int number);

#endif /* NEOPIXEL_H_ */
