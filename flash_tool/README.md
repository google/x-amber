# NOTICE
* The materials covered by the associated license relate to a general purpose product and its underlying designs, schematics, and hardware and software code (collectively “Contributions”), none of which have been evaluated, cleared or approved for any medical or other purpose by and may not meet applicable safety or other legal or regulatory requirements of any governmental agency, including but not limited to the United States Food and Drug Administration (“FDA”), or equivalent regulatory bodies outside of the US.
* Any use of the term “EEG” or “electroencephalogram” contained in the Contribution is for informational purposes only and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* Any article, material or other publication by any person or entity referencing the use or operation of the Contribution for a medical or other any other purpose is for informational purposes only; does not directly or indirectly represent or infer any intended use in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals; and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* While the Contributions may have potential medical applications, many countries, including the US, require regulatory approvals prior to using any device in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals.
* It is the user’s responsibility to secure all necessary regulatory approvals and meeting all applicable government safety and environmental standards associated with use of the Contributions.

# Amber Firmware Loader IFU

last updated: 1/16/2019

The flashing station will be equipped with a Windows PC with the flash tool preinstalled.

Before you start:


*   Build a firmware binary. Source code is under `/firmware/source/Luchador Serial Firmware KL27`.
*   Copy the firmware binary to flash to the same directory as `Amber\_Production\_Loader.py`
    *   Source code is under `Amber Production Loader/`.
*   Ensure python 2.7 is installed
*   Record the firmware version used
*   Additional equipment: 1x USB A-B cable

Instructions:

1. Open Command Prompt
2. cd to the directory containing `Amber\_Production\_Loader.py`.
3. Run the following command: `python Amber\_Production\_Loader.py <firmware file>`.
4. Connect Luchador board to PC (J5 on the board) with USB A-B cable.
5. Wait for firmware to flash (about 100 seconds)
6. Note the status. “Programming successful” if success
7. If flashing is successful, the tool will automatically run diagnostics (about 20 seconds)
8. Note the test results. Log from successful test run:

```
Checking onboard power supplies:

***VSYS***
VAL             MAX     MIN     RESULT
4.81277 5.5     4.5     PASS

***+3.3V***
VAL             MAX     MIN     RESULT
3.304799        3.4     3.2     PASS

***+2.5V***
VAL             MAX     MIN     RESULT
2.518807        2.6     2.4     PASS

***-2.5V***
VAL             MAX     MIN     RESULT
-2.517468       -2.4    -2.6    PASS
Power supplies PASSED
Checking ADS1299:
Turning channels off
Sending test command
Gathering 5 seconds of data
Acquisition complete, analyzing...
Checking for sequential data...
PASS
Checking Amplitudes...
2140<HIGH<2624
-4882<LOW<-4381
PASSED
Checking synchronization...
PASSED
Checking frequency:
Average frequency=5.0 Hz
PASSED
Stopping serial thread
**************  ALL TESTS PASSED
```


1. When the tool prints “Remove programmed board and insert new board”, disconnect the board and connect the next board.
