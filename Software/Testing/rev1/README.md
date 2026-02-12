# Rev1

First attempt at operating software

Some thoughts:

Device spends most of it's time in sleep mode.
* Wake up every few s with WDT and increment a counter

Each minute:
* Check dark level.  If newly dark, wake up, turn on LEDs and set a timer for ~1h
* If light and light has persisted for 5m, turn off and go back to sleep

Each button press:
* turn on LEDs and set timer for 10s
* subsequent presses change dimmer level
* after 10s timeout, go back to checking light level


```
Power up:
  load config from EEPROM or set default (intensity, timeout)
  flash_leds()
  check_light()
  if(light)
    goto SLEEP_START
  else(dark)
	goto START_DARK
	
START_DARK:
  reset slow_time
  turn on LEDs
  do while slow_time < long_timeout
	(optional:  go to idle mode with interrupt check for timeout)
  sleep_mode = NIGHT
  sleep_timer = 1h

SLEEP_START:
  configure watchdog wakeup every 8s
  configure pin change wakeup
  prepare to sleep
  goto sleep
  
WDT Wakeup:
  if(dark)
    disable WDT
	goto START_DARK
```

Functions:

```
check_light():
  power up ACMP
  wait for stable
  take a reading
  power down ACMP
  return result
```  
