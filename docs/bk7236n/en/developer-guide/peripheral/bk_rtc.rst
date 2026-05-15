===
RTC
===

:link_to_translation:`zh_CN:[中文]`

Overview
========

BK7236N integrates a single **AON RTC** (Always-On Real-Time Counter) located in the
AON (Always-On) power domain. The RTC is clocked from a 32 kHz low-frequency source
and keeps running across Low-Voltage and Deep-Sleep modes, making it the canonical
timekeeping and wake-up source for low-power scenarios.

Key features:

 - **36-bit free-running counter** exposed to software through ``aon_pmu``
   ``r78.rtc_tick_h`` (4 bit) + ``r79.rtc_tick_l`` (32 bit); rolls over at
   ``0xFFFFFFFFF``.
 - **Selectable 32 kHz clock source**:

   - On-chip ROSC 32 kHz by default (~32 000 Hz nominal; the actual frequency is
     returned by ``bk_rosc_32k_get_freq()`` after CKMN calibration).
   - External 32.768 kHz crystal (XTAL) when ``CONFIG_EXTERN_32K`` is enabled at
     build time.

 - **Always-on operation**: AON-domain 32 kHz clock and counter are not powered down
   in Low-Voltage / Deep-Sleep, so the count value remains continuous across wake-up.
 - **Application-level alarm framework**: the driver maintains a singly-linked alarm
   list sorted by expiration tick, supporting up to ``AON_RTC_MAX_ALARM_CNT = 8``
   alarms; each alarm has a configurable period (in ticks), repeat count, callback,
   and a name of at most ``ALARM_NAME_MAX_LEN = 7`` bytes.
 - **Time-of-day support**: ``bk_rtc_settimeofday()`` / ``bk_rtc_gettimeofday()``
   provide POSIX-style wall-clock interfaces. When
   ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1``, the boot-time offset ``s_boot_time_us``
   is persisted in EasyFlash so the wall clock survives power-down.
 - **Deep-Sleep duration accounting**: the current RTC microsecond reading is saved
   to EasyFlash before entering Deep-Sleep; on wake-up the difference between the
   current RTC reading and the saved value gives the sleep duration, exposed via
   ``bk_rtc_get_deepsleep_duration_seconds()``.

Hardware Architecture
=====================

Counter and Sampling
--------------------

The RTC body consists of two parts (see ``middleware/soc/bk7236n/soc/aon_pmu_struct.h``):

.. list-table::
    :header-rows: 1
    :widths: 25 12 8 55

    * - Register
      - Offset
      - Width
      - Description
    * - ``r79.rtc_tick_l``
      - 0x1E4
      - 32
      - Low 32 bits of the 32 kHz tick counter.
    * - ``r78.rtc_tick_h``
      - 0x1E0
      - 4
      - High 4 bits of the tick counter (concatenated with the low 32 bits to
        form a 36-bit count).
    * - ``r2.rtc_value_samp_sel``
      - 0x008 bit[8]
      - 1
      - Cross-clock-domain sampling edge for the RTC tick: 1 = rising edge,
        0 = falling edge.

The driver hard-codes falling-edge sampling in ``ana_rtc_hw_init()``
(``aon_pmu_hal_rtc_samp_sel(ANA_RTC_SAMPLE_NEGATIVE_EDGE)``);
``bk_aon_rtc_get_current_tick()`` uses ``BK_ASSERT`` to verify the configuration is
not overwritten.

Compare / Wake-up Registers
---------------------------

There is no dedicated compare register inside the RTC block. Instead the system
analog control space (``ana_reg``) provides two 36-bit compare channels with
different semantics:

.. list-table::
    :header-rows: 1
    :widths: 30 18 52

    * - Channel (concatenated 36 bit)
      - Bits
      - Purpose
    * - ``ana_reg11.rtcsel``
      - [27:24]
      - High 4 bits of the alarm compare value
    * - ``ana_reg13.rtcsel``
      - [31:0]
      - Low 32 bits of the alarm compare value
    * - ``ana_reg11.timersel``
      - [31:28]
      - High 4 bits of the RTC counter top value
    * - ``ana_reg12.timersel``
      - [31:0]
      - Low 32 bits of the RTC counter top value

