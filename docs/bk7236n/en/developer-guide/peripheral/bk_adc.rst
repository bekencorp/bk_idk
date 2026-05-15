======
SARADC
======

:link_to_translation:`zh_CN:[中文]`

Overview
========

BK7236N integrates a 14-bit **SARADC** (Successive Approximation Analog-to-Digital
Converter), supporting two sampling modes: One-shot and Continuous.

Key features:

 - **Resolution**: 14-bit.
 - **Programmable sample rate**: 51.28 kHz – 1.538 MHz.
 - **Up to 10 external analog input channels**: ADC2 / ADC3 / ADC4 / ADC5 / ADC6 /
   ADC10 / ADC12 / ADC13 / ADC14 / ADC15.
 - **5 internal dedicated channels**:

   - ADC0: battery-voltage monitor (VBAT/4)
   - ADC7: internal temperature sensor (TEMP)
   - ADC8: TSSIO
   - ADC9 / ADC11: internal debug channels

 - **Sampling modes**:

   - One-shot / Single-step: collect a single batch of samples and stop, waiting for
     the MCU to read.
   - Continuous: hardware keeps sampling at the configured rate; an interrupt fires
     when the FIFO occupancy reaches the watermark.

 - **Hardware FIFO**: an internal FIFO buffers the sampled data; in continuous mode
   it is delivered to software via the FIFO-watermark interrupt.
 - **Per-bit weighted calibration (CWT)**: 16 24-bit ``cwt_calXX`` registers correct
   each of the 16 SAR bits' weights, on top of which a gain ``co_gain`` and offset
   ``co_offset`` are applied to improve linearity and unit-to-unit consistency.

Hardware Architecture
=====================

Register Layout
---------------

The SARADC register block (defined in ``middleware/soc/bk7236n/soc/adc_struct.h``):

+-----------------------+---------+--------------------------------------------------+
| Register              | Offset  | Function                                         |
+=======================+=========+==================================================+
| ``reg0``              | 0x00    | Device ID (``0x53414443`` = "SADC", read-only)   |
+-----------------------+---------+--------------------------------------------------+
| ``reg1``              | 0x04    | Version ID (``0x00010002``, read-only)           |
+-----------------------+---------+--------------------------------------------------+
| ``reg2``              | 0x08    | Soft reset / pclk gating bypass control          |
+-----------------------+---------+--------------------------------------------------+
| ``reg3``              | 0x0C    | Global status (read-only)                        |
+-----------------------+---------+--------------------------------------------------+
| ``reg4``              | 0x10    | Core control: enable, mode, channel, clock div   |
+-----------------------+---------+--------------------------------------------------+
| ``reg5``              | 0x14    | Calibration gain                                 |
+-----------------------+---------+--------------------------------------------------+
| ``reg6``              | 0x18    | Calibration offset                               |
+-----------------------+---------+--------------------------------------------------+
| ``reg7`` ~ ``reg16``  | 0x1C ~  | 16 ``cwt_cal00`` ~ ``cwt_cal0f`` per-bit weight  |
|                       | 0x58    | coefficients (24 bit each)                       |
+-----------------------+---------+--------------------------------------------------+
| ``reg17``             | 0x5C    | Interrupt enable, FIFO watermark                 |
+-----------------------+---------+--------------------------------------------------+
| ``reg18``             | 0x60    | FIFO read port (16-bit)                          |
+-----------------------+---------+--------------------------------------------------+
| ``reg19``             | 0x64    | FIFO status, interrupt status (write-1-to-clear) |
+-----------------------+---------+--------------------------------------------------+

Core Control Register (reg4)
----------------------------

``reg4`` is the central SARADC control register. Its bit fields are:

