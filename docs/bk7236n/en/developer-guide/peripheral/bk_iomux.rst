======
IOMUX
======

:link_to_translation:`zh_CN:[中文]`

Overview
========

The IOMUX (I/O Multiplexer), also referred to as the IO Matrix, is the hardware module in the BK7236N
that manages the function multiplexing of the GPIO pins.

The BK7236N provides **24 GPIOs** (GPIO_0 ~ GPIO_23). Each GPIO can be configured through the IOMUX
as either a general-purpose I/O or mapped to a peripheral function (UART, SPI, I2C, I2S, PWM, SDIO,
QSPI, and so on).

The IOMUX exposes one dedicated configuration register for each pin. The most important field is
``GPIOx_FUNC_SEL[6:0]``:

 - ``FUNC_SEL = 0 ~ 3``: the pin is used as a GPIO, corresponding to **High-Z / Input / Output /
   Input+Output** respectively.
 - ``FUNC_SEL = 4 ~ 94``: the pin is routed to a specific peripheral function (full crossbar: any
   GPIO can select any function).
 - ``FUNC_SEL = 95 ~ 127``: reserved, behaves as High-Z.

The main features of the IOMUX module on the BK7236N are:

 - **Function multiplexing**: 7-bit function-code field (0~127), with 95 valid peripheral functions.
 - **Direction control**: High-Z, input, output, and input/output.
 - **Push-pull output**: Push-pull drive is supported.
 - **Internal pull-up / pull-down**: Independent enable and direction selection.
 - **Configurable drive strength**: 4 levels (0~3).
 - **Interrupt generation**: Level-triggered (high / low) and edge-triggered (rising / falling).
 - **Aggregated registers**: Global registers to query the interrupt status and input level of all
   GPIOs, or to drive the output of all GPIOs, in a single access.
 - **Low-power management**: GPIO configuration backup / restore, wake-up source support.

.. important::

    Hardware constraint: **The same peripheral function cannot be mapped to multiple GPIOs
    simultaneously**. If more than one GPIO has its ``FUNC_SEL`` set to the same peripheral function
    code, the signals will collide and the behavior is undefined. Applications must ensure that each
    peripheral function is assigned to exactly one GPIO.

Hardware Architecture
=====================

Register Layout
---------------

The IOMUX register block is composed of the following registers (defined in
``middleware/soc/bk7236n/soc/iomx_struct.h``):

+-------------------------+----------------------------------------------------------+
| Register                | Function                                                 |
+=========================+==========================================================+
| ``dev_id``              | Device ID (read-only)                                    |
+-------------------------+----------------------------------------------------------+
| ``ver_id``              | Version ID (read-only)                                   |
+-------------------------+----------------------------------------------------------+
| ``clkg_reset``          | Clock gating and soft-reset control                      |
+-------------------------+----------------------------------------------------------+
| ``dev_status``          | Device status (read-only)                                |
+-------------------------+----------------------------------------------------------+
| ``gpio_cfg[N]``         | Per-GPIO configuration register (core register)          |
+-------------------------+----------------------------------------------------------+
| ``gpio_intsta``         | Aggregated interrupt status of all GPIOs                 |
+-------------------------+----------------------------------------------------------+
| ``gpio_input``          | Aggregated input levels of all GPIOs                     |
+-------------------------+----------------------------------------------------------+
| ``gpio_output``         | Aggregated output levels of all GPIOs                    |
+-------------------------+----------------------------------------------------------+
| ``gpio_func_*_atpg``    | Registers reserved for ATPG testing                      |
+-------------------------+----------------------------------------------------------+

The most important structure is the ``gpio_cfg`` array: each GPIO on the BK7236N has its own
independent 32-bit configuration register.

GPIO Configuration Register Fields
----------------------------------

The fields of each per-GPIO ``gpio_cfg`` register are summarized below:

