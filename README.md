# Lightsaber

EE445L Embedded Systems Lab with Dr. Valvano <br />
Final Project, 1st place in class design competition <br />
Spring 2021 <br />
Team Members: Allen Jiang, Sophia Jiang, Adeel Rehman, Sharon Chen <br />
<br />
<b>Features:</b> <br />
* Circuit Board
  * Compact 4-Layer design (Board dimensions: 1.94" x 0.90")
  * TM4C Microcontroller
  * 12-bit DAC and audio amplifier output
  * MPU-6500 IMU
  * ESP-01S Wifi Module
  * Power initiated by button and latched on by MCU - allows for zero vampire power
  * USB-C Battery Charging
  * Onboard battery protection (overvoltage, undervoltage, short)
* Firmware
  * Neopixels driven by Ping-Pong uDMA
  * ESP-01S AP-mode for TCP-IP communication with Android app
  * Android app for on-the-fly color, blade effect, and audio configuration with voltage and temperature telemetry
  * Smoothswing v2 algorithm implementation
* Hardware
  * Hilt
    * Designed in Fusion 360
    * 1" aluminum core hilt
    * Holes drilled and tapped for button access, charging port access, and mounting 3D printed parts
    * Grooves cut using a milling bit on a drill press
    * 3D printed cosmetic attachments are finished through sanding, filler spray coat, and final spray paint coat
  * Blade
    * Polycarbonate tubing (1" OD, 0.125" walls)
    * 2 parallel back-to-back LED strips of 144 pixels/m WS2812B neopixels - up to 15 amps of current
    * LEDs diffused using layers of foam and cellophane. Polycarbonate tubing is sanded on the outside.
    * 3D printed blade cap using translucent filament
  * Blade connector
    * Custom designed as two 1" circular 4-layer PCBs
    * No latching components - easy and quick connect/disconnect
    * Hilt-side connector uses 19 pogo pins to contact with 3 large ring pads on blade-side connector
  * Chassis
    * Holds electronic internals together for durability and convenience
    * 3D printed PLA
    * Houses RGB button, PCB, 21700 Li-ion battery, speaker, and hilt-side blade connector
    * Blade connector mounted using screws and hot-melt insert
    * RGB button connected with ribbon cable
<br />
  <b>Video Link:</b>
<br />

[![Alt text](https://img.youtube.com/vi/Yrs18WD5HyQ/0.jpg)](https://www.youtube.com/watch?v=Yrs18WD5HyQ)