+-------------------+---------+----------------------------------------------------+
| Field             | Bit     | Description                                        |
+===================+=========+====================================================+
| enable            | [0]     | ADC enable (1: start sampling)                     |
+-------------------+---------+----------------------------------------------------+
| calib_mode        | [1]     | Calibration mode: 1 outputs raw samples for LMS    |
+-------------------+---------+----------------------------------------------------+
| calib_done        | [2]     | Software calibration done flag: 1 outputs the      |
|                   |         | calibrated data                                    |
+-------------------+---------+----------------------------------------------------+
| samp_sel          | [3]     | Sampling clock edge select                         |
+-------------------+---------+----------------------------------------------------+
| adc_chan          | [7:4]   | Channel select (0~15, maps to ADC0~ADC15)          |
+-------------------+---------+----------------------------------------------------+
| adc_mode          | [8]     | Operating mode (0: single; 1: continuous)          |
+-------------------+---------+----------------------------------------------------+
| adc_dump_num      | [11:9]  | Number of leading samples discarded at start       |
+-------------------+---------+----------------------------------------------------+
| adc_sclk_div      | [15:12] | Sampling clock divider ``adc_clk = src/(div * 2)`` |
+-------------------+---------+----------------------------------------------------+
| adc_sraw          | [16]    | In normal mode, 1 outputs raw, 0 outputs           |
|                   |         | calibrated data                                    |
+-------------------+---------+----------------------------------------------------+
| adc_sat           | [17]    | Data truncation: 0 → ``[15:0]``;                   |
|                   |         | 1 → ``{[15:2], 2'b00}``                            |
+-------------------+---------+----------------------------------------------------+

Interrupt & FIFO Registers (reg17 / reg18 / reg19)
--------------------------------------------------

+-------------------+----------+----------------------------------------------------+
| Field             | Register | Description                                        |
+===================+==========+====================================================+
| adc_int_en        | reg17[0] | Interrupt enable                                   |
+-------------------+----------+----------------------------------------------------+
| adc_int_level     | reg17    | FIFO interrupt watermark: interrupt fires when     |
|                   | [6:1]    | FIFO occupancy > level (continuous mode only;      |
|                   |          | single-step is fixed at 2 samples)                 |
+-------------------+----------+----------------------------------------------------+
| fifo_rdata        | reg18    | FIFO read port (16-bit; each read pops one entry)  |
+-------------------+----------+----------------------------------------------------+
| fifo_full         | reg19[0] | FIFO full flag (read-only)                         |
+-------------------+----------+----------------------------------------------------+
| fifo_empty        | reg19[1] | FIFO empty flag (read-only, reset value = 1)       |
+-------------------+----------+----------------------------------------------------+
| int_state         | reg19[2] | Interrupt status flag (write 1 to clear, RW1C)     |
+-------------------+----------+----------------------------------------------------+

Calibration Coefficient Registers (reg5 / reg6 / reg7~reg16)
------------------------------------------------------------

The ADC output is computed as follows, where ``ADCout`` is the final 14-bit sample
value and ``adci<k>`` is the raw SAR comparison result for bit *k*:

.. math::

    ADCout_{[13:0]} = \mathrm{Round}\Big( (co\_gain + 1) \times \sum_{k=0}^{15} adci_{<k>} \times reg\_cwt\_ini_{<k>}\;+\;co\_offset \Big)

- ``co_gain`` (reg5): 8-bit gain correction
- ``co_offset`` (reg6): 16-bit offset correction
- ``cwt_cal00`` ~ ``cwt_cal0f`` (reg7 ~ reg16): 16 24-bit per-bit weight coefficients

These coefficients can be computed online by the LMS (Least Mean Squares) algorithm
in software. See :ref:`saradc-calibration` for details.

Software Architecture
=====================

The SARADC software stack has 4 layers:

.. list-table::
    :header-rows: 1
    :widths: 6 14 42 38

    * - Layer
      - Name
      - Source
      - Responsibility
    * - 1
      - Application API
      - ``include/driver/adc.h``
      - User-facing ``bk_adc_*`` API: channel init, single/continuous sampling,
        ISR-callback registration, etc.
    * - 2
      - Driver
      - ``middleware/driver/saradc/v1_2/adc_driver.c``
      - Implements all ``bk_adc_*`` business logic: power/clock control, GPIO
        muxing, ISR dispatch, FIFO drain, low-power callbacks, CWT calibration
        entry, etc.
    * - 3
      - HAL
      - ``middleware/soc/bk7236n/hal/adc_hal.c``
      - Provides the ``adc_hal_*`` abstract interface (init / set_clk / set_mode /
        start / get_data, etc.); hosts the LMS CWT algorithm ``adc_cali_lms()``.
    * - 4
      - LL
      - ``middleware/soc/bk7236n/hal/adc_ll.h``
      - ``inline`` register bit-field accessors. **This file is auto-generated;
        do not edit by hand.**
    * - 5
      - Hardware registers
      - ``middleware/soc/bk7236n/soc/adc_struct.h``
      - SARADC register struct definition.

Call direction: **Application → Driver → HAL → LL → Hardware registers**.

Channel & GPIO Mapping
======================

ADC channels are split into two categories:

+--------------+---------------------------------+--------------------------------------------+
| Category     | Channel                         | Description                                |
+==============+=================================+============================================+
| Internal /   | ADC0                            | VBAT/4 battery-voltage monitor             |
| dedicated    +---------------------------------+--------------------------------------------+
|              | ADC7                            | Internal temperature sensor (TEMP)         |
|              +---------------------------------+--------------------------------------------+
|              | ADC8                            | TSSIO                                      |
|              +---------------------------------+--------------------------------------------+
|              | ADC9 / ADC11                    | Internal debug channels                    |
+--------------+---------------------------------+--------------------------------------------+
| External /   | ADC2 / 3 / 4 / 5 / 6            | Mapped to GPIO_3 ~ GPIO_7                  |
| analog       +---------------------------------+--------------------------------------------+
|              | ADC10                           | Mapped to GPIO_8                           |
|              +---------------------------------+--------------------------------------------+
|              | ADC12 / 13 / 14 / 15            | Mapped to GPIO_0 / GPIO_1 / GPIO_12 /      |
|              |                                 | GPIO_13                                    |
+--------------+---------------------------------+--------------------------------------------+

The full channel-to-GPIO table is given by ``ADC_DEV_MAP`` in
``middleware/soc/bk7236n/soc/adc_map.h``:

.. code-block:: c

    /* ADC_DEV_MAP (excerpt) */
    {ADC_2,  GPIO_3,  GPIO_DEV_ADC2 },
    {ADC_3,  GPIO_4,  GPIO_DEV_ADC3 },
    ...
    {ADC_14, GPIO_12, GPIO_DEV_ADC14},
    {ADC_15, GPIO_13, GPIO_DEV_ADC15},

The mapping between an ADC channel and its GPIO is **fixed in hardware**: unlike
digital peripherals such as UART or SPI, no IOMUX function-code configuration is
required — simply leaving the GPIO in its default GPIO mode is enough. The driver
takes care of this internally inside ``bk_adc_channel_raw_read()`` /
``bk_adc_channel_read()`` via ``adc_init_gpio()``; the application does not need to
intervene.

.. note::

    The SARADC hardware can only sample one channel at a time. The driver provides
    its own mutex inside ``bk_adc_channel_raw_read()`` /
    ``bk_adc_channel_read()``, so concurrent reads from multiple tasks targeting
    different channels are safe and need no extra locking on the application side.

Operating Modes
===============

The BK7236N SARADC supports two hardware operating modes, selected by the single
``reg4.adc_mode`` bit and exposed through ``adc_mode_t``:

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - Mode
      - Description
    * - ``ADC_SINGLE_STEP_MODE``
      - Single-step mode: once enabled, the hardware writes 2 samples into the FIFO
        and stops, waiting for the MCU to read; the next sample requires re-enabling.
    * - ``ADC_CONTINUOUS_MODE``
      - Continuous mode: the hardware keeps writing into the FIFO at the configured
        ``sample_rate``; an interrupt fires when the FIFO occupancy exceeds
        ``adc_int_level``.

.. note::

    ``adc_mode_t`` also defines ``ADC_SLEEP_MODE`` and ``ADC_SOFTWARE_CONTRL_MODE``;
    those are **legacy values retained for old-chip compatibility**. On BK7236N the
    driver silently ignores them. New code must pick one of single-step / continuous
    only.

Clock & Sample Rate
===================

The SARADC clock source is **fixed in hardware to XTAL 40 MHz** and cannot be
switched in software. The other values in the ``adc_src_clk_t`` enum
(``ADC_SCLK_DCO`` / ``ADC_SCLK_32M`` / ``ADC_SCLK_DPLL``) are retained for old-chip
compatibility and are not usable on BK7236N.

The only software knob is the 4-bit divider ``adc_sclk_div`` in ``reg4[15:12]``,
which determines the sampling clock as
``adc_clk = 40 MHz / (adc_sclk_div × 2)``. With ``adc_sclk_div`` from 1 to 15,
``adc_clk`` ranges roughly **1.33 MHz ~ 20 MHz**.

The recommended approach is to set the desired ``adc_clk`` in ``adc_config_t.clk``
and pass it together with the other fields to ``bk_adc_channel_init()``; the driver
computes the divider and writes the register. Below is the real init code used by
the RF calibration module (see ``components/bk_phy/src/bk_phy_adapter.c``):

.. code-block:: c

    adc_config_t config = {0};
    config.chan          = adc_channel;             /* required */
    config.adc_mode      = ADC_CONTINUOUS_MODE;     /* required: single / continuous */
    config.src_clk       = ADC_SCLK_XTAL;           /* fixed: must be XTAL */
    config.clk           = 5 * 1000 * 1000;         /* adc_clk = 5 MHz, i.e. sclk_div = 4 */
    /* The fields below have no effect on BK7236N; leave them at default 0.
     * They are kept here only for source-level compatibility with old-chip code. */
    config.saturate_mode = ADC_SATURATE_MODE_3;
    config.sample_rate   = 0;
    config.steady_ctrl   = steady_time;
    config.adc_filter    = 0;

    bk_adc_channel_init(&config);

.. warning::

    The driver writes the 4-bit divider directly from
    ``adc_sclk_div = 40 MHz / (2 × config.clk)`` and the hardware **takes only the
    low 4 bits**, with no range check and no error reporting. If ``config.clk`` is
    below 1.33 MHz, ``adc_sclk_div`` will be ≥ 16, get truncated to a wrong value,
    and the actual ``adc_clk`` will deviate from the expected one. Make sure
    ``config.clk`` stays in the **1.33 MHz ~ 20 MHz** range.

.. _saradc-calibration:

Calibration Mechanism
=====================

SARADC calibration consists of two parts:

1. **CWT coefficients (the core)**: identify the 16 per-bit weights and write them
   to the ``cwt_cal00`` ~ ``cwt_cal0f`` registers.
2. **Gain / offset correction**: write to ``co_gain`` (reg5) and ``co_offset``
   (reg6).

There are two sources for the CWT coefficients. Inside ``bk_adc_driver_init()`` →
``bk_adc_enter_calib_mode()`` the driver picks one with an **OTP-first,
LMS-fallback** policy:

.. code-block:: text

    bk_adc_enter_calib_mode()                         # only with CONFIG_SARADC_V1P2
        ├── adc_try_load_otp_cwt()                    # priority: load from OTP
        │     └── read OTP_GADC_CALIBRATION (72 B); validate last byte != 0
        │
        ├── if OTP load succeeds ─────► adc_hal_set_cwt(otp_cwt)   # write to regs
        │                              (skip LMS, near-zero latency)
        │
        └── if OTP load fails (blank chip / validation failure)
              ├── adc_hal_calib_init()               # reset + analog power up
              ├── collect ADC_CALIBRATION_DATA_NUM raw samples
              │     (calib_mode=1, sraw=1, gain=0, offset=0)
              ├── adc_hal_set_cwt_calib(buf, n)      # invoke LMS
              │     └── adc_cali_lms()               # iterate over 16 bit weights
              └── write back cwt_cal00..0f, calib_done=1   # tens of ms

Under normal conditions BK7236N has its ADC calibrated at the CP/FT production stage
and the resulting CWT coefficients are programmed into the OTP region
``OTP_GADC_CALIBRATION`` (72 bytes, including the backup region
``OTP_GADC_CALIBRATION_BACKUP``). On a production part,
``adc_try_load_otp_cwt()`` finds valid coefficients and applies them directly; the
application is unaware.

Only on a "blank chip" — where the OTP holds no valid calibration value (e.g. an
engineering sample at development time, or a part whose OTP got corrupted) — does
the driver fall back to the LMS online calibration path. A single LMS run takes
roughly tens of milliseconds.

Data Path
=========

The driver maintains two layers of buffer:

.. list-table::
    :header-rows: 1
    :widths: 22 78

    * - Buffer
      - Description
    * - **Hardware FIFO**
      - Built into the IP. Each completed sample pushes a 16-bit ADC datum;
        ``reg18.fifo_rdata`` is the read port and ``reg19.fifo_full`` /
        ``reg19.fifo_empty`` reflect the full / empty state.
    * - **Software Buffer**
      - The ``uint16_t* buf`` passed in by the application call to
        ``bk_adc_channel_raw_read()``. The ISR drains the FIFO into this buffer
        whenever it is non-empty.

Continuous-mode data flow:

.. code-block:: text

    ADC HW   ──sample──>  HW FIFO
                            │ adc_int_level triggers
                            ▼
                         INT_SRC_SARADC
                            │
                            ▼
                    adc_isr() (Driver)
                            │
                            ├── clear interrupt status
                            ├── while (!fifo_empty):
                            │      buf[i++] = fifo_rdata
                            ├── when i >= sampling_request:
                            │      adc_stop_conversion()
                            │      rtos_set_semaphore(sync)
                            └── invoke user-registered isr_cb (if any)

The application thread waits for sampling completion via
``rtos_get_semaphore(&dev->ctx.sync, timeout)``.

Interrupt Handling
==================

The ADC interrupt source is ``INT_SRC_SARADC``. ``bk_adc_driver_init()`` registers
a unified ISR ``adc_isr()`` that does:

1. ``adc_hal_clear_int_status()`` — write ``int_state = 1`` to clear the flag.
2. Drain the FIFO until ``fifo_empty = 1``, copying samples into the software
   buffer.
3. Once the buffer is full, stop conversion and signal the semaphore.
4. Invoke the user callback registered via ``bk_adc_register_isr_callback()`` (if
   any).

Applications can fetch data in two ways:

 - **Synchronously**: call ``bk_adc_channel_read()`` /
   ``bk_adc_channel_raw_read()``; the driver waits on the internal semaphore.
 - **Asynchronously**: register an ``adc_isr_t`` callback; the driver forwards each
   interrupt to it.

Low-Power Management
====================

The SARADC sits inside the **APB-BAKP** power domain and is registered with the
low-power subsystem under ``PM_DEV_ID_SARADC``. The relevant callbacks and power
votes are:

 - ``bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_SADC, ON)``: voted
   in ``bk_adc_channel_init()`` to power up the SADC sub-module of the APB-BAKP
   domain.
 - ``adc_pm_restore_cb()``: invoked only after a ``PM_MODE_LOW_VOLTAGE`` wake-up.
   Internally it calls ``hal_calib_apply()`` to perform an ADC soft reset, write
   the reg4 baseline, restore CWT coefficients, mask interrupts, etc., bringing
   the ADC back to a working state.
 - If ``CONFIG_SARADC_PM_CB_SUPPORT`` is not enabled, the application must
   re-invoke ``bk_adc_driver_init()`` and ``bk_adc_channel_init()`` itself after
   wake-up.