The two channels behave differently in hardware:

 - ``rtcsel`` (**alarm channel**): when the RTC counter reaches the value
   programmed into ``rtcsel`` it raises ``INT_SRC_ANA_RTC`` **but does not
   reset the counter** — the counter keeps incrementing. This is the standard
   alarm / wake-up path on BK7236N, gated by ``ana_reg8.rtcwk_rstn``. The
   driver (``ana_rtc_set_tick``) simply writes the next alarm's
   ``expired_tick`` into these registers.
 - ``timersel`` (**wrap channel**): when the RTC counter reaches the value
   programmed into ``timersel``, the hardware **clears the RTC counter back to
   0 and restarts counting from 0**. BK7236N does not use this channel to
   generate alarm interrupts; instead the driver uses it as the RTC **wrap
   limit**: ``ana_rtc_hw_init`` unconditionally writes ``timersel`` to
   ``AON_RTC_ROUND_TICK = 0xFFFFFFFFF`` (the natural 36-bit max), so the wrap
   matches the natural 36-bit counter boundary. The driver software extends
   this to 64 bits via ``s_high_tick`` (see :ref:`bk7236n_rtc_64bit_view`),
   producing a continuous 64-bit tick to upper layers.

The default on BK7236N is ``CONFIG_ANA_RTC_WAKUP_BY_RTC = 1``; the API / ISR
descriptions in this document assume that configuration.

Interrupt and Clock-Source ana_reg Fields
-----------------------------------------

Other ``ana_reg`` fields touched by ``ana_rtc_hw_init()``:

.. list-table::
    :header-rows: 1
    :widths: 32 68

    * - Register field
      - Function
    * - ``ana_reg5.pwd_rosc_spi``
      - Power-down enable for the ROSC 32K analog block. The driver writes 0 when
        powering up the RTC to release the power-down.
    * - ``ana_reg5.rosc_disable``
      - ROSC oscillator disable. The driver writes 0 when powering up the RTC to
        release the oscillator.
    * - ``ana_reg7.clk_sel``
      - 32 kHz source: ``1`` = external 32.768 kHz XTAL, ``0`` = internal ROSC.
        Driven by ``CONFIG_EXTERN_32K``.
    * - ``ana_reg8.spi_latch1v``
      - SPI write latch. The driver toggles this bit to 1 / 0 around any
        ``ana_reg`` write to bracket the critical section.
    * - ``ana_reg9.spi_timerwken``
      - RTC counter enable / clear: writing 0 clears the RTC counter and stops
        it; writing 1 starts the counter from 0. With BK7236N's default
        ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT = 1``, ``sys_hal_init`` sets this
        bit to 1 early during system init and ``ana_rtc_hw_init`` no longer
        touches it, so the RTC count value is preserved across driver
        re-initialization.
    * - ``ana_reg9.spi_byp32pwd``
      - 32 kHz clock bypass / power-down (bypass).

For interrupts, the driver registers an ISR for ``INT_SRC_ANA_RTC``
(NVIC entry 59 in ``icu_map.h``). Interrupts are enabled / disabled via
``sys_hal_enable_ana_rtc_int()`` / ``sys_hal_disable_ana_rtc_int()``, which besides
toggling the group2 interrupt also touch ``ana_reg8.rtcwk_rstn`` and
``ana_reg8.rst_timerwks1v`` (status clear).

Software Architecture
=====================

Driver File Path
----------------

The BK7236N RTC driver source file is
``middleware/driver/rtc/ana_rtc_driver.c``.

The public header ``include/driver/aon_rtc.h`` and the type definitions in
``include/driver/aon_rtc_types.h`` keep the same namespace as the legacy
chip's interface, so the public API on BK7236N is still
``bk_aon_rtc_*`` / ``bk_alarm_*`` / ``bk_rtc_*``.

