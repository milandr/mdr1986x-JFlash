## LOADER – RAMCode for Milandr MCU 1986x

#### Contents
`source/` – LOADER source code
`LOADER_F9Qx.bin` – LOADER for MDR32F9Qx
`LOADER_F1.bin` – LOADER for MDR32F1 and MDR32F3
`.project` – project to build LOADER in GNU ARM Eclipse
`mapper.py` – script to configure `JFlash.py` according to LOADER layout

```
Usage: mapper.py (F1|F3|F9)
F1 - MDR32F1, 1986BE1
F3 - MDR32F3, 1986BE3
F9 - MDR32F9Qx, 1986BE9x
```
