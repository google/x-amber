R44 needs to be populated with a 0R resistor

To get LED insert jumpers on JP1 and JP2

Flashing:

Flash bootloader (“Luchador Bootloader PE 3.0”) via JTAG - .hex file

Flash FW (“Luchador Serial R3.0”) via usb (“data” port, J5 (?)) - .bin file using Luchador Loader

LED info:


*   The green LED is the RGB power indicator.  Flashing amber is a MCU running indicator.  4 amber LEDs are on the chip select lines of the ADCs to show that data is being read.  The row of LEDs shows the power supply status of the board.
*   When in bootloader mode, it does a fast red-green-blue flashing of the RGB LED.  The other LEDs would function the same.

Serial ports


*   J5 - data
*   J6 - mark