.. note::

    **Low-Voltage mode**: to drive power down further, the APB-BAKP domain is shut
    off, which clears all of the SARADC digital registers attached to it. After
    wake-up:

     - **CWT calibration coefficients**: the driver writes them back into hardware
       automatically via ``adc_pm_restore_cb()`` → ``hal_calib_apply()``, using the
       copy cached in RAM.
     - **Application-level configuration** (``chan`` / ``adc_mode`` / ``clk`` …):
       these are *not* restored inside the PM callback. Instead, the next call to
       ``bk_adc_channel_raw_read()`` / ``bk_adc_channel_read()`` re-applies them
       internally via ``adc_activate_config()``.

    As a result, no extra application-side initialization is needed after an LV
    wake-up.

    **Deep-Sleep mode**: the entire SARADC (analog included) is fully powered off;
    a wake-up is equivalent to a soft reboot, and the application must re-invoke
    ``bk_adc_driver_init()`` and ``bk_adc_channel_init()``.

API Usage Examples
==================

``adc_config_t`` Field Notes
----------------------------

.. note::

    Of all fields in ``adc_config_t``, only **4** actually affect BK7236N hardware
    behaviour: ``chan``, ``adc_mode``, ``src_clk`` (which can only be set to
    ``ADC_SCLK_XTAL``), and ``clk``. The remaining fields are **silently ignored
    by the driver** on BK7236N — the HAL layer is an empty macro / empty function
    for each of them and writes nothing to hardware. They are kept solely for
    source compatibility with old-chip configuration code:

    +------------------------------+--------------------------------------------------------------+
    | Field                        | Effect on BK7236N                                            |
    +==============================+==============================================================+
    | ``sample_rate``              | ``adc_hal_set_sample_rate`` is an empty macro                |
    +------------------------------+--------------------------------------------------------------+
    | ``adc_filter``               | ``adc_hal_set_adc_filter`` is an empty macro                 |
    +------------------------------+--------------------------------------------------------------+
    | ``steady_ctrl``              | ``adc_hal_set_steady_ctrl`` is an empty macro                |
    +------------------------------+--------------------------------------------------------------+
    | ``saturate_mode``            | ``adc_hal_set_saturate_mode`` is an empty function           |
    +------------------------------+--------------------------------------------------------------+
    | ``vol_div``                  | ``adc_hal_set_vol_div`` is an empty function                 |
    +------------------------------+--------------------------------------------------------------+
    | ``is_hw_using_cali_result``  | The branched ``adc_hal_enable/disable_bypass_calib`` are     |
    |                              | empty macros                                                 |
    +------------------------------+--------------------------------------------------------------+

    Additionally, ``adc_dump_num`` (the number of leading samples discarded at
    start-up) is hard-coded to 5 inside ``adc_hal_init()``; it is neither exposed
    to nor configurable by the application.

