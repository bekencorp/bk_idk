.. _bk_securify_efuse:

EFUSE
=======================

:link_to_translation:`zh_CN:[中文]`

Overview
------------------------

EFUSE (Electrically Programming Efuse) is used to configure safety-related features in BK7236N. It can only be configured from 0 to 1, and cannot be configured from 1 to 0.

BK7236N EFUSE has a total of 4x8 and a total of 32 Bits. Before secure boot is enabled, it can be configured through :ref:`BKFIL<bk_tool_bkfil>`. For details, please refer to :ref:`EFUSE configuration <bk_config_otp_efuse>`.

+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| Bit     | Name                         | Description                                                                                                  |
+=========+==============================+==============================================================================================================+
| 0       | Secure boot                  | 0: Do not force-enable secure boot; 1: Force-enable secure boot.                                             |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 1       | Security boot debug mode     | 0: Enable secure boot debug information; 1: Disable secure boot debug information.                           |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 2       | Fast boot disable            | 0: Enable fast boot; 1: Disable fast boot.                                                                   |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 3       | Download disable             | 0: Download mode enabled; 1: Download mode disabled, UART download prohibited.                               |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 4       | Security boot clock select   | 0: Use XTAL clock for secure boot; 1: Enable PLL for secure boot.                                            |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 5       | Random delay enable          | 0: Disable random delay; 1: Enable random delay (FIH delay).                                                 |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 6       | Bl1 power on fastboot        | 0: Skip signature verification on power-on; 1: Do not skip signature verification on power-on.               |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 7       | Security boot critical error | 0: Enable critical error printing during secure boot; 1: Disable critical error printing during secure boot. |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 8       | reserved                     | Reserved.                                                                                                    |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 9       | Flash ctrl access disable    | 0: Allow flash control register access via HCI; 1: Disable flash control register access via HCI.            |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 10      | Deep LV fast boot disable    | 0: Deep LV fast boot enabled; 1: Deep LV fast boot disabled.                                                 |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 11      | Long bl1 security cnt enable | 0: BL1 security counter occupies 4 bytes; 1: BL1 security counter occupies 68 bytes.                         |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 12      | Secure Download              | 0: Secure download disabled; 1: Secure download enabled.                                                     |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 13/14/15| Disable BKFIL channel        | Bits 13/14/15 combined: 1/7 - BKFIL channel disabled, 0/2/3/4/5/6 - BKFIL channel enabled.                   |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 16      | reserved                     | Reserved.                                                                                                    |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 17      | Enable CPU 120M              | 0: CPU 80M; 1: CPU 120M.                                                                                     |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 18      | Enable flash 80M             | 0: Flash 40M; 1: Flash 80M.                                                                                  |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 19      | CBUS disable                 | 0: CBUS port enabled; 1: CBUS port disabled.                                                                 |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 20      | Anti injection attack        | 0: Anti-injection attack disabled; 1: Anti-injection attack enabled.                                         |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 21      | Spi to ahb disable           | 0: SPI debug enabled; 1: SPI debug disabled.                                                                 |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 22      | Bl1 auto reset enable[0]     | 0: BL1 auto reset disabled; 1: BL1 auto reset enabled.                                                       |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 23      | Bl1 auto reset enable[1]     | 0: BL1 auto reset disabled; 1: BL1 auto reset enabled.                                                       |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 24      | Memory check disable         | 0: Execute memcheck on cold boot; 1: Do not execute memcheck on cold boot.                                   |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 25      | HW JTAG disable              | 0: Hardware debug (JTAG) not disabled; 1: Hardware debug (JTAG) disabled.                                    |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 26      | Encryption clk gating enable | 0: Encryption engine clock gating disabled; 1: Encryption engine clock gating enabled.                       |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 27      | Flash crc mode               | 0: CRC mode; 1: No CRC mode.                                                                                 |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 28      | Flash aes mode               | 0: FLASH AES-XTS-128 mode; 1: Not supported yet.                                                             |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 29      | Flash aes enable             | 0: FLASH AES encryption disabled; 1: FLASH AES encryption enabled.                                           |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 30      | Spi download disable         | 0: SPI download enabled; 1: SPI download disabled.                                                           |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+
| 31      | Swd disable                  | 0: Enable CPU SWD; 1: Disable CPU SWD.                                                                       |
+---------+------------------------------+--------------------------------------------------------------------------------------------------------------+

.. _efuse_bit0:

BIT(0) - secure boot
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(0) is used to control whether secure boot is force-enabled:

  - 0 - Do not force-enable secure boot.
  - 1 - Force-enable secure boot.

