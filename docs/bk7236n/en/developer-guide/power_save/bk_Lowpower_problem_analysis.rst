Low Power Problem Analysis
=======================================================

:link_to_translation:`zh_CN:[中文]`

Low Voltage Problem Analysis Guide
--------------------------------------------------------------

1. Low voltage entry fails
##############################################################

- Check whether RTOS can enter IDLE task (for example, avoid always-running loops in business threads).
- Check whether required sleep votes are all set (for example, APP vote and module votes).
- Use debug command ``pm_debug 1`` to inspect low-voltage voting status and compare current votes with required votes.

2. System resets after low voltage wakeup
##############################################################

- A common cause is interrupt storm after wakeup, which may lead to watchdog reset.
- Check interrupt clear flow and confirm no interrupt source keeps retriggering continuously.

Deep Sleep Problem Analysis Guide
--------------------------------------------------------------

1. Deep sleep entry fails
##############################################################

- Check whether RTOS can enter IDLE task.
- Check whether required deep-sleep votes are all satisfied.
- Use debug command ``pm_debug 2`` / ``pm_debug 1`` to inspect deep-sleep vote status.

Debug Information Collection
--------------------------------------------------------------

- Use debug command ``pm_debug 8`` to collect low-power related status.
- Focus on module vote bitmaps, power domain states, and key module runtime states.

Q&A
--------------------------------------------------------------

- High current in deep sleep: verify VDDDIG and peripheral leakage.
- High current in low voltage: verify module close state, test environment, and Wi-Fi keep-alive behavior.

