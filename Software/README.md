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

**2026-01-28**

Changed to 1MHz clock.  Now the ISR takes a lot of time,
so the loop-based delay (`delay_ms()`) are off calibration.

Working on power consumption.  Sleep mode timeout working.
Power down current ~ 18uA.

The light sensor dominates.  Current varies from 18-25uA.
Should have powered it with a spare I/O (doh!).
This is about 1 year though for AA batts so no big deal, I guess.

Tried powering the pull-up using PB2.  Still 18uA, but no
additional current from the LDR.  Not worth the mod.
And where is the extra power going?

Write all 1's to the PRR, no change.

AHA!  Turning of the comparator by writing ACD in ACR does it!
Now ~ 0.5uA.

Without the LDR pull-up mod, bright ambient light draws 5uA.
This seems like the best we can do without modding the PCB.

=== Refactor the code ===

* Timer is 10kHz (100us)
   * 


**2026-01-27**

PWM dimmer based on timer 0 interrupt at 10kHz

Light sensor wired to AIN1 instead of ADCn pin!
<br>Maybe we can make it work with analog comparator
using internal 1.1V reference by changing pull-up resistor.

