## Steps to build the MCL firmware on Mac, tested with Sonoma 14.5
1) Download and install the Arduino Legacy IDE https://www.arduino.cc/en/software (1.8.19 tested, don't use 2.x)
2) Download and install homebrew https://brew.sh/
3) Install Powershell with homebrew https://formulae.brew.sh/cask/powershell
4) Install dotnet with homebrew https://formulae.brew.sh/cask/dotnet
5) Add avr-gcc Arduino toolchain path to PATH environment variable:
    ```bash
    export PATH=$PATH:/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin
    ```
6) Clone the MCL repository:
    ```bash
   git clone https://github.com/jmamma/MCL.git
    ```
7) Generate the compressed assets for the graphics and menu structures:
    ```bash
    cd MCL/resource
    pwsh ./gen-resource-mac.ps1
    ```
8) Compile the MCL firmware:
    ```bash
    cd ../avr/cores/megacommand
    make
    ```
9) The compiled firmware `main.hex` will be in the `avr/cores/megacommand` folder. The file main.hex should be something like 689174 Bytes large. Flash e.g. with avrdude (replace /dev/cu.usbmodem141201 with the correct port):
   ```bash
   avrdude -C /Applications/Arduino.app/Contents/Java/hardware/tools/avr/etc/avrdude.conf -v -patmega2560 -cwiring -P/dev/cu.usbmodem141201 -b115200 -D -Uflash:w:./main.hex:i
   ```