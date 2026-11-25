#
# Makefile for rev3 night light
#
#   make                       build main.hex
#   make flash PORT=COM5       program via Arduino Uno as ISP
#   make fuse PORT=COM5        set fuses (1MHz clock) -- once per new chip
#

MCU = attiny85
# MCU = atmega328

# 8 MHz for on-board oscillator
# F_OSC = 8000000
F_OSC = 1000000

# 16MHz for Stock arduino
# F_OSC = 16000000

CFLAGS = -g -Os -mmcu=$(MCU) $(EXTRAOPT) -DF_CPU=$(F_OSC) -Wl,-u,vfprintf -lprintf_min -lm

LDFLAGS = -Wl,-Map=$(<:.o=.map),--cref -mmcu=$(MCU)

# disable CLKDIV8 on ATTiny85
# default Fuses for ATTiny85 = 0x62
# FUSEOPT = -U lfuse:w:0xe2:m
FUSEOPT = -U lfuse:w:0x62:m

# programmer configuration for avrdude programming utility

# for AVRISP programmer on USB
# DUDEPORT  = -c avrispv2 -P usb

# USBTiny programmer
# DUDEPORT  = -c usbtiny -P usb

# Arduino Uno running the ArduinoISP sketch (File > Examples > ArduinoISP)
# serial port of the Uno: COMn on Windows, /dev/ttyACM0 on Linux
PORT ?= COM6
DUDEPORT  = -c stk500v1 -P $(PORT) -b 19200

# for Arduino bootloader
# DUDEPORT  = -c arduino -P /dev/ttyACM0

# DUDEMCU = m328p
DUDEMCU = t85
# (-y -u removed, avrdude 7+ rejects them)
AVRDUDE  = avrdude $(DUDEPORT) -p $(DUDEMCU)

objects = main.o timer_10kHz.o uart.o acomp.o sleep.o wdt.o adc.o

headers = timer_10kHz.h uart.h acomp.h sleep.h wdt.h adc.h

all: $(objects)  main.hex list

clean:
	rm -f *.o *~ *.hex *.elf *.map *.lst

flash: main.hex
	$(AVRDUDE) -U flash:w:$^:i

list:
	avr-objdump -h -S main.elf > main.lst
	avr-size --common -x main.elf

$(objects): %.o: %.c $(headers)
	avr-gcc -c $(CFLAGS) $< -o $@

main.elf: $(objects)
	avr-gcc $(LDFLAGS) $(objects) -o $@

main.hex: main.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

fuse:
	$(AVRDUDE) $(FUSEOPT)

rfuse:
	$(AVRDUDE) -U lfuse:r:-:h