Continuous-Mode Sampling Example
--------------------------------

A typical flow that captures 32 raw samples in continuous mode:

.. code-block:: c

    #include <driver/adc.h>

    #define ADC_SAMPLE_NUM   32

    void adc_continuous_example(void)
    {
        uint16_t buf[ADC_SAMPLE_NUM] = {0};

        /* 1. Initialize the ADC driver (CWT calibration is done inside) */
        bk_adc_driver_init();

        /* 2. Configure channel ADC_3 (GPIO_4) */
        adc_config_t cfg = {
            /* required fields */
            .chan         = ADC_3,
            .adc_mode     = ADC_CONTINUOUS_MODE,
            .src_clk      = ADC_SCLK_XTAL,
            .clk          = 5 * 1000 * 1000,   /* adc_clk = 5 MHz */
            /* The fields below have no effect on BK7236N (see the field notes
             * above); leaving them at default 0 is fine. */
            .sample_rate  = 0,
            .adc_filter   = 0,
            .steady_ctrl  = 0,
            .saturate_mode = 0,
            .vol_div      = 0,
            .is_hw_using_cali_result = 0,
        };
        bk_adc_channel_init(&cfg);

        /* 3. One synchronous capture (driver starts hw, waits IRQ, stops hw) */
        /*    The 3rd argument is a sample count (number of uint16_t entries),  */
        /*    not a byte count.                                                 */
        bk_adc_channel_raw_read(ADC_3, buf, ADC_SAMPLE_NUM, BEKEN_WAIT_FOREVER);

        /* 4. Tear down */
        bk_adc_channel_deinit(ADC_3);
    }

