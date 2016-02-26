@echo off
start /B JLinkGDBServerCL.exe -if swd -device "Cortex-M3" -endian little -speed 2000 -port 2331 -vd -localhostonly 1 -singlerun -strict -notimeout
arm-none-eabi-gdb-py --batch -x JFlash.py -ex "py program_from_shell('%1')"
timeout 1 > NUL
