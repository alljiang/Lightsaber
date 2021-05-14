
#ifndef POWER_H_
#define POWER_H_

#define NO_CHARGE   0
#define CHARGING    1
#define FULL_CHARGE 2

extern void Power_initialize();
extern void Power_latchOn();
extern void Power_latchOff();
extern int Power_getBatteryVoltage();
extern int Power_isCharging();

#endif /* POWER_H_ */