Layering
--------

.. list-table::
    :header-rows: 1
    :widths: 14 28 58

    * - Layer
      - Main files
      - Responsibility
    * - **Driver**
      - ``ana_rtc_driver.c``
      - Maintains the alarm linked list and node pool, registers the
        ``INT_SRC_ANA_RTC`` ISR, and exposes APIs such as
        ``bk_alarm_register`` / ``bk_aon_rtc_get_us`` / ``bk_rtc_settimeofday``.
    * - **HAL (sys / pmu)**
      - ``sys_hal.c`` , ``aon_pmu_hal.c``
      - Wraps RTC tick reads, sampling-edge configuration, ana_reg field access,
        and interrupt enable / disable.
    * - **LL (registers)**
      - ``sys_ll.h`` , ``aon_pmu_ll.h``
      - Direct bit-field accessors such as ``aon_pmu_ll_get_r79_rtc_tick_l`` /
        ``sys_ll_set_ana_reg11_rtcsel``.

Driver State
------------

``ana_rtc_driver`` keeps a global state struct ``s_ana_rtc`` (``inited`` flag,
alarm list head ``alarm_head_p``, and node count ``alarm_node_cnt``), plus a
separate node pool ``s_ana_rtc_nodes_p`` allocated by ``os_zalloc`` inside
``bk_aon_rtc_driver_init()`` containing ``AON_RTC_MAX_ALARM_CNT`` entries of
``alarm_node_t`` and a ``busy_bits`` bitmap.

Clock and Precision
===================

Clock Source and Frequency
--------------------------

The RTC 32 kHz clock source is decided at build time by ``CONFIG_EXTERN_32K``:

 - **Undefined** (default on BK7236N): on-chip ROSC 32K, nominally 32 000 Hz; the
   actual frequency is dynamically corrected by CKMN calibration and read at
   runtime via ``bk_rosc_32k_get_freq()`` (``middleware/driver/pwr_clk/rosc_32k.c``).
 - **Defined**: external 32.768 kHz XTAL, with frequency given by the constant
   ``AON_RTC_EXTERN_32K_CLOCK_FREQ = 32768``.

Regardless of the selected source, the driver uses ``bk_rtc_get_clock_freq()`` to
return the current frequency as a ``uint32_t``; all upper-layer tick / us conversions
build on this.

Tick / Time Conversion
----------------------

The driver provides the following pure-software conversion helpers
(implementation in ``ana_rtc_driver.c``):

.. list-table::
    :header-rows: 1
    :widths: 45 55

    * - API
      - Equivalent expression
    * - ``bk_rtc_get_ms_tick_count()``
      - ``bk_rtc_get_clock_freq() / 1000``
    * - ``bk_aon_rtc_tick_to_us(tick)``
      - ``tick * 1000000 / bk_rtc_get_clock_freq()``
    * - ``bk_aon_rtc_us_to_tick(us)``
      - ``us * bk_rtc_get_clock_freq() / 1000000``

Therefore:

 - **Time resolution** equals one 32 kHz period — about **30.5 us**
   (XTAL 32 768 Hz) or **31.25 us** (ROSC 32 000 Hz).
 - **Absolute accuracy**: with XTAL 32K it is determined by the crystal (typically
   a few tens of ppm; check the chosen crystal spec); with ROSC 32K, after CKMN
   calibration the residual error can be read with ``bk_rosc_32k_get_ppm()`` and
   is **not** persisted across power-down — recalibration is required after each
   reset.
 - **Minimum alarm period**: ``bk_alarm_register()`` enforces
   ``period_tick >= AON_RTC_PRECISION_TICK = 32`` and otherwise returns
   ``BK_FAIL``, i.e. the minimum period is roughly **1 ms**.

