
#ifndef POWER_H_
#define POWER_H_

extern void Power_initialize();
extern void Power_latchOn();
extern void Power_latchOff();
extern int Power_getBatteryVoltage();

#endif /* POWER_H_ */
