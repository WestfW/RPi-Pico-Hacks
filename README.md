# RPi-Pico-Hacks
This is for hacks for or using the Rasberry Pi Cortex-m0+ microcontroller and SDK ("rp2040" and "Pico" board)

## Pico_labels
EAGLE "PCB Board" that is just a pin-label sticker designed to be stuck on top of a Pico Board to let you see which pins are which, during prototyping.

## Systick_delay
Delay for a specified number of CPU cycles, using the systick time.  Modified for the non-CMSIS RP2040 systick definitions.  Generally consumes about 20 cycles of overhead beyond the given cycle count, which means it's pretty accurate for timing things on the order of microseconds.
