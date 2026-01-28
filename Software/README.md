# Software

## Specification

* On/Off switch (this actually interrupts the power)
* Photo sensor turns on light in darkness
* On power-up, turn on light for 10-30s
* Intensity button:
   * Turns on light when pressed
   * Subsequent presses change intensity (3-5 options)
   * Turns off (fade out?) after 10-30s
* Only on for ~1 hour after dark (configurable?)

## Development log

**2026-01-27**

PWM dimmer based on timer 0 interrupt at 10kHz

Light sensor wired to AIN1 instead of ADCn pin!
<br>Maybe we can make it work with analog comparator
using internal 1.1V reference by changing pull-up resistor.