.. note::

    When the application thinks in milliseconds / microseconds, **never multiply or
    divide by the literal constants 32 / 32768 / 32000**. Always use
    ``AON_RTC_MS_TICK_CNT`` (i.e. ``bk_rtc_get_ms_tick_count()``) or
    ``bk_aon_rtc_us_to_tick()``; otherwise drift caused by ROSC calibration or by a
    build-time switch to ``CONFIG_EXTERN_32K`` will make timings ~2.4% long or short.

.. _bk7236n_rtc_64bit_view:

64-bit tick View
----------------

With ``CONFIG_ANA_RTC_SUPPORT_64BIT = 1`` (default on BK7236N), ``rtc_tick_t`` is
``uint64_t`` and ``bk_aon_rtc_get_current_tick()`` returns a 64-bit tick. The
construction is:

 - The lower 36 bits come from a direct read of ``r79`` + ``r78``;
 - The upper bits are software-tracked: every time the low 32 bits decrease (i.e. a
   wrap), the software-side high counter ``s_high_tick`` is incremented by 1.

As long as the application periodically calls ``bk_aon_rtc_get_us()`` /
``bk_aon_rtc_get_current_tick()`` (or has at least one alarm guaranteed to fire
within the 36-bit period), wraps are not lost.

Operation
=========

Alarm Registration and Ordering
-------------------------------

When the application calls ``bk_alarm_register(id, &alarm_info)``, the driver
performs the following (see ``middleware/driver/rtc/ana_rtc_driver.c``):

1. Disable interrupts → claim a free slot from the node pool (up to
   ``AON_RTC_MAX_ALARM_CNT = 8``);
2. Copy ``name`` / ``period_tick`` / ``period_cnt`` / ``callback`` / ``param_p``;
3. Compute ``expired_tick = bk_aon_rtc_get_current_tick(id) + period_tick``;
4. Reject duplicate-name alarms (compared per ``ALARM_NAME_MAX_LEN`` bytes), then
   insert into the list in ascending ``expired_tick`` order;
5. If the new node becomes the new list head, immediately program its
   ``expired_tick`` into the hardware compare register (``ana_rtc_set_tick``).

ana_rtc_set_tick Synchronization
--------------------------------

A write to ``ana_reg11`` / ``13`` (``rtcsel``) takes ~3 32 kHz cycles to cross the
clock-domain barrier; the driver loops reading back until the read value matches the
written value. The timeout threshold is ``ANA_RTC_OPS_SAFE_DELAY_US = 125``; on
timeout the driver prints ``set tick timeout``.

ISR Handling
------------

``ana_rtc_isr_handler()`` is invoked when ``INT_SRC_ANA_RTC`` fires:

1. Disable the RTC interrupt (``sys_hal_disable_ana_rtc_int``) and the compare
   hardware;
2. Walk the alarm list starting from ``alarm_head_p``:

   - If ``expired_tick <= cur_tick + AON_RTC_PRECISION_TICK``, invoke its callback;
   - One-shot alarm (``period_cnt == 1``): remove from the list and return its node
     to the pool;
   - Periodic alarm (``period_cnt > 1`` or ``ALARM_LOOP_FOREVER``):
     ``expired_tick += period_tick`` and re-insert at the correct position;

3. Program the new list head's ``expired_tick`` into the compare register; if the
   list is empty, write ``AON_RTC_ROUND_TICK + 1``;
4. Re-enable the RTC interrupt (``sys_hal_enable_ana_rtc_int``, called at the end of
   ``ana_rtc_set_tick``).

.. warning::

    Alarm callbacks run in **ISR context**; they must respect bare-interrupt rules:

     - Do not call any blocking RTOS API (``sleep`` / ``wait`` / mutex etc.);
     - Do not call ``free`` / ``malloc`` from inside the callback (FreeRTOS forbids
       heap allocation / free in ISR context);
     - Do not unregister the calling alarm itself; if you want a "fire-and-forget"
       behaviour, set ``period_cnt`` to 1 and the driver will return the node to
       the pool inside the ISR automatically.

