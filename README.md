# Pulser_LED_Sel

Command line interface to select the corresponding LED of the SiPM PCB to pulse.<br>
It transmits the selection to the FPGA which then opens the corresponding MOSFET.

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
We need preferably VS-Code with *Desktop development with C++*.
Install [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/overview?utm_source=chatgpt.com) and then install boost, which is needed for the serial communication via:
```
vcpkg install boost-asio:x64-windows
```
Inside of a *Developer PowerShell for Visual Studio* compile:
```
cl /std:c++17 /EHsc /O2 sel_SiPM_led.cpp /Fe:sel_SiPM_led.exe
```

## How to use