For single-step mode, switch ``adc_mode`` to ``ADC_SINGLE_STEP_MODE`` and call
``bk_adc_single_read(&data)`` to read.

Read the average value (the driver discards the first half and averages the second
half internally):

.. code-block:: c

    uint16_t avg = 0;
    bk_adc_channel_read(ADC_3, &avg, BEKEN_WAIT_FOREVER);

Convert a raw ADC code to a voltage (mV):

.. code-block:: c

    float voltage_mv = bk_adc_data_calculate(adc_val, ADC_3);

Key API Reference
=================

The table below lists only the ADC APIs that are **actually usable** on BK7236N.
All declarations live in ``include/driver/adc.h``.

Driver Init & Channel Management
--------------------------------

+-----------------------------------------+--------------------------------------+
| API                                     | Description                          |
+=========================================+======================================+
| ``bk_adc_driver_init``                  | Initialize the ADC driver + run the  |
|                                         | online CWT calibration               |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_driver_deinit``                | De-initialize the ADC driver         |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_channel_init``                 | Configure and initialize the given   |
|                                         | channel in one call via              |
|                                         | ``adc_config_t``                     |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_channel_deinit``               | De-initialize the given channel      |
+-----------------------------------------+--------------------------------------+

Data Acquisition
----------------

+-----------------------------------------+----------------------------------------------+
| API                                     | Description                                  |
+=========================================+==============================================+
| ``bk_adc_channel_raw_read``             | Synchronously capture N raw samples          |
|                                         | (recommended; counted in uint16_t entries)   |
+-----------------------------------------+----------------------------------------------+
| ``bk_adc_channel_read``                 | Synchronously capture N samples and return   |
|                                         | the average (recommended)                    |
+-----------------------------------------+----------------------------------------------+
| ``bk_adc_single_read``                  | Single-step mode: read one sample            |
+-----------------------------------------+----------------------------------------------+
| ``bk_adc_cont_start``                   | Start continuous-mode sampling               |
+-----------------------------------------+----------------------------------------------+
| ``bk_adc_cont_get_raw``                 | Continuous mode: fetch a batch of raw data   |
+-----------------------------------------+----------------------------------------------+
| ``bk_adc_cont_mode_cont_read``          | Continuous mode: rolling read                |
+-----------------------------------------+----------------------------------------------+