Low-Power Management
====================

Always-On Operation
-------------------

The RTC counter, the 32 kHz clock, and the compare registers all live in the AON
domain. **In Low-Voltage and Deep-Sleep modes the RTC counter keeps running, the
count value stays continuous, and the wake-up source remains functional.** This is
the very reason the RTC exists on BK7236N — to provide accurate timing and wake-up
capability for low-power modes.

Wake-up Source Wiring
---------------------

The standard procedure to wake the system at a given time (see
:doc:`../power_save/bk_rtc_sleep`):

1. Register a one-shot alarm with ``bk_alarm_register(AON_RTC_ID_1, &alarm)``
   (``period_cnt = 1``); ``period_tick`` determines the sleep duration.
2. Tell PM to use the RTC as a wake-up source via
   ``bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL)``.
3. Enter low power: ``bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE)`` or
   ``PM_MODE_DEEP_SLEEP``.

Before each compare-register write the driver calls ``ana_rtc_wakeup_config()`` to
verify the next expiration is at least ``AON_RTC_MS_TICK_CNT`` (~1 ms) away;
otherwise it returns ``BK_FAIL`` to avoid missing the wake-up.

Deep-Sleep Time Persistence
---------------------------

When ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT = 1`` the driver registers a pair of
callbacks via ``bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_RTC, ...)``:

 - **Entering Deep-Sleep**\ : ``bk_rtc_keep_time_enter_deepsleep()`` writes the
   current ``bk_aon_rtc_get_us()`` reading to the EasyFlash environment variable
   ``rtc_ds_start_us``;
 - **After wake-up**\ : ``aon_rtc_compute_boot_timeofday()`` is invoked from
   ``bk_aon_rtc_driver_init()``. It uses ``bk_misc_get_reset_reason()`` to
   determine whether the boot followed a Deep-Sleep wake-up; if so it subtracts
   the saved ``rtc_ds_start_us`` from the current ``bk_aon_rtc_get_us()`` to obtain
   the sleep duration, stored in ``s_rtc_keep_time_offset_us``, and restores the
   wall-clock offset from ``s_boot_time_us``.

Applications consume the result via:

 - ``bk_rtc_get_deepsleep_duration_seconds(uint32_t *)`` — duration of the previous
   Deep-Sleep in seconds;
 - ``bk_rtc_gettimeofday()`` — wall-clock time computed as
   ``s_boot_time_us + bk_aon_rtc_get_us()``.

.. note::

    Deep-Sleep persistence requires ``CONFIG_EASY_FLASH = 1`` and a working
    EasyFlash (``bk_set_env`` / ``bk_save_env`` available); with
    ``CONFIG_EASY_FLASH = 0`` the persistence path returns ``BK_FAIL`` and the
    wall-clock value is lost across power-down.

API Usage Examples
==================

``alarm_info_t`` Field Reference
--------------------------------

.. note::

    On BK7236N every field of ``alarm_info_t`` is effective; there are no
    "kept-for-legacy-chips" placeholders:

    .. list-table::
        :header-rows: 1
        :widths: 20 80

        * - Field
          - Description
        * - ``name``
          - Alarm name; up to ``ALARM_NAME_MAX_LEN = 7`` bytes (excess is
            truncated). Re-registering an alarm with the same name is rejected.
        * - ``period_tick``
          - Period in 32 kHz ticks. Minimum value is
            ``AON_RTC_PRECISION_TICK = 32`` (~1 ms). To convert from milliseconds
            use ``period_tick = ms * AON_RTC_MS_TICK_CNT``.
        * - ``period_cnt``
          - Repeat count. ``1`` = one-shot, removed from the list automatically
            after firing once; ``2`` ~ ``0xFFFFFFFE`` = fire N times then stop;
            ``ALARM_LOOP_FOREVER (0xFFFFFFFF)`` = infinite, must call
            ``bk_alarm_unregister`` explicitly to stop.
        * - ``callback``
          - Function pointer invoked at expiration. Signature is
            ``void cb(aon_rtc_id_t, uint8_t *name, void *param)``;
            runs in **ISR context**.
        * - ``param_p``
          - Opaque ``void *`` user parameter passed through to the callback.