+-------------------+---------+----------------------------------------------------+
| Field             | Bit     | Description                                        |
+===================+=========+====================================================+
| gpio_input        | [0]     | Input enable                                       |
+-------------------+---------+----------------------------------------------------+
| gpio_output       | [1]     | Output enable                                      |
+-------------------+---------+----------------------------------------------------+
| gpio_pull_mode    | [4]     | Pull direction (0: pull-down, 1: pull-up)          |
+-------------------+---------+----------------------------------------------------+
| gpio_pull_ena     | [5]     | Pull-up/-down enable                               |
+-------------------+---------+----------------------------------------------------+
| input_monitor     | [7]     | Input monitor enable                               |
+-------------------+---------+----------------------------------------------------+
| gpio_capacity     | [8:9]   | Drive strength (4 levels: 0~3)                     |
+-------------------+---------+----------------------------------------------------+
| gpio_int_type     | [10:11] | Interrupt type (low / high level, rising / falling)|
+-------------------+---------+----------------------------------------------------+
| gpio_int_ena      | [12]    | Interrupt enable                                   |
+-------------------+---------+----------------------------------------------------+
| gpio_int_clear    | [13]    | Interrupt clear                                    |
+-------------------+---------+----------------------------------------------------+
| gpio_state        | [16:20] | Current state                                      |
+-------------------+---------+----------------------------------------------------+
| gpio_func_sel     | [24:30] | Function-select code ``FUNC_SEL[6:0]``             |
+-------------------+---------+----------------------------------------------------+

.. note::

    ``gpio_func_sel`` is a 7-bit field (``FUNC_SEL[6:0]``) with a value range of 0~127, and it is
    the key field that selects which peripheral function the GPIO is connected to. The valid values
    are enumerated by ``IOMX_CODE_T`` in ``include/driver/hal/hal_io_matrix_types.h``, including
    ``FUNC_CODE_HIGH_Z``, ``FUNC_CODE_INPUT``, ``FUNC_CODE_OUTPUT``, ``FUNC_CODE_UART0_TXD``, etc.

.. note::

    In addition to the per-GPIO configuration registers, the IOMUX also provides a set of
    **aggregated registers** that cover all GPIOs:

     - ``gpio_intsta``: read the interrupt status of all GPIOs in a single access (24-bit aggregate).
     - ``gpio_input``: read the input level of all GPIOs.
     - ``gpio_output``: set the output level of all GPIOs.

    These registers significantly speed up use cases such as GPIO polling, keypad scanning, and
    parallel data capture.

Software Architecture
=====================

The IOMUX subsystem is organized in 4 software layers (Adapter / Driver / HAL / LL). It connects
the application-layer API above to the hardware registers below:

.. list-table::
    :header-rows: 1
    :widths: 8 18 32 42

    * - Level
      - Name
      - Code location
      - Responsibility
    * - 1
      - Application API
      - ``include/driver/gpio.h``
      - User-facing ``bk_gpio_xxx()`` interfaces, preserved for backward compatibility.
    * - 2
      - GPIO adapter
      - ``middleware/driver/io_matrix/v2.0/gpio_adapter.c``
      - Maps ``bk_gpio_*`` to ``bk_iomx_*``; provides the ``gpio_dev_t`` → ``IOMX_CODE_T``
        conversion (``contert_gpio_dev_to_iomx_code``) for backward compatibility with the legacy
        GPIO driver interface.
    * - 3
      - Driver layer
      - ``middleware/driver/io_matrix/v2.0/io_matrix_driver.c``
      - Exposes the ``bk_iomx_*`` API family (for example ``bk_iomx_driver_init`` and
        ``bk_iomx_set_gpio_func``); handles parameter checks, interrupt registration, ISR dispatch,
        and low-power backup/restore.
    * - 4
      - HAL layer
      - ``middleware/soc/common/hal/iomx_hal.c``
      - Wraps the LL register operations and exposes GPIO-id based APIs such as
        ``iomx_hal_set_func_code()``, ``iomx_hal_set_int_type()``, ``iomx_hal_bakup_configs()``.
    * - 5
      - LL layer
      - ``middleware/soc/bk7236n/hal/iomx_ll.h``
      - ``inline`` bit-field access to the IOMUX registers, providing atomic register-level
        operations. **This file is auto-generated by a script and must not be modified by hand**.
    * - 6
      - Hardware registers
      - ``middleware/soc/bk7236n/soc/iomx_struct.h``
      - IOMUX hardware register structure definitions.

