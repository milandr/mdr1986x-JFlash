# Milandr MCU 1986x flashing with J-Link

#### What's this project for?

- Debugging on [Milandr 32-bit Cortex-М microcontrollers]
(http://milandr.ru/en/index.php?mact=Products,cntnt01,default,0&cntnt01hierarchyid=5&cntnt01returnid=141)
using [GNU ARM Eclipse](http://gnuarmeclipse.github.io/)
with native [SEGGER J-Link drivers](https://www.segger.com/jlink-software.html).
- Internal EEPROM programming using [GNU toolchain](https://launchpad.net/gcc-arm-embedded).

#### What's the problem?

Unfortunately, SEGGER still knows nothing about EEPROM programming algorithm for Milandr MCU 1986x series.

As a result you cannot use the native J-Flash utility. Moreover, you had to use [OpenOCD](http://openocd.org/)
instead of native drivers for debugging with GNU ARM Eclipse.

OpenOCD is quite good, but at present, slightly less functional, for example, [OpenOCD debugging Eclipse plug-in]
(http://gnuarmeclipse.github.io/debug/openocd/)<br>
does not support capturing of Serial Wire Output (SWO).

Also you are able to use SEGGER [Real Time Transfer](https://www.segger.com/jlink-rtt.html) (RTT)
only with native drivers.

#### How does it work?

- RAM code (`LOADER.bin`) implements EEPROM programming algorithm.
- GDB script on Python (`JFlash.py`) redefines the GDB `load` command.

#### Any limits?

Yes, at present only MDR1986BE9x (MDR32F9Qx) series is supported.

It hasn't been tested on Linux yet...

#### How to program EEPROM using GNU toolchain

- Install [SEGGER J-Link Software](https://www.segger.com/jlink-software.html) (tested with `5.10o`).
- Install [GNU toolchain](https://launchpad.net/gcc-arm-embedded) (tested with `4.9-2015-q3`).
- Install Python 2.7 and set `PYTHON_PATH` and `PYTHON_LIB` environment variables.
- You may need to configure `PATH` environment variable.

To program EEPROM, run the command:
```
JFlash.bat <BIN_FILE>
```
The batch file starts J-Link GDB server at first, then runs GDB client with JFlash script
and the binary file as arguments. Something in this way:
```
start /B JLinkGDBServerCL -if swd -device "Cortex-M3" -endian little -speed 2000 -port 2331 -singlerun -strict
arm-none-eabi-gdb-py --batch -x JFlash.py -ex "py program_from_shell('yourapp.bin')"
```

#### How to debug under Eclipse

- Install [GNU ARM Eclipse](http://gnuarmeclipse.github.io/install/).
- Configure [J-Link debugging Eclipse plug-in](http://gnuarmeclipse.github.io/debug/jlink/).

In the debugger launch configuration `GDB SEGGER J-Link Debugging → Debugger → GDB Client Setup`, you should:
- Replace `gdb` with `gdb-py` in `Executable`.
- Add `-x JFlash.py` into `Other options` (use the filename with full path).

The `JFlash.py` script redefines GDB `load` command, so when Eclipse calls `load`, the script runs instead.