Periodic Alarm Example
----------------------

The following shows the typical sequence to register a 1-second periodic, infinitely
looping alarm on BK7236N (cf. ``middleware/driver/rtc/aon_rtc_test.c``):

.. code-block:: c

    #include <driver/aon_rtc.h>

    static void on_alarm(aon_rtc_id_t id, uint8_t *name, void *param)
    {
        /* ISR context: keep this function short, no malloc / no blocking calls */
        (void)id; (void)name; (void)param;
        /* e.g. set a flag, post a task notification ... */
    }

    void rtc_periodic_example(void)
    {
        /* 1. Driver init (also initializes HW counter / interrupt) */
        bk_aon_rtc_driver_init();

        /* 2. Build alarm descriptor */
        alarm_info_t alarm = {
            .name        = "tick_1s",                          /* <= 7 bytes */
            .period_tick = (rtc_tick_t)(1000 * AON_RTC_MS_TICK_CNT),
            .period_cnt  = ALARM_LOOP_FOREVER,
            .callback    = on_alarm,
            .param_p     = NULL,
        };

        /* 3. Register; takes effect immediately */
        bk_alarm_register(AON_RTC_ID_1, &alarm);

        /* ... later, when no longer needed ... */
        bk_alarm_unregister(AON_RTC_ID_1, (uint8_t *)"tick_1s");
    }

One-shot Wake-up (Low-Voltage / Deep-Sleep)
-------------------------------------------

Setting ``period_cnt`` to ``1`` causes the alarm to vanish automatically after a
single fire — the canonical "sleep N seconds and wake up" pattern:

.. code-block:: c

    static void wake_cb(aon_rtc_id_t id, uint8_t *name, void *param) { /* ... */ }

    void rtc_oneshot_wakeup(uint32_t sleep_ms)
    {
        alarm_info_t a = {
            .name        = "wake",
            .period_tick = (rtc_tick_t)(sleep_ms * AON_RTC_MS_TICK_CNT),
            .period_cnt  = 1,
            .callback    = wake_cb,
            .param_p     = NULL,
        };
        bk_alarm_register(AON_RTC_ID_1, &a);

        /* Tell PM to use RTC as a wake-up source */
        bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
        bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);   /* or PM_MODE_DEEP_SLEEP */
    }

Reading the Current Time
------------------------

.. code-block:: c

    /* Raw 64-bit tick (32 kHz units, range up to 36 bits hardware + software extension) */
    uint64_t tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);

    /* Free-running microseconds since power-on (or since clock-source switch) */
    uint64_t us = bk_aon_rtc_get_us();
    uint64_t ms = bk_aon_rtc_get_ms();

Setting / Reading the Wall-clock Time
-------------------------------------

Note that ``bk_rtc_settimeofday()`` actually sets ``s_boot_time_us``, i.e. "the
wall-clock time corresponding to the current RTC microsecond reading"; thereafter
each ``bk_rtc_gettimeofday()`` returns
``s_boot_time_us + bk_aon_rtc_get_us()``.

.. code-block:: c

    #include <sys/time.h>
    #include <driver/aon_rtc.h>

    void wall_clock_demo(void)
    {
        /* Set wall-clock to a specific epoch (e.g. from NTP) */
        struct timeval tv = {
            .tv_sec  = 1714190400,   /* example: 2024-04-27 00:00:00 UTC */
            .tv_usec = 0,
        };
        bk_rtc_settimeofday(&tv, NULL);   /* persisted when CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1 */

        /* Read it back later */
        struct timeval now;
        bk_rtc_gettimeofday(&now, NULL);
    }

Reading the Previous Deep-Sleep Duration
----------------------------------------

