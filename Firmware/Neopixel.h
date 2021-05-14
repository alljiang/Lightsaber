
#ifndef NEOPIXEL_H_
#define NEOPIXEL_H_

#define NEOPIXEL_COUNT 165

#define GAMMA_CORRECTION 1

// Macros to convert between RGB values and integer
#define rgb_to_int(r,g,b) ((r << 16) | (g << 8) | b)
#define get_red(rgb) (rgb >> 16 & 0xFF)
#define get_green(rgb) (rgb >> 8 & 0xFF)
#define get_blue(rgb) (rgb & 0xFF)

// Blade movement modes
#define NO_MOTION               0
#define EXTEND_MODE             1
#define RETRACT_MODE            2

// Neopixel patterns
#define NONE                    0
#define EXTEND_PATTERN          1
#define RETRACT_PATTERN         2
#define SOLID_PATTERN           3
#define SPARK_PATTERN           4
#define FLAME_PATTERN           5
#define RAINBOW_PATTERN         6
#define RAINBOW_PULSE_PATTERN   7
#define RAINBOW_STROBE_PATTERN  8
#define UNSTABLE_PATTERN   9
#define ENERGETIC_PATTERN  10

// Colors used for rainbow
#define RED_COLOR rgb_to_int(255,0,0)
#define ORANGE_COLOR rgb_to_int(240,90,0)
#define YELLOW_COLOR rgb_to_int(230,170,0)
#define GREEN_COLOR rgb_to_int(0,255,0)
#define BLUE_COLOR rgb_to_int(0,0,255)
#define PURPLE_COLOR rgb_to_int(150,0,150)

extern void Neopixel_initialize(void);
extern void Neopixel_setPixel(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b);
extern void Neopixel_update(void);
extern void Neopixel_handler(void);

extern void Neopixel_setExtend(void);
extern void Neopixel_setRetract(void);
extern void Neopixel_setOff(void);
extern void Neopixel_setSolid(uint8_t r, uint8_t g, uint8_t b);
extern void Neopixel_setSpark(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);
extern void Neopixel_setFlame(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);
extern void Neopixel_setRainbow(uint8_t nyan_flag);
extern void Neopixel_setRainbowPulse(void);
extern void Neopixel_setRainbowStrobe(void);
extern void Neopixel_setUnstable(uint8_t r, uint8_t g, uint8_t b);
extern void Neopixel_setEnergetic(uint8_t r, uint8_t g, uint8_t b);

extern void Neopixel_doExtend(void);
extern void Neopixel_doRetract(void);
extern void Neopixel_doSolid(void);
extern void Neopixel_doSpark(void);
extern void Neopixel_doFlame(void);
extern void Neopixel_doRainbow(void);
extern void Neopixel_doRainbowPulse(void);
extern void Neopixel_doRainbowStrobe(void);
extern void Neopixel_doUnstable(void);
extern void Neopixel_doEnergetic(void);

#endif /* NEOPIXEL_H_ */