Call direction: **Application → Adapter → Driver → HAL → LL → Hardware registers**; each layer calls
only the layer immediately below it.

Initialization Flow
===================

The GPIO subsystem is initialized by ``bk_gpio_driver_init()``, which internally calls
``bk_iomx_driver_init()``:

.. code-block:: text

    bk_gpio_driver_init()
        ├── bk_iomx_driver_init()
        │     ├── iomx_hal_init()                  # Soft-reset IOMUX, enable GPIO clock
        │     ├── bk_int_isr_register(INT_SRC_GPIO, iomx_gpio_isr)
        │     └── sys_drv_int_group2_enable(GPIO_INTERRUPT_CTRL_BIT)
        ├── gpio_keep_status_init()                # Init low-power keep-status table
        ├── gpio_record_wakeup_pin_id()            # Record wake-up source GPIOs
        └── gpio_dynamic_wakeup_source_init()      # Init dynamic wake-up source

.. note::

    After initialization, every GPIO is left in the High-Z state and no peripheral function is
    claimed. Applications must explicitly call ``bk_gpio_set_config()`` or
    ``bk_iomx_set_gpio_func()`` to configure each pin.

.. note::

    BK7236N does not enable the ``CONFIG_GPIO_DEFAULT_SET_SUPPORT`` option, so the static mapping
    tables ``GPIO_DEFAULT_DEV_CONFIG`` and ``GPIO_DEV_MAP`` in ``gpio_map.h`` **do not participate
    in the default initialization**; they are kept only as a legacy-compatibility artifact. After
    reset every GPIO takes its register reset value, and the application is free to configure it
    through the API.

Function Multiplexing
=====================

Unlike traditional "fixed-mapping" architectures, the IOMUX of the BK7236N is designed as a
**full crossbar**:

 - Each GPIO has its own 7-bit ``FUNC_SEL[6:0]`` field and can independently select any function
   code in the range 0~127.
 - **Any peripheral function can be mapped to any GPIO**; there are no fixed multiplexing groups.
 - This gives maximum flexibility for PCB routing and peripheral pin assignment.

The function codes are split into the following ranges:

+-----------+-------------------+---------------------------------------------+
| FUNC_SEL  | Type              | Description                                 |
+===========+===================+=============================================+
| 0         | GPIO              | High-Z                                      |
+-----------+-------------------+---------------------------------------------+
| 1         | GPIO              | Input                                       |
+-----------+-------------------+---------------------------------------------+
| 2         | GPIO              | Output                                      |
+-----------+-------------------+---------------------------------------------+
| 3         | GPIO              | Input / Output                              |
+-----------+-------------------+---------------------------------------------+
| 4 ~ 94    | Peripheral I/O    | Connected to a specific peripheral          |
|           |                   | (UART / SPI / PWM / ...)                    |
+-----------+-------------------+---------------------------------------------+
| 95 ~ 127  | Reserved          | Equivalent to High-Z                        |
+-----------+-------------------+---------------------------------------------+

For example, to route UART0_TXD to GPIO_5, simply call:

.. code-block:: c

    bk_iomx_set_gpio_func(GPIO_5, FUNC_CODE_UART0_TXD);

which is equivalent to writing ``gpio_cfg[5].gpio_func_sel = FUNC_CODE_UART0_TXD``.

