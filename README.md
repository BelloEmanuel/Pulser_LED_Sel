# Pulser_LED_Sel

Command line interface to select the corresponding LED of the SiPM PCB to pulse.<br>
It transmits the selection to the FPGA which then sends the corresponding signals to the Switch-IC as well as the command to start and stop the pulsing.

## How to compile
In general the file *sel_SiPM_led.cpp* should be inside the directory where the compilation should take place.
<br>
---
compile on MacOS:
```
clang++ -std=c++17 -O2 -Wall -Wextra \
    sel_SiPM_led.cpp \
    -o sel_SiPM_led \
    -I"$(brew --prefix boost)/include"
```
---
compile on Linux:
```
sudo apt update
sudo apt install build-essential libboost-dev
```

```
g++ -std=c++17 -O2 -Wall -Wextra \
    sel_SiPM_led.cpp \
    -o sel_SiPM_led
```
---
compile on Windows:<br>

Inside of a *Developer Command Prompt for Visual Studio* compile:
```
cl /std:c++17 /EHsc /W4 sel_SiPM_led.cpp /Fe:sel_SiPM_led.exe
```

## How to use

---
After compilation you are left with an executable:
- ./sel_SiPM_LED for Linux- and Mac-based operating systems
- sel_SiPM_LED.exe for Windows

For Linux and MacOS the executable directly tries to find the correct USB-Port which connects the FPGA with the Computer when sending a command. If the port-finder algorithm is finding multiple connected USB-Ports the user must select the correct one. For Windows the port **has** to be manually selected.

_Windows Powershell_

```
$env:SEL_SiPM_PORT = "COM{X}"
```

_Windows CMD_

```
set SEL_SiPM_PORT=COM{X}
```

_Linux Terminal_
```
export SEL_SiPM_PORT=/dev/ttyUSB{X}
```
or
```
export SEL_SiPM_PORT=/dev/ttyACM{X}
```


_MacOS Terminal_
```
export SEL_SiPM_PORT=/dev/cu.usbserial-{XXXX}
```
or
```
export SEL_SiPM_PORT=/dev/cu.usbmodem-{XXXX}
```


This can be checked after programming the FPGA and sending any command for testing. The FPGA will itself transmit an acknowledgment signal back to the computer that the command has been fully received.<br>

**Before starting with the pulsing:**
- Select the LED to pulse, if no selection is made the onboard LED (LED0) will be selected, if its installed (This has to be checked for the final design!).

```
./sel_SiPM_LED LED_{X}
```

- Only after selecting a LED the pulsing can start. The configured frequency is 1kHz.

```
./sel_SiPM_LED start_pulsing
```

- Stopping the pulsing keeps the LED selection unchanged.

```
./sel_SiPM_LED stop_pulsing
```

- Reseting the LED selection goes back to LED0 or the unconnected path, depending on the actual design of the PCB. This leaves the pulsing status unaffected.

```
./sel_SiPM_LED reset
```




