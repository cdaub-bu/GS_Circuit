# Rev3 operating firmware for GSA night light

Adds the light sensor back.  Needs the rev2 board modified as below
(no MOSFET needed).  Will not work on unmodified rev2 boards.

Pins:

| Pin | GPIO | Use                                        |
|----:|------|--------------------------------------------|
|   5 | PB0  | Button to GND (internal pull-up)           |
|   6 | PB1  | LED2 + 91 ohm (moved from PB3)             |
|   2 | PB3  | TEPT4400 collector + 10K to VCC (ADC3)     |
|   3 | PB4  | LED1 + 91 ohm (unchanged)                  |

Board mods:
* Cut LED2 from PB3, jumper it to PB1 (pin 6)
* Rewire the TEPT4400 per the top-level README: emitter to GND,
  collector to PB3 and through 10K to VCC

The LEDs are wired from VCC to the pin, so they are active low
(pin low = LED on).

LED2 on MISO is fine for ISP since the ATtiny drives MISO itself.
Don't put LEDs on SCK or MOSI (the programmer would have to drive them).

## Programming with an Arduino Uno

One-time setup of the Uno:
1. In the Arduino IDE open File > Examples > 11.ArduinoISP > ArduinoISP
   and upload it to the Uno (normal upload, nothing connected).
2. Put a 10uF capacitor between Uno RESET and GND (- side to GND)
   so the Uno doesn't reset itself when avrdude connects.
   Remove it if you need to upload a new sketch to the Uno.

Wiring (Uno → ATtiny85 or the 6-pin ISP header J1):

| Uno | ATtiny85 pin | Signal |
|-----|--------------|--------|
| D13 | 7 (PB2)      | SCK    |
| D12 | 6 (PB1)      | MISO   |
| D11 | 5 (PB0)      | MOSI   |
| D10 | 1 (PB5)      | RESET  |
| 5V  | 8            | VCC    |
| GND | 4            | GND    |

Remove the batteries (or turn off the power switch) while the
Uno is powering the board.

Then, with the Uno's serial port (Device Manager / Arduino IDE
shows it):

```
make fuse     # once per new chip: 1MHz clock
make flash
```

`PORT` defaults to COM6 (the Uno used on 2026-10-26); for another
port use e.g. `make flash PORT=COM5`.

Needs `avr-gcc`, `avrdude` and `make` on the path.  The Arduino IDE
installs `avr-gcc` and `avrdude` under
`%LOCALAPPDATA%\Arduino15\packages\arduino\tools\`.

Operation:

Power-up:
* Read dimmer level from EEPROM or default to level 1
* Flash, then light for 2s
* If already dark, stay on

Night cycle:
* Lights off, nap in power-down, check sensor every ~8s (watchdog)
* Dark for 2 checks in a row → lights on
* Lights stay on while dark, sensor still checked every ~8s
* Light for 2 checks in a row → lights off

No time-out: the lights are on whenever it's dark.  The LEDs must be
shielded from the sensor, or they will switch themselves off.

Button press (wakes from a nap too):
* Increase brightness by one level
* Write new level to EEPROM
* Light for 2s, then back to what it was doing

`DARK_LEVEL` (1000) and `LIGHT_LEVEL` (940) in `main.c` were checked
on a board at 5V: room light reads 712-848, covered reads >= 1020.

## Debugging

Uncomment `#define DEBUG_SENSOR` in `main.c` to:
* Blink on each sensor reading (1 = light, 2 = dark)
* Log readings/4 to EEPROM in 16-byte blocks: 16 = power-up
  reading, 32 = latest reading, 48 = boot count, 64 = MCUSR

Read them with `avrdude -c stk500v1 -P COM6 -b 19200 -p t85 -U eeprom:r:-:h`.
Through ArduinoISP the dump shows byte 2*i at index i, so each block
appears as 8 bytes at half its address.  Reading also resets the chip.

PB3 can be jumpered to GND (reads light) or VCC (reads dark) to test.