.. important::

    **The same peripheral function cannot be mapped to multiple GPIOs at the same time**. For
    instance, GPIO_5 and GPIO_10 must not both be set to ``FUNC_CODE_UART0_TXD``, otherwise the
    signals will collide and the behavior is undefined. Before switching a peripheral to a new pin,
    set the previous GPIO to ``FUNC_CODE_HIGH_Z`` first, then configure the new GPIO.

.. note::

    Even though the hardware allows any function to be routed to any GPIO, in practice you should
    still follow the recommendations in the hardware manual: high-speed signals (QSPI, SDIO, I2S,
    etc.) have impedance and length-matching requirements on the PCB, so the pins suggested in the
    manual should be preferred.

Hardware Function Code List
---------------------------

The valid values of the ``gpio_func_sel`` field are defined by the hardware. The complete list is
given in the table below (excerpted from the hardware manual):

+--------+------------------+--------+
| Code   | Function         | Dir    |
+========+==================+========+
| 0      | GPIO             | Z      |
+--------+------------------+--------+
| 1      | GPIO             | I      |
+--------+------------------+--------+
| 2      | GPIO             | O      |
+--------+------------------+--------+
| 3      | GPIO             | IO     |
+--------+------------------+--------+
| 4      | BT_ACTIVE        | I      |
+--------+------------------+--------+
| 5~8    | BT_ANT[0..3]     | O      |
+--------+------------------+--------+
| 9      | BT_PRIORITY      | I      |
+--------+------------------+--------+
| 10     | CLK_AUXS         | O      |
+--------+------------------+--------+
| 11     | CLK_XTAL         | O      |
+--------+------------------+--------+
| 12     | CLK_XTAL_DIV     | O      |
+--------+------------------+--------+
| 13~28  | DEBUG[0..15]     | O      |
+--------+------------------+--------+
| 29     | FEM_LNA_EN       | O      |
+--------+------------------+--------+
| 30~33  | I2C0/1 SCL/SDA   | IOZ    |
+--------+------------------+--------+
| 34     | I2S_MCLK         | O      |
+--------+------------------+--------+
| 35~38  | I2S0 DIN/DOUT/   | I/O/IO |
|        | SCK/SYNC         |        |
+--------+------------------+--------+
| 39     | LEDC             | O      |
+--------+------------------+--------+
| 40     | LPO_CLK          | O      |
+--------+------------------+--------+
| 41~43  | OTP_FRE_*        | I/O    |
+--------+------------------+--------+
| 44~55  | PWM0[0..11]      | IO     |
+--------+------------------+--------+
| 56~61  | QSPI0 CS/DAT/SCK | IO/IOZ |
+--------+------------------+--------+
| 62~67  | SDIO CLK/CMD/DAT | IO/IOZ |
+--------+------------------+--------+
| 68~75  | SPI0/1 MISO/MOSI/| IO     |
|        | NSS/SCK          |        |
+--------+------------------+--------+
| 76     | SWCLK            | I      |
+--------+------------------+--------+
| 77     | SWDIO            | IO     |
+--------+------------------+--------+
| 78~79  | TAMP_RX/TX       | I/O    |
+--------+------------------+--------+
| 80~91  | UART0~3          | I/O    |
|        | CTS/RTS/RXD/TXD  |        |
+--------+------------------+--------+
| 92     | WIFI_ACTIVE      | O      |
+--------+------------------+--------+
| 93~94  | WIFI_RX/TX_EN    | O      |
+--------+------------------+--------+
| 95~127 | GPIO (reserved)  | Z      |
+--------+------------------+--------+

The software side defines the matching ``FUNC_CODE_XXX`` enumeration in
``include/driver/hal/hal_io_matrix_types.h``. Using the enumeration instead of raw numbers is
strongly recommended for readability and maintainability.

When a pin function is switched, the internal call flow is:

.. code-block:: text

    bk_gpio_enable_output(gpio_id)
        └── bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_OUTPUT)
              └── iomx_hal_set_func_code(gpio_id, FUNC_CODE_OUTPUT)
                    └── iomx_ll_set_gpio_function_sel(FUNC_CODE_OUTPUT, gpio_id)
                          └── write gpio_cfg[gpio_id].gpio_func_sel = FUNC_CODE_OUTPUT

Interrupt Handling
==================

All 24 GPIOs share a single interrupt vector (``INT_SRC_GPIO``). The interrupt processing flow is:

.. code-block:: text

    GPIO hardware interrupt
        └── iomx_gpio_isr()                         # Unified ISR in the Driver layer
              ├── iomx_hal_get_interrupt_status()   # Read gpio_intsta
              ├── iterate over the 24 GPIOs
              │    ├── check which GPIO triggered
              │    ├── invoke the isr registered for that gpio_id
              │    └── bk_iomx_clear_interrupt()    # Clear the interrupt status
              └── return

The application registers an interrupt callback through ``bk_gpio_register_isr(gpio_id, isr)``; the
callback is stored internally in the ``s_iomx_gpio_isr[]`` array. The interrupt type is configured
through ``bk_gpio_set_interrupt_type()`` and must be one of the following:

 - ``GPIO_INT_TYPE_LOW_LEVEL``
 - ``GPIO_INT_TYPE_HIGH_LEVEL``
 - ``GPIO_INT_TYPE_RISING_EDGE``
 - ``GPIO_INT_TYPE_FALLING_EDGE``

.. note::

    Before calling ``bk_iomx_enable_interrupt()``, the driver inserts a short busy-wait loop
    (approximately 1000 empty iterations) so that the internal signals settle, preventing spurious
    interrupts.

Low-Power Management
====================

Backup / Restore of GPIO Configuration on Low-Power Entry
---------------------------------------------------------

When the system enters a low-power mode, the driver automatically performs the following steps
(see ``gpio_enter_low_power()``) so that the GPIO state remains consistent after wake-up:

1. ``iomx_backup_gpio_configs()`` saves the ``gpio_cfg`` register of every GPIO into RAM.
2. ``iomx_disable_all_interrupts()`` disables all GPIO interrupts to prevent spurious triggers
   during sleep.
3. Apply the keep-status configuration (``gpio_keep_status_config``).
4. Apply the wake-up-source configuration (``gpio_wakeup_source_config``).
5. All GPIOs that are neither wake-up sources nor configured to keep their status are switched to
   High-Z to reduce leakage current.

On wake-up, ``gpio_exit_low_power()`` is called:

1. ``iomx_restore_gpio_configs()`` restores every GPIO configuration register from RAM.
2. Clear the wake-up-source status produced by the ANA GPIO.

.. note::

    After exiting Low-Voltage mode, the GPIO state is restored automatically. Exiting Deep-Sleep is
    equivalent to a soft reboot, so the GPIO state is **not** restored automatically in that case.

Wake-Up Source Configuration
----------------------------

There are two ways to configure a GPIO as a wake-up source:

1. **Static configuration**: declare it in the ``GPIO_STATIC_WAKEUP_SOURCE_MAP`` table of
   ``gpio_map.h``.
2. **Dynamic configuration**: call ``bk_gpio_register_wakeup_source()`` and the related APIs at
   runtime.

The interrupt type of a wake-up source should be set to an edge trigger
(``GPIO_INT_TYPE_RISING_EDGE`` or ``GPIO_INT_TYPE_FALLING_EDGE``), and ``low_power_io_ctrl`` should
be configured as ``GPIO_LOW_POWER_KEEP_INPUT_STATUS``.

API Usage Example
=================

A typical GPIO usage pattern is shown below:

.. code-block:: c

    #include <driver/gpio.h>

    void gpio_example(void)
    {
        /* 1. Initialize the GPIO driver */
        bk_gpio_driver_init();

        /* 2. Configure GPIO_10 as input with pull-up */
        gpio_config_t cfg = {
            .io_mode   = GPIO_INPUT_ENABLE,
            .pull_mode = GPIO_PULL_UP_EN,
        };
        bk_gpio_set_config(GPIO_10, &cfg);

        /* 3. Register interrupt on falling edge */
        bk_gpio_register_isr(GPIO_10, my_gpio_isr);
        bk_gpio_set_interrupt_type(GPIO_10, GPIO_INT_TYPE_FALLING_EDGE);
        bk_gpio_enable_interrupt(GPIO_10);

        /* 4. Use GPIO_11 as output */
        bk_gpio_enable_output(GPIO_11);
        bk_gpio_set_output_high(GPIO_11);
    }

    static void my_gpio_isr(gpio_id_t id)
    {
        /* handle the interrupt */
    }

Key API Reference
=================

Public GPIO API (``include/driver/gpio.h``)
-------------------------------------------

+-----------------------------------------+----------------------------------+
| API                                     | Description                      |
+=========================================+==================================+
| ``bk_gpio_driver_init``                 | Initialize the GPIO/IOMUX driver |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_set_config``                  | Configure the GPIO working mode  |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_enable_output``               | Enable output mode               |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_enable_input``                | Enable input mode                |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_set_output_high/low``         | Drive the output level           |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_get_input``                   | Read the input level             |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_pull_up/pull_down``           | Enable internal pull-up/-down    |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_register_isr``                | Register an interrupt callback   |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_set_interrupt_type``          | Set the interrupt trigger type   |
+-----------------------------------------+----------------------------------+
| ``bk_gpio_enable_interrupt``            | Enable the interrupt             |
+-----------------------------------------+----------------------------------+

Low-Level IOMUX API (``include/driver/io_matrix.h``)
----------------------------------------------------

+-----------------------------------------+-----------------------------------------+
| API                                     | Description                             |
+=========================================+=========================================+
| ``bk_iomx_set_gpio_func``               | Set the GPIO function code directly     |
|                                         | (``FUNC_CODE_XXX``)                     |
+-----------------------------------------+-----------------------------------------+
| ``bk_iomx_get_gpio_func_code``          | Get the current function code           |
+-----------------------------------------+-----------------------------------------+
| ``bk_iomx_set_value``                   | Write the whole ``gpio_cfg`` register   |
+-----------------------------------------+-----------------------------------------+
| ``bk_iomx_get_value``                   | Read the whole ``gpio_cfg`` register    |
+-----------------------------------------+-----------------------------------------+
| ``bk_iomx_set_capacity``                | Set the drive strength (0~3)            |
+-----------------------------------------+-----------------------------------------+

Notes and Best Practices
========================

1. A GPIO can only be mapped to a single peripheral function at any given time. Before configuring
   a peripheral, the upper-layer driver should check whether the GPIO is already claimed. If
   ``bk_gpio_set_config()`` returns ``GPIO_ERR_INTERNAL_USED``, the GPIO is in use by another
   peripheral.

2. When switching the function of a GPIO, it is recommended to clear the register first by calling
   ``bk_iomx_set_value(id, 0)`` and then configure the new function, to avoid leftover settings
   interfering with the new function.

3. The drive strength (``GPIO_DRIVER_CAPACITY_0`` ~ ``GPIO_DRIVER_CAPACITY_3``) should be selected
   according to the actual load current: a higher drive strength also means higher power
   consumption. Buses that need a strong drive (such as I2C) are typically configured as
   ``GPIO_DRIVER_CAPACITY_3``.

4. Always register the callback via ``bk_gpio_register_isr()`` before enabling the interrupt for a
   GPIO. Otherwise, when the interrupt is triggered the callback pointer will be NULL and the
   driver will only clear the interrupt status without performing any handling.

5. In low-power mode, any GPIO that is not explicitly configured as a wake-up source or as a
   keep-status pin is automatically switched to High-Z to reduce leakage current.
