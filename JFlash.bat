@echo off
set SCRIPT_DIR=%~dp0

rem start JLinkRTTClient.exe
start JLinkGDBServerCL.exe -if swd -device "Cortex-M1" -endian little -speed 2000 -port 2331 -vd -localhostonly 1 -singlerun -strict -notimeout
arm-none-eabi-gdb-py --batch -x "%SCRIPT_DIR%JFlash.py" -ex "py program_from_shell('%1')"
