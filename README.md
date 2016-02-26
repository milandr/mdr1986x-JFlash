# Milandr MCU 1986x flashing with J-Link

#### What's this project for?

- Debugging on [Milandr 32-bit Cortex-М microcontrollers]
(http://milandr.ru/en/index.php?mact=Products,cntnt01,default,0&cntnt01hierarchyid=5&cntnt01returnid=141)
using [GNU ARM Eclipse](http://gnuarmeclipse.github.io/)
with original [SEGGER J-Link drivers](https://www.segger.com/jlink-software.html).
- Internal EEPROM programming using [GNU toolchain](https://launchpad.net/gcc-arm-embedded).

#### What's the problem?

Unfortunately, SEGGER still knows nothing about EEPROM programming algorithm for Milandr MCU 1986x series.<br>
As a result you cannot use the native J-Flash utility. Moreover, you had to use [OpenOCD](http://openocd.org/)
instead of the native drivers for debugging with GNU ARM Eclipse. It's quite good, but at present
[OpenOCD debugging Eclipse plug-in](http://gnuarmeclipse.github.io/debug/openocd/)
does not support capturing of Serial Wire Output (SWO).<br>
Also you are able to use SEGGER [Real Time Transfer](https://www.segger.com/jlink-rtt.html) (RTT) only with the native drivers.

#### How does it work?

- LOADER implements EEPROM programming algorithm.
- GDB script on Python redefines the GDB load command.

#### Any limits?

Yes, at present only MDR1986BE9x (MDR32F9Qx) series is supported.
