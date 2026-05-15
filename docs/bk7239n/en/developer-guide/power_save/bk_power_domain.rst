Power Domain Overview
==================================================

:link_to_translation:`zh_CN:[中文]`

Power Domain Overview
-----------------------------------------------------
Power domain basics
+++++++++++++++++++++++++++++++++++++++++++++++++++

The entire development board is divided into three types of power domains:
 -  Board-level power domain
 -  Analog power domain
 -  Digital power domain

+++++++++++++++++++++++++++++++++++++++++++++++++++

Analog Power Domain
+++++++++++++++++++++++++++++++++++++++++++++++++++
The analog power domain mainly covers analog modules such as crystal oscillators, PLL, analog front-end, bias circuits, and reference circuits.
These modules are closely related to clock accuracy, RF performance, and analog signal quality, and have direct impact on power and performance in different scenarios.

Modules under the analog power domain can also be managed by scenario:
 - Keep key analog modules powered on during active service to ensure stable function and performance;
 - Before entering low power mode, disable unnecessary analog submodules to reduce static and dynamic power;
 - After exiting low power mode, restore required analog modules in service sequence to avoid functional issues or performance jitter.

Pin Power Domain to Module Mapping
+++++++++++++++++++++++++++++++++++++++++++++++++++
From chip pins, the related power domains and module mapping are:
 - VCCPA: PA (power amplifier) module power;
 - VCCPAD: PAD/IO module power;
 - VCCIF: IF (intermediate frequency interface) module power;
 - VCCA: analog common module power;
 - VCCRXRF: RF receive-chain module power;
 - VCCPLL: PLL module power.

Voltage grouping is:
 - VCCPA and VCCPAD are consistent with VBAT;
 - VCCIF, VCCA, VCCRXRF, and VCCPLL are consistent with VDDA.

Digital power is VDDDIG - Digital Core power domain
+++++++++++++++++++++++++++++++++++++++++++++++++++
Each service module under the Digital Core power domain has its own independent sub-domain. Each service can power on or power off modules according to scenario, providing flexible power control and helping optimize power consumption for different product forms.