When :ref:`BIT(0)<efuse_bit0>` is set to 0, it means the EFUSE does not force-enable secure boot, and the secure boot flow is controlled by the following conditions:

  - If the secure boot flag (BK.SB) is not set, BL2 does not perform signature verification and only performs IMAGE integrity verification.
  - If the secure boot flag (BK.SB) is set, BL2 will perform the full secure boot verification flow.

When :ref:`BIT(0)<efuse_bit0>` is set to 1, it means the EFUSE force-enables secure boot. In this case, BL1 will perform the full verification flow.

.. note::

   Whether the boot fails due to a missing :ref:`BL1 ROTPK HASH<otp_bl1_rotpk_hash>` in OTP depends on the current life cycle (LCS) of the chip:

    - When the LCS is in CM/DM (development stage), even if secure boot is enabled, if the public key HASH is not configured in OTP, BL1 will bypass verification (printing "Secure Boot is bypassed") and continue to boot.
    - When the LCS is in DD/DR (deployment/production stage), the public key HASH must be configured in OTP, otherwise the boot will fail; secure boot can no longer be disabled at this stage.

.. note::

   Once secure boot is enabled, it cannot be downloaded and configured through BKFIL! Therefore, BIT(3) should be enabled last after deploying all other OTP/EFUSE configurations.
   When using :ref:`config scripts <bk_config_otp_efuse_scripts>` for deployment, please make sure that all OTP/EFUSE items that need to be configured are included in the configuration scripts.

.. _efuse_bit1:

BIT(1) - security boot debug mode
++++++++++++++++++++++++++++++++++++++++++++++++

BL1 defines two levels of debugging information for users to locate problems:

  - :ref:`BIT(1)<efuse_bit1>` - Controls general debug information.
  - :ref:`BIT(7)<efuse_bit7>` - Control fatal errors.

In addition to errors, general debugging information also includes some process log printing. Serious errors usually refer to errors that will cause BL1 to fail to start. Currently, BL1 supports the following serious errors:

+------------+---------------------------------------+
| Error code | Meaning                               |
+============+=======================================+
| 0x1        | Error reading EFUSE 1                 |
+------------+---------------------------------------+
| 0x2        | Error reading EFUSE 2                 |
+------------+---------------------------------------+
| 0x3        | Error reading FLASH 1                 |
+------------+---------------------------------------+
| 0x11       | CPU exception NMI                     |
+------------+---------------------------------------+
| 0x12       | CPU exception MemManage               |
+------------+---------------------------------------+
| 0x13       | CPU exception HardFault               |
+------------+---------------------------------------+
| 0x14       | CPU exception BusFault                |
+------------+---------------------------------------+
| 0x15       | CPU exception UsageFault              |
+------------+---------------------------------------+
| 0x16       | CPU exception SecurityFault           |
+------------+---------------------------------------+
| 0x21       | OTP is empty                          |
+------------+---------------------------------------+
| 0x22       | public key is empty                   |
+------------+---------------------------------------+
| 0x23       | Jump BIN verification failed          |
+------------+---------------------------------------+
| 0x1xxx     | OTP read failure                      |
+------------+---------------------------------------+
| 0x8yyyyyyy | Signature verification failed         |
+------------+---------------------------------------+

Among them, xxx refers to the OFFSET of OTP, and yyyyyyy refers to the specific failure point of signature verification.

For serious errors, only the error code is displayed when printing, such as "E16" means that the CPU is abnormal SecurityFault.

.. note::

  When critical errors are enabled, the BL1 will not initialize the UART during safe boot, and will only initialize the UART for printing when an unrecoverable serious error occurs.
  Therefore, critical errors do not affect normal boot functionality and do not pose security issues.

.. important::

   Generally, during the development stage, especially before the secure boot has been successfully configured on any board, it is recommended to enable the normal log, so that more debugging information can be seen;
   Normal logging should be turned off after getting familiar with the secure boot configuration, or during mass production. For serious errors, it is recommended not to close it in the mass production version.

.. _efuse_bit2:

BIT(2) - fast boot disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(2) Enable Fast Boot startup when set to 0, and disable Fast Boot startup when set to 1.

Fast Boot is used to control the process of waking up the system from Deep Sleep. When Fast Boot is enabled, Deep Sleep will skip the safe boot after waking up and jump directly to the application;
When Fast Boot is turned off, it will do a complete safe boot similar to power-on restart.

.. important::

   When Fast Boot is enabled, the startup speed is faster, but it is not safe; when Fast Boot is disabled, the startup speed is slower, but it is safe and reliable.
   The application should decide whether to enable or disable Fast Boot based on actual needs.

.. _efuse_bit3:

BIT(3) - download disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(3) is used to disable download mode:

 - 0 - Download mode enabled, supports traditional download.
 - 1 - Download mode disabled, UART download prohibited.