.. code-block:: c

    uint32_t deep_sleep_sec = 0;
    if (bk_rtc_get_deepsleep_duration_seconds(&deep_sleep_sec) == BK_OK) {
        /* deep_sleep_sec is valid only when this boot followed a Deep-Sleep wake-up */
    }

Key API Reference
=================

The tables below list only the RTC APIs **actually available** on BK7236N. All
declarations live in ``include/driver/aon_rtc.h``; the implementations live in
``middleware/driver/rtc/ana_rtc_driver.c``.

Driver Initialization
---------------------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - Description
    * - ``bk_aon_rtc_driver_init``
      - Initializes software state, allocates the node pool, configures the 32 kHz
        clock and compare registers, and registers the ``INT_SRC_ANA_RTC`` ISR.
        With ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1`` it also registers the
        Deep-Sleep keep-time callbacks. **Must be called before any other RTC API.**
    * - ``bk_aon_rtc_driver_deinit``
      - Deinitializes the driver, powers down ROSC, releases the node pool, and
        unregisters the ISR.

Alarm Management
----------------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - Description
    * - ``bk_alarm_register``
      - Registers an alarm (parameters described in the ``alarm_info_t`` field
        reference table).
    * - ``bk_alarm_unregister``
      - Removes an alarm by name.

Time Reading and Conversion
---------------------------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - Description
    * - ``bk_aon_rtc_get_current_tick``
      - Returns the current 64-bit tick (with software-tracked high-bit wrap
        accumulation).
    * - ``bk_aon_rtc_get_us``
      - Returns the current accumulated microsecond count (based on
        ``s_time_base_us`` accumulation).
    * - ``bk_aon_rtc_get_ms``
      - ``bk_aon_rtc_get_us() / 1000``.
    * - ``bk_aon_rtc_tick_to_us``
      - tick → us conversion.
    * - ``bk_aon_rtc_us_to_tick``
      - us → tick conversion.
    * - ``bk_rtc_get_clock_freq``
      - Returns the current measured 32 kHz frequency (XTAL 32768 or ROSC
        calibrated value).
    * - ``bk_rtc_get_ms_tick_count``
      - ``bk_rtc_get_clock_freq() / 1000``; backs the macro
        ``AON_RTC_MS_TICK_CNT``.
    * - ``bk_rtc_set_clock_freq``
      - Notifies the driver of a clock-frequency change (internally only
        re-baselines time by calling ``bk_aon_rtc_get_us`` once; does not write
        any hardware register).

Wall-clock Time
---------------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - Description
    * - ``bk_rtc_settimeofday``
      - Sets the wall-clock time (only updates ``s_boot_time_us``; persisted to
        EasyFlash when ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1``).
    * - ``bk_rtc_gettimeofday``
      - Reads the wall-clock time.
    * - ``bk_rtc_get_deepsleep_duration_seconds``
      - Reads the duration in seconds of the previous Deep-Sleep.

Debug
-----

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - Description
    * - ``bk_aon_rtc_dump``
      - Prints the current alarm list (with ``CONFIG_ANA_RTC_DEBUG`` it also
        prints recorded ISR timing).

.. note::

    The following APIs visible in ``include/driver/aon_rtc.h`` are **not compiled
    into the BK7236N binary**; they are kept only as legacy declarations for
    older chips. Do not call them on BK7236N — the linker will fail to resolve
    the symbols:

     - ``bk_aon_rtc_create`` / ``bk_aon_rtc_destroy`` / ``bk_aon_rtc_tick_init`` /
       ``bk_aon_rtc_open_rtc_wakeup`` / ``bk_aon_rtc_register_upper_isr``
     - ``bk_rtc_ana_register_wakeup_source``: under
       ``#if CONFIG_RTC_ANA_WAKEUP_SUPPORT``, not enabled on BK7236N by default.
     - ``bk_aon_rtc_register_tick_isr``: on BK7236N the function body merely stores
       the callback pointer into ``s_ana_rtc_tick_isr``; the driver ISR only invokes
       callbacks from the alarm list, so **this callback is never invoked**. Use
       ``bk_alarm_register`` for periodic / one-shot callbacks; do not rely on this
       interface.

