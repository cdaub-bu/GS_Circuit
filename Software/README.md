# Software

## Specification

* On/Off switch (this actually interrupts the power)
* Photo sensor turns on light in darkness
* On power-up, turn on light for 10-30s
* Intensity button:
   * Turns on light when pressed
   * Subsequent presses change intensity (3-5 options)
   * Turns off (fade out?) after 10-30s
* ~~Only on for ~1 hour after dark (configurable?)~~
  <br>Dropped in rev3 (2026-10-26): on whenever it's dark

## Development log

**2026-10-26**

rev3 programmed with an Uno (CH340 clone) running ArduinoISP on COM6.
`avr-gcc`, `avrdude` 8.3 and `make` installed with winget.
Fuses read 0x62 / 0xDF (1MHz, factory default).

LEDs are active low (VCC → LED → pin).  Firmware had them inverted,
so "off" was full on and the 1% level was 99%.  Fixed in rev3 with
`LEDS_ON()` / `LEDS_OFF()`.

Clear `MCUSR` at start-up.  After a watchdog reset WDRF forces WDE on,
so the WDT would stay in reset mode instead of interrupt mode.

Sensor measured at 5V (ADC counts, VCC ref):
* Room light: 712-848
* Covered: >= 1020

So `DARK_LEVEL` 1000 / `LIGHT_LEVEL` 940 work.

`DEBUG_SENSOR` in rev3 `main.c` blinks each reading (1 = light,
2 = dark) and logs readings to EEPROM.  Gotcha: avrdude reading
EEPROM through ArduinoISP shows byte 2*i at index i (only even
addresses, aliased at 256), so values are logged as 16-byte blocks.
Flashing erases the EEPROM (EESAVE off), including the dimmer level.

Removed the 45 minute night time-out (and the WAIT_LIGHT state).
Lights are now on whenever it's dark, off when light.  2 readings
in a row to switch either way (was 4) for faster response.  The sensor is checked every ~8s while on too, so the
LEDs must be shielded from it.

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

