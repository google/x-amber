# Amber Luchador Flash Tool Requirements

## Overview

Amber team needs provide a tool to the contract manufacturer (CM) to flash software onto Luchador 3.1 boards.

The flow of an operator using the tool is as follows:



*   Before flashing any boards, set up flash tool (FT) with the firmware (FW) file to flash
*   FT detects when a board is plugged in (no need for the user to select the com port)
*   FT flashes and reboots the board - manual power cycle is ok here if there is no hard reset line
*   FT detects the board booted into the correct FW and shows status
*   FT writes serial number (date and time tool is set up and number, e.g. 201812061424-0000 - last 4 numbers increment by 1 per board - **TBD**
    *   do not attempt to write a new serial number if one already exists
*   FT runs onboard diagnostics and test mode and shows status
    *   test 1: verify power supplies 5V, 3.3V, 2.5V using on-chip ADC
    *   test 2: ADC self test - generate internal signals on ads1299 and verify output signals using data port output
*   Operator checks the status (if unsuccessful set it aside?) and plugs in the next board

(post assembly) test: test all 32 channels in jig (some % of boards only) 


## Must Have Requirements



1. The FT may either be in setup mode or operating mode
2. In setup mode, the operator can select a firmware file that will be used while it is in operating mode
3. After setup, the FT transitions to operating mode and stays there until the operator chooses
4. The FT auto-detects connected board, either one that’s ready to be flashed or one that’s already been flashed and is ready for diagnostics
5. The FT displays status (flashing, flash success/fail, running diagnostics, diagnostics pass/fail) clearly to the operator
6. The FT runs onboard diagnostics after successful flashing
7. Status screen: shows pass/fail, FW version on board


## Nice to have requirements



1. Log board flashes (timestamps, serial number, firmware version, user)

2 tests - 1 for 32 channel inputs, 1 for 3 channel inputs on a head. 

10 hz 

Cycle through each, 500ms each channel

Signal analysis either from the visualizer “test input mode”
