
#ifndef BUTTON_H_
#define BUTTON_H_

extern void Button_Handler();

extern void Button_initialize();
extern void Button_setSolid(int r, int g, int b, int alpha);
extern void Button_setBlink(int r, int g, int b, int alpha, int onPeriod, int offPeriod);
extern void Button_setPulse(int r, int g, int b, int lowAlpha, int highAlpha, int rampUpPeriod, int topPeriod, int rampDownPeriod, int bottomPeriod);
extern int Button_isPressed();

#endif /* BUTTON_H_ */
