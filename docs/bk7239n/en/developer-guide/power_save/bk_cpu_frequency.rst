CPU Clock Source Overview
=============================

:link_to_translation:`zh_CN:[中文]`

CPU clock sources:

- 40M crystal
- 480M DPLL

CPU theoretically supports a 4-bit divider:

F = 480M / (1+N), N from 0 to 15

Note:
Validated stable range is up to 240M and down to 60M.

CPU Frequency Switching
-----------------------------------------------
- CPU frequency can be switched dynamically based on workload to optimize power consumption. CPU main frequency switching is relatively complex and is implemented internally by the SDK.

