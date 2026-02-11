# Rev2 operating firmware for GSA night light

This is a basic version with dim capability and fixed 45m time-out.

Operation is simple:

Power-up:
* Read dimmer level from EEPROM or default to level 1
* Start timer
* When timer expires, go to power-down mode (0.5uA).
  <br>(no wake except by reset/power cycle)

Button press:
* Increase brightness by one level
* Write new level to EEPROM
* Start 2s timer on LEDs



