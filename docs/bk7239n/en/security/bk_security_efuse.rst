.. _bk_securify_efuse:

EFUSE
=======================

:link_to_translation:`zh_CN:[中文]`

Overview
------------------------

EFUSE (Electrically Programming Efuse) is used to configure safety-related features in BK7239. It can only be configured from 0 to 1, and cannot be configured from 1 to 0.

BK7239 EFUSE has a total of 4x8 and a total of 32 Bits. Before secure boot is enabled, it can be configured through :ref:`BKFIL<bk_tool_bkfil>`. For details, please refer to :ref:`EFUSE configuration <bk_config_otp_efuse>`.

+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| Bit       | Name                        | description                                                                                              |
+===========+=============================+==========================================================================================================+
| 0         | Reserved                    | Reserved                                                                                                 |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 1         | security boot debug mode    | 0: Enable secure boot debug information. 1: Disable secure boot debug information.                       |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 2         | fast boot disable           | 0: enable fast boot; 1: disable fast boot.                                                               |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 3         | boot mode                   | 0: traditional download mode; 1: secure boot mode.                                                       |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 4         | security boot clock select  | 0: use XTAL clock for secure boot; 1: enable PLL for secure boot.                                        |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 5         | random delay enable         | 0: Disable random delay; 1: Enable random delay.                                                         |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 6         | bl1 power on fastboot       | 0: skip image verify when power on ;1: no skip image verify when power on.                               |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 7         | security boot critical error| 0: Enable crit error print during secure boot; 1: Disable cri error printduring secure boot.             |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 8         | bl2 info log disable        | 0: Enable print normal log in BL2 secureboot;1:  Disable print normal log in BL2 secureboot.             |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 9         | bl2 error log disable       | 0: Enable print error log in BL2 secureboot;1:  Disable print error log in BL2 secureboot.               |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 10        | bl2 secure download disable | 0: Enable BL2 secure download;1: Disable BL2 secure download.                                            |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 11        | Long bl1 security cnt enable| 0: BL1 security counter length is 4 Bytes;1: BL1 security counter length is 68 Bytes.                    |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 12        | Secure Download             | 0: Enable secure download;1: Disable secure download.                                                    |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 13        | Disable BKFIL channel       | bit 13/14/15 , 1/7 - disable BKFIL download,0/2/3/4/5 - enable BKFIL download.                           |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 14        | Disable BKFIL channel       | bit 13/14/15 , 1/7 - disable BKFIL download,0/2/3/4/5 - enable BKFIL download.                           |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 15        | Disable BKFIL channel       | bit 13/14/15 , 1/7 - disable BKFIL download,0/2/3/4/5 - enable BKFIL download.                           |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 16        | Disable download high freq  | 0: Disable high frequency download;1: Enable high frequency download.                                    |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 17        | Enable CPU 120M             | 0: CPU 80M 1: CPU 120M.                                                                                  |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 18        | Enable flash 80M            | 0: flash XTAL 40M 1: flash 80M.                                                                          |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 19        | Reserved                    | Reserved                                                                                                 |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 20        | attack_nmi_enable           | 0: No NMI is generated when an injection attack is detected; 1: NMI when an injection attack is detected.|
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 21        | spi_to_ahb_disable          | 0: SPI to AHB channel is not disabled; 1: SPI to AHB channel is disabled.                                |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 22        | auto_reset_enable[0]        | Temperature and voltage abnormal reset.                                                                  |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 23        | auto_reset_enable[1]        | Temperature and voltage abnormal reset.                                                                  |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 24        | Memchk_bps                  | 0: Enable memorycheck when startup;1: Disable memorycheck when startup.                                  |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 25        | HW JTAG disable             | 0: Enable JTAG by HW;1: Disable JTAG by HW.                                                              |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 26        | shanhai_clkgating_en        | 0: Disable clk_gating;1: Enable clk_gating.                                                              |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 27        | flash_crc_mode              | 0: crc mode;1: no_crc mode.                                                                              |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 28        | Reserved                    | Reserved                                                                                                 |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 29        | flash aes enable            | 0: Disable FLASH AES;1: Enable FLASH AES.                                                                |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 30        | spi_dld_disable             | 0: Enable SPI download; 1: Disable SPI download.                                                         |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
| 31        | swd_disable                 | 0: Enable CPU SWD; 1: Disable CPU SWD.                                                                   |
+-----------+-----------------------------+----------------------------------------------------------------------------------------------------------+
 
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
| 0x12       | CPU exception MemMange                |
+------------+---------------------------------------+
| 0x13       | CPU exception HardFault               |
+------------+---------------------------------------+
| 0x14       | CPU exception BusFault                |
+------------+---------------------------------------+
| 0x15       | CPU exception UserFault               |
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