CLI Commands
============

When ``CONFIG_AON_RTC_TEST = 1`` (default on BK7236N), ``bk_aon_rtc_driver_init()``
automatically registers a set of CLI commands (implementation in
``middleware/driver/rtc/aon_rtc_test.c``):

.. list-table::
    :header-rows: 1
    :widths: 38 62

    * - Command
      - Purpose
    * - ``aon_rtc_driver {init|deinit}``
      - Manually init / deinit the RTC driver
    * - ``aon_rtc_get_time {id}``
      - Print the current tick value (already converted to ms)
    * - ``aon_rtc_register {id} {name} {period_ms} {period_cnt}``
      - Register an alarm whose callback prints ``id, name``. The CLI internally
        converts ``period_ms * AON_RTC_MS_TICK_CNT`` into ticks before writing.
    * - ``aon_rtc_unregister {id} {name}``
      - Unregister an alarm by name
    * - ``aon_rtc_auto_test {id} {start|stop}``
      - Start / stop a built-in self-test that registers 6 alarms recursively
    * - ``aon_rtc_time_of_day {get|set} [sec usec]``
      - Read / set the wall-clock time. The ``get_deepsleep_time`` sub-command
        prints the value returned by
        ``bk_rtc_get_deepsleep_duration_seconds()``.
    * - ``aon_rtc_dump {id}``
      - Print the alarm list

.. note::

    The ``aon_rtc_clock_src {26m|rosc}`` command actually controls the LPO
    (low-frequency oscillator) source via ``aon_pmu_drv_lpo_src_set``. It is
    **not** the same thing as the "RTC 32 kHz clock source (XTAL/ROSC)" discussed
    in this document — do not confuse them. On BK7236N the RTC clock source is
    selected at build time by ``CONFIG_EXTERN_32K`` and cannot be changed at
    runtime.

Caveats
=======

1. **Always call bk_aon_rtc_driver_init first**\ : ``bk_alarm_register`` /
   ``bk_aon_rtc_get_us`` / ``bk_rtc_settimeofday`` and others depend on internal
   driver state (``s_ana_rtc.inited``, the node pool, the registered ISR). Skipping
   init causes either a non-responding ISR or a NULL-pointer access.

2. **Alarm callbacks run in ISR context**\ : no blocking, no ``malloc`` / ``free``,
   no self-unregister. For deferred work, set a flag or post a task notification
   inside the callback.

3. **Alarm name is at most 7 bytes**\ : ``ALARM_NAME_MAX_LEN = 7``; excess bytes
   are truncated. Registering an alarm with a duplicate name fails with
   ``BK_FAIL``.

4. **Minimum period ~1 ms**\ : ``period_tick < AON_RTC_PRECISION_TICK (32)`` is
   rejected by the driver outright; ISR-path overhead also limits sub-ms stability.

5. **Single hardware instance**\ : ``SOC_AON_RTC_UNIT_NUM = 1``. ``aon_rtc_id_t``
   only takes ``AON_RTC_ID_1``; any other value is rejected by the driver with
   ``BK_ERR_PARAM``.

6. **Clock source decided at build time**\ : ``CONFIG_EXTERN_32K`` selects XTAL or
   ROSC; this cannot be changed at runtime. If you need long-term timing accuracy
   on the ROSC source, make sure CKMN calibration (``bk_rosc_32k_calib``) is
   enabled as needed.

7. **Deep-Sleep keep-time depends on EasyFlash**\ : with
   ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT = 1`` the persistence path requires
   ``CONFIG_EASY_FLASH = 1`` to succeed. Otherwise
   ``bk_rtc_get_deepsleep_duration_seconds`` still works (it reads the RAM-side
   ``s_rtc_keep_time_offset_us``), but the wall-clock value is lost across
   power-down.