.. _efuse_bit4:

BIT(4) - security boot clock select
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(4) is used to enable/disable safe boot high frequency mode.

  - 0 means CPU and FLASH use XTAL as the clock.
  - 1 means enable PLL, CPU and FLASH configured in high frequency mode.

In the high-frequency mode, the safe boot speed is faster, and it is generally recommended to enable the high-frequency mode for applications that require boot performance.

.. _efuse_bit5:

BIT(5) - random delay enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(5) is used to control the random delay of the shutdown judgment statement in BootROM:

  - 0 means random delay is off.
  - 1 means on. when random delayWhen enabled, BL1 will make a random delay before calling key functions, through this mechanism to slow down the impact of :ref:`fault injection attack <fault_injection_attack>`.

.. note::

  Enabling the random delay will increase the secure boot time. It is generally not recommended to enable this feature unless the application scenario requires particularly strong protection against injection attacks!

.. _efuse_bit6:

BIT(6) - bl1 power on fastboot
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(6) configures whether BL1 skips the signature verification process during power-on boot:

  - 0 - Allow skipping verification: only when it is a power-on restart and the bypass flag in FLASH meets the skip condition does BL1 skip image signature verification to speed up boot.
  - 1 - Do not skip verification: in any secure boot scenario, BL1 always performs the full verification process.

.. _efuse_bit7:

BIT(7) - security boot critical error
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(7) controls printing of critical errors during secure boot:

  - 0 - Enable critical error printing during secure boot. When an unrecoverable critical error occurs, BL1 initializes UART and prints the error code.
  - 1 - Disable critical error printing during secure boot.

BIT(7) works together with :ref:`BIT(1) <efuse_bit1>` to control critical errors and general debug information respectively. See the error code table in the BIT(1) section for critical error meanings.

.. note::

  When critical error printing is enabled, BL1 does not initialize UART during normal secure boot; UART is initialized only when an unrecoverable critical error occurs. Therefore, enabling critical errors does not affect normal boot or pose security risks.

.. important::

  It is recommended to keep critical error printing enabled (BIT(7) set to 0) in production builds to help diagnose boot failures.

.. _efuse_bit8:

BIT(8) - reserved
++++++++++++++++++++++++++++++++++++++++++++++++

Reserved bit, no configuration required.

.. _efuse_bit9:

BIT(9) - flash ctrl access disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(9) controls whether flash control register access via HCI is allowed:

  - 0 - Allow flash control register access via HCI.
  - 1 - Disable flash control register access via HCI.

.. _efuse_bit10:

BIT(10) - deep LV fast boot disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(10) controls the Deep LV fast boot function:

  - 0 - Deep LV fast boot enabled.
  - 1 - Deep LV fast boot disabled.

.. _efuse_bit11:

BIT(11) - Long bl1 security cnt enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(11) configures the space occupied by the BL1 security counter:

  - 0 - BL1 security counter occupies 4 bytes.
  - 1 - BL1 security counter occupies 68 bytes.

.. _efuse_bit12:

BIT(12) - Secure Download
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(12) controls the secure download function:

  - 0 - Secure download disabled.
  - 1 - Secure download enabled.

.. _efuse_bit13:
.. _efuse_bit14:
.. _efuse_bit15:

BIT(13)/BIT(14)/BIT(15) - Disable BKFIL channel
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(13), BIT(14) and BIT(15) are used in combination to control the BKFIL channel:

  - Combined value 1/7 - BKFIL channel disabled.
  - Combined value 0/2/3/4/5/6 - BKFIL channel enabled.

.. important::

   Once the BKFIL channel is disabled, EFUSE and OTP can no longer be configured through BKFIL. Disable it only after confirming the configuration is correct.

.. _efuse_bit16:

BIT(16) - reserved
++++++++++++++++++++++++++++++++++++++++++++++++

Reserved bit, no configuration required.

.. _efuse_bit17:

BIT(17) - Enable CPU 120M
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(17) configures the CPU operating frequency:

  - 0 - CPU 80M.
  - 1 - CPU 120M.

.. _efuse_bit18:

BIT(18) - Enable flash 80M
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(18) configures the FLASH operating frequency:

  - 0 - Flash 40M.
  - 1 - Flash 80M.

.. _efuse_bit19:

BIT(19) - CBUS disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(19) controls the CBUS port:

  - 0 - CBUS port enabled.
  - 1 - CBUS port disabled.

.. _efuse_bit20:

BIT(20) - Anti injection attack
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(20) controls the anti-injection attack function:

  - 0 - Anti-injection attack disabled.
  - 1 - Anti-injection attack enabled.

.. _efuse_bit21:

BIT(21) - spi to ahb disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(21) controls the SPI debug function:

  - 0 - SPI debug enabled.
  - 1 - SPI debug disabled.

It should be noted that BIT(21) and :ref:`BIT(30) spi flash download disable<efuse_bit30>` are independent of each other and need to be configured separately.

.. important::

   When Secure Boot is enabled, SPI debug must be disabled.

.. _efuse_bit22:
.. _efuse_bit23:

BIT(22)/BIT(23) - Bl1 auto reset enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(22) and BIT(23) control the BL1 auto reset function:

  - BIT(22) Bl1 auto reset enable[0] - control bit 0.
  - BIT(23) Bl1 auto reset enable[1] - control bit 1.
  - 0 - BL1 auto reset disabled.
  - 1 - BL1 auto reset enabled.

.. _efuse_bit24:

BIT(24) - Memory check disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(24) controls whether memcheck is executed on cold boot:

  - 0 - Execute memcheck on cold boot.
  - 1 - Do not execute memcheck on cold boot.

.. _efuse_bit25:

BIT(25) - HW JTAG disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(25) controls the hardware JTAG debug interface:

  - 0 - Hardware debug (JTAG) not disabled.
  - 1 - Hardware debug (JTAG) disabled.

Note that BIT(25) and :ref:`BIT(31) SWD debug <efuse_bit31>` are independent and control the hardware JTAG and CPU SWD debug ports respectively.

.. important::

   When Secure Boot is enabled, it is recommended to disable both JTAG and SWD debug interfaces.

.. _efuse_bit26:

BIT(26) - Encryption clk gating enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(26) controls the encryption engine clock gating function:

  - 0 - Encryption engine clock gating disabled.
  - 1 - Encryption engine clock gating enabled.

.. _efuse_bit27:

BIT(27) - flash crc mode
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(27) configures whether CRC verification is enabled when accessing FLASH via the instruction port:

  - 0 - CRC mode. The FLASH controller appends 2 CRC bytes to every 32 bytes of data, and virtual and physical addresses are mapped.
  - 1 - No CRC mode. Virtual and physical addresses are the same.

This bit must match the CRC setting used during firmware packaging. The packaging tool generates images according to ``FLASH_CRC_ENABLE`` in ``security.csv`` or ``auto_partitions.csv``.
In :ref:`OTP EFUSE configuration <bk_config_otp_efuse>`, the corresponding field is ``flash_no_crc_enable`` (BIT(3) of EFUSE byte 3).
See :ref:`FLASH AES encryption and CRC <bk_security_flash_aes_crc>` for details.

.. important::

   The CRC setting in EFUSE must match the image format burned into FLASH; otherwise the chip may fail to boot.

.. _efuse_bit28:

BIT(28) - flash aes mode
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(28) selects the FLASH AES encryption algorithm mode and takes effect after :ref:`BIT(29) flash aes enable <efuse_bit29>` is enabled:

  - 0 - FLASH AES-XTS-128 mode. AES KEY length is 32 bytes.
  - 1 - Not supported yet.

This bit must match the length of the :ref:`FLASH AES KEY <otp_flash_aes_key>` deployed in OTP and the packaging configuration.

.. _efuse_bit29:

BIT(29) - flash aes enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(29) is used to enable FLASH AES encryption:

  - 0 - FLASH AES encryption disabled.
  - 1 - FLASH AES encryption enabled. The :ref:`FLASH AES KEY <otp_flash_aes_key>` must be configured.

.. _efuse_bit30:

BIT(30) - spi download disable
++++++++++++++++++++++++++++++++++++++++++++++++

Disable SPI download function:

  - 0 - Enable internal SPI FLASH channel, support SPI download.
  - 1 - Internal SPI FLASH channel closed, does not support SPI download.

It should be noted that BIT(30) and :ref:`BIT(21) spi to ahb disable<efuse_bit21>` are independent of each other and need to be configured separately.

.. important::

   To avoid potential security risks, SPI downloads should be disabled in production builds. However, do not disable SPI downloads until Secure Boot has been successfully deployed,
   In this way, when the secure boot deployment fails, the version can still be downloaded to FLASH through SPI download. Otherwise, once the secure boot deployment fails, try again
   Also unable to download the version, the board becomes bricked.

.. _efuse_bit31:

BIT(31) - SWD debug
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(31) is used to control the switch of CPU debug port:

  - 0 - CPU debugging is enabled, BK7236N supports SWD debugging.
  - 1 - CPU debugging off. At this time, the CPU debugging function must be enabled through :ref:`secure debugging <security_secure_debug>`.

.. important::

  When secure boot is enabled, SWD debugging needs to be turned off.