BIT(3) - boot mode
++++++++++++++++++++++++++++++++++++++++++++++++

It is used to configure the startup mode, disable download and enable secureboot.

.. _efuse_bit4:

BIT(4) - secure boot clock select
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(4) is used to enable/disable safe boot high frequency mode.

  - 0 means CPU and FLASH use XTAL as the clock, usually 26Mhz.
  - 1 means enable PLL, CPU and FLASH configuration

in high frequency mode. In the high-frequency mode, the safe boot speed is faster, and it is generally recommended to enable the high-frequency mode for applications that require boot performance.

.. _efuse_bit5:

BIT(5) - random delay enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(5) is used to control the random delay of the shutdown judgment statement in BootROM:

  - 0 means random delay is off.
  - 1 means on. when random delayWhen enabled, BL1 will make a random delay before calling key functions, through this mechanism to slow down the impact of :ref:`fault injection attack <fault_injection_attack>`.

.. note::

  Enabling the random delay will increase the secure boot time, unless the application has a particularly high defense against injection attacks, it is generally not recommended to enable the random delay function!

.. _efuse_bit6:

BIT(6) - bl1 power on fastboot 
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(6) Whether BL1 skips the signature verification process:

  - 0 indicates that during every power-on startup, it directly enters the fast signature verification process.
  - 1 indicates that in any secure boot scenario, a complete signature verification process is required.

.. _efuse_bit7:

BIT(7) - security boot critical error
++++++++++++++++++++++++++++++++++++++++++++++++

See :ref:`BIT(1)<efuse_bit1>`. .

.. _efuse_bit20:

BIT(20) - attack NMI
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(20) is used to configure whether an NMI exception is generated after detecting a :ref:`fault injection attack <fault_injection_attack>`:

  - 0 - BL1 does not perform fault injection attack detection at the hardware level.
  - 1 - BL1 performs fault injection attack detection at the hardware level, and generates an NMI exception when an attack is detected.

.. note::

   For applications that are particularly concerned about fault injection attacks, it is recommended to enable this switch.

.. _efuse_bit21:

BIT(21) - spi to ahb disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(21) is used to disable SPI to AHB channel:

  - 0 - SPI to AHB channel enable. At this time, the BK7239 registers can be directly operated through the SPI interface.
  - 1 - SPI to AHB channel off. At this time, the BK7239 registers cannot be operated through the SPI interface.

It should be noted that BIT(21) and :ref:`BIT(30) spi flash download disable<efuse_bit30>` are independent of each other and need to be configured separately.

.. important::

   When Secure Boot is enabled, the SPI to AHB connection must be closed.

.. _efuse_bit29:

BIT(29) - flash aes enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(29) is used to enable FLASH AES encryption:

  - 0 - FLASH AES encryption disabled.
  - 1 - FLASH AES encryption enable. This must configure :ref:`FASH AES KEY <otp_flash_aes_key>`.

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

  - 0 - CPU debugging is enabled, BK7239 supports SWD debugging.
  - 1 - CPU debugging off. At this time, the CPU debugging function must be enabled through :ref:`secure debugging <security_secure_debug>`.

.. important::

  When secure boot is enabled, SWD debugging needs to be turned off.