Interrupt & Calibration
-----------------------

+-----------------------------------------+--------------------------------------+
| API                                     | Description                          |
+=========================================+======================================+
| ``bk_adc_register_isr_callback``        | Register the ADC interrupt callback  |
|                                         | (for application use)                |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_enter_calib_mode``             | Enter the CWT online calibration     |
|                                         | mode                                 |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_enable_bypass_clalibration``   | Output uncalibrated raw data         |
|                                         | (debugging)                          |
+-----------------------------------------+--------------------------------------+
| ``bk_adc_data_calculate``               | Convert an ADC code to a voltage (V) |
+-----------------------------------------+--------------------------------------+

.. note::

    All clock, mode, sample-rate, steady-time, and filter parameters are passed in
    once via ``adc_config_t`` to ``bk_adc_channel_init()``; you **do not** need to
    invoke ``bk_adc_set_clk()`` / ``bk_adc_set_mode()`` and similar standalone
    setters separately — those exist only in the old-chip driver (V1P1) and are
    not linked into the BK7236N binary.

    The headers also expose ``bk_adc_init`` / ``bk_adc_deinit`` /
    ``bk_adc_acquire`` / ``bk_adc_release`` / ``bk_adc_start`` / ``bk_adc_stop`` /
    ``bk_adc_set_config`` / ``bk_adc_set_channel`` / ``bk_adc_read`` /
    ``bk_adc_read_raw`` and similar APIs; these are legacy stubs retained for
    BK7236 compatibility (implemented in ``adc_legacy_api.c``) and must not be
    used in new code.

    In addition, ``bk_adc_get_raw(chan, buf, buf_size_in_bytes, timeout)`` is a
    V1P1-era low-level helper (its size argument is in **bytes**) that the
    application-facing wrapper ``bk_adc_channel_raw_read(chan, buf, sample_cnt,
    timeout)`` (size in **uint16_t entries**, more intuitive) calls internally.
    New code must use ``bk_adc_channel_raw_read``; do not call ``bk_adc_get_raw``
    directly.

