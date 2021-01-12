
#ifndef BUTTONLED_H_
#define BUTTONLED_H_

extern void buttonLEDHandler();

extern void ButtonLED_setSolid(int r, int g, int b, int alpha);
extern void ButtonLED_setBlink(int r, int g, int b, int alpha, int onPeriod, int offPeriod);
extern void ButtonLED_setPulse(int r, int g, int b, int lowAlpha, int highAlpha, int rampUpPeriod, int topPeriod, int rampDownPeriod, int bottomPeriod);

#endif /* BUTTONLED_H_ */