Caveats
=======

1. **Driver init is mandatory before use**: ``bk_adc_driver_init()`` loads the
   CWT calibration coefficients (see :ref:`saradc-calibration`); skipping it makes
   every subsequent sample diverge from the actual voltage.

2. **Only one channel can be active at a time**: ``adc_chan`` is 4 bits in
   ``reg4``. The driver already guards ``bk_adc_channel_raw_read()`` /
   ``bk_adc_channel_read()`` with its own mutex, so concurrent reads from
   different channels in different tasks are safe; no extra locking is needed at
   the application level.

3. **Do not enable other GPIO functions on an ADC pin**: while sampling, the
   driver puts the corresponding GPIO into a non-driving plain-GPIO state. The
   application must not enable pull-up / pull-down / input / output on that GPIO
   during sampling.

4. **Source of the calibration data**: BK7236N has its CWT coefficients programmed
   into ``OTP_GADC_CALIBRATION`` at CP/FT in production; the driver loads OTP
   first at init time, and only falls back to LMS online calibration when OTP is
   invalid (blank chip).

5. **Clock source is fixed to XTAL 40 MHz**: the ADC clock source cannot be
   switched on BK7236N. The application only needs to fill ``adc_config_t.clk``
   with the desired ``adc_clk`` frequency (1.33 MHz ~ 20 MHz);
   ``adc_config_t.src_clk`` must be ``ADC_SCLK_XTAL``.
