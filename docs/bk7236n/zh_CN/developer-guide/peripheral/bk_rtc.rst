===
RTC
===

:link_to_translation:`en:[English]`

概述
======

BK7236N 集成一颗 **AON RTC**\ （Always-On Real-Time Counter，常开域实时计数器），位于 AON（Always-On）电源域，时钟源采用 32 kHz 低频时钟，可在系统进入 Low-Voltage 甚至 Deep-Sleep 模式时持续运行，是低功耗场景下的核心计时与唤醒源。

主要特性：

 - **36 bit 自由运行计数器**\ ：通过 ``aon_pmu`` 的 ``r78.rtc_tick_h`` (4 bit) +
   ``r79.rtc_tick_l`` (32 bit) 暴露给软件，绕回值为 ``0xFFFFFFFFF`` 。
 - **可选 32 kHz 时钟源**\ ：

   - 默认使用片上 ROSC 32 kHz（约 32 000 Hz，校准后由 ``bk_rosc_32k_get_freq()`` 返回实测值）；
   - 编译时打开 ``CONFIG_EXTERN_32K`` 后改用外部 32.768 kHz 晶振 (XTAL)。

 - **常驻运行**\ ：AON 域 32 kHz 时钟和计数器在 Low-Voltage / Deep-Sleep 模式下不掉电，计数值在唤醒后保持连续。
 - **应用层 alarm 框架**\ ：驱动维护一条按到期时刻升序排列的 alarm 链表，最多支持 ``AON_RTC_MAX_ALARM_CNT = 8`` 条；每条 alarm 可配置周期 tick、循环次数、回调函数和不超过 ``ALARM_NAME_MAX_LEN = 7`` 字节的名称。
 - **time-of-day 支持**\ ：基于 ``bk_rtc_settimeofday()`` / ``bk_rtc_gettimeofday()`` 提供 POSIX 风格的墙钟时间接口；当 ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1`` 时，启动时刻偏移 ``s_boot_time_us`` 会通过 EasyFlash 持久化，掉电后能保持系统时钟连续。
 - **Deep-Sleep 持续时间统计**\ ：进入 Deep-Sleep 前把当时的 RTC us 写入 EasyFlash，唤醒后用当前 RTC us 与之相减得到睡眠时长，由 ``bk_rtc_get_deepsleep_duration_seconds()`` 输出。

硬件架构
==========

计数器与采样
-------------

RTC 主体由两部分组成 (见 ``middleware/soc/bk7236n/soc/aon_pmu_struct.h``)：

.. list-table::
    :header-rows: 1
    :widths: 25 12 8 55

    * - 寄存器
      - 偏移
      - 位宽
      - 说明
    * - ``r79.rtc_tick_l``
      - 0x1E4
      - 32
      - 32 kHz tick 计数低 32 位。
    * - ``r78.rtc_tick_h``
      - 0x1E0
      - 4
      - tick 计数高 4 位 (与低 32 位拼接成 36 bit 计数)。
    * - ``r2.rtc_value_samp_sel``
      - 0x008 bit[8]
      - 1
      - RTC tick 跨时钟域采样边沿选择：1 = 上升沿，0 = 下降沿。

驱动在 ``ana_rtc_hw_init()`` 中固定写入下降沿采样 (``aon_pmu_hal_rtc_samp_sel(ANA_RTC_SAMPLE_NEGATIVE_EDGE)``)； ``bk_aon_rtc_get_current_tick()`` 用 ``BK_ASSERT`` 校验该配置不会被改写。

比较 / 唤醒寄存器
------------------

RTC 没有专用的 compare 寄存器，而是借用系统模拟控制 (``ana_reg``) 中的两套 36 bit 比较通道，二者职责不同：

.. list-table::
    :header-rows: 1
    :widths: 30 18 52

    * - 通道 (拼接 36 bit)
      - 位段
      - 用途
    * - ``ana_reg11.rtcsel``
      - [27:24]
      - alarm 比较值高 4 bit
    * - ``ana_reg13.rtcsel``
      - [31:0]
      - alarm 比较值低 32 bit
    * - ``ana_reg11.timersel``
      - [31:28]
      - RTC 计数最大值高 4 bit
    * - ``ana_reg12.timersel``
      - [31:0]
      - RTC 计数最大值低 32 bit

两条通道的硬件行为差异：

 - ``rtcsel`` (\ **alarm 通道**\ )：当 RTC 计数值到达 ``rtcsel`` 设定值时产生 ``INT_SRC_ANA_RTC`` 中断，\ **RTC 计数值不清零，继续递增**\ 。这是 BK7236N 上 alarm / 唤醒功能的标准路径，由 ``ana_reg8.rtcwk_rstn`` 使能。驱动 (``ana_rtc_set_tick``) 把下一条 alarm 的 ``expired_tick`` 写入这两个寄存器即可。
 - ``timersel`` (\ **wrap 通道**\ )：当 RTC 计数值到达 ``timersel`` 设定值时，硬件\ **把 RTC 计数器清零并从 0 重新计数**\ 。BK7236N 不用它来产生 alarm 中断，而是把它当作 RTC 的 \ **wrap 上限**\ ：``ana_rtc_hw_init`` 中无条件把 ``timersel`` 写为 ``AON_RTC_ROUND_TICK = 0xFFFFFFFFF`` (即 36 bit 自然上限)，对应 36 bit 计数器的自然回绕。软件再通过 ``s_high_tick`` 累加高位 (见后文 :ref:`64 bit tick 视图 <bk7236n_rtc_64bit_view>`)，对外呈现为一个连续的 64 bit tick。

BK7236N 默认 ``CONFIG_ANA_RTC_WAKUP_BY_RTC = 1`` ，本文及后续 API / ISR 描述均以该配置为准。

中断与时钟源相关 ana_reg
-------------------------

驱动 ``ana_rtc_hw_init()`` 中涉及的其它 ana_reg 字段：

.. list-table::
    :header-rows: 1
    :widths: 32 68

    * - 寄存器字段
      - 作用
    * - ``ana_reg5.pwd_rosc_spi``
      - ROSC 32K 模拟电路掉电使能；上电 RTC 时由驱动写 0 解除掉电。
    * - ``ana_reg5.rosc_disable``
      - ROSC 振荡器禁用；上电 RTC 时由驱动写 0 释放振荡器。
    * - ``ana_reg7.clk_sel``
      - 32 kHz 来源： ``1`` = 外部 32.768 kHz XTAL， ``0`` = 内部 ROSC。值由 ``CONFIG_EXTERN_32K`` 决定。
    * - ``ana_reg9.spi_timerwken``
      - RTC 计数器使能 / 清零控制：写 0 时 RTC 被清零且停止计数；写 1 时 RTC 从 0 开始计数。
    * - ``ana_reg9.spi_byp32pwd``
      - 32K 时钟旁路掉电（旁路）。

中断方面，驱动注册 ``INT_SRC_ANA_RTC`` (``icu_map.h`` 中映射到中断号 59) 的 ISR；中断使能 / 禁用通过 ``sys_hal_enable_ana_rtc_int()`` / ``sys_hal_disable_ana_rtc_int()`` 完成，二者内部除了开关 group2 中断，还会顺带操作 ``ana_reg8.rtcwk_rstn`` 与 ``ana_reg8.rst_timerwks1v``\ （清状态）。

软件架构
==========

驱动文件路径
-------------

BK7236N 上 RTC 驱动源文件位于 ``middleware/driver/rtc/ana_rtc_driver.c`` 。

公开头文件 ``include/driver/aon_rtc.h`` 与类型定义 ``include/driver/aon_rtc_types.h`` 与历史芯片保持兼容，对外的 API 命名空间仍沿用历史接口： ``bk_aon_rtc_*`` / ``bk_alarm_*`` / ``bk_rtc_*`` 。

层次划分
---------

.. list-table::
    :header-rows: 1
    :widths: 14 28 58

    * - 层
      - 主要文件
      - 职责
    * - **Driver**
      - ``ana_rtc_driver.c``
      - 维护 alarm 链表与节点池、注册 ``INT_SRC_ANA_RTC`` ISR、对外提供 ``bk_alarm_register`` / ``bk_aon_rtc_get_us`` / ``bk_rtc_settimeofday`` 等 API。
    * - **HAL (sys / pmu)**
      - ``sys_hal.c`` , ``aon_pmu_hal.c``
      - 封装 RTC tick 读取、采样边沿、ana_reg 字段读写、中断使能。
    * - **LL (寄存器)**
      - ``sys_ll.h`` , ``aon_pmu_ll.h``
      - 直接位段访问： ``aon_pmu_ll_get_r79_rtc_tick_l`` / ``sys_ll_set_ana_reg11_rtcsel``
        等内联函数。

驱动状态机
-----------

ana_rtc_driver 维护一个全局状态结构 ``s_ana_rtc``\ （含 ``inited`` 标志、alarm 链表头 ``alarm_head_p`` 、链表节点计数 ``alarm_node_cnt``\ ），加上一个独立的节点池 ``s_ana_rtc_nodes_p``\ （在 ``bk_aon_rtc_driver_init()`` 中通过 ``os_zalloc`` 申请，内含 ``AON_RTC_MAX_ALARM_CNT`` 个 ``alarm_node_t`` 与一个 ``busy_bits`` 位图）。

时钟与精度
============

时钟源与频率
-------------

BK7236N RTC 的 32 kHz 时钟来源由 ``CONFIG_EXTERN_32K`` 在编译时决定：

 - **未定义**\ （BK7236N 默认）：使用片上 ROSC 32K，标称 32 000 Hz；实际频率由 CKMN 校准动态修正，运行时通过 ``bk_rosc_32k_get_freq()`` 读取 (``middleware/driver/pwr_clk/rosc_32k.c``)。
 - **已定义**\ ：使用外部 32.768 kHz XTAL，频率为常量 ``AON_RTC_EXTERN_32K_CLOCK_FREQ = 32768`` 。

无论选哪种时钟源，驱动统一通过 ``bk_rtc_get_clock_freq()`` 把当前频率转成 ``uint32_t``
返回；上层的 tick / us 换算都基于它。

tick 与时间换算
----------------

驱动提供以下纯软件换算接口 (实现见 ``ana_rtc_driver.c``)：

.. list-table::
    :header-rows: 1
    :widths: 45 55

    * - API
      - 等价表达
    * - ``bk_rtc_get_ms_tick_count()``
      - ``bk_rtc_get_clock_freq() / 1000``
    * - ``bk_aon_rtc_tick_to_us(tick)``
      - ``tick * 1000000 / bk_rtc_get_clock_freq()``
    * - ``bk_aon_rtc_us_to_tick(us)``
      - ``us * bk_rtc_get_clock_freq() / 1000000``

由此可知：

 - **时间分辨率**\ 等于 32 kHz 时钟周期，约 **30.5 us**\ （XTAL 32 768 Hz）或
   **31.25 us**\ （ROSC 32 000 Hz）。
 - **绝对精度**\ ：XTAL 32K 由晶振决定（典型 ±20 ppm 量级，具体见所选晶振规格）；ROSC 32K 经 CKMN 校准后偏差 ``bk_rosc_32k_get_ppm()`` 可读，掉电不保持，复位后需重新校准。
 - **alarm 周期下限**\ ：驱动在 ``bk_alarm_register()`` 内强制 ``period_tick >= AON_RTC_PRECISION_TICK = 32`` ，否则返回 ``BK_FAIL`` ，相当于最小周期约 **1 ms** 。

.. note::

    应用层只关心毫秒 / 微秒时，\ **不要直接乘除常数 32 / 32768 / 32000**\ ：始终使用 ``AON_RTC_MS_TICK_CNT`` (即 ``bk_rtc_get_ms_tick_count()``) 或 ``bk_aon_rtc_us_to_tick()`` ，否则 ROSC 校准频率漂移、或编译期切换 ``CONFIG_EXTERN_32K`` 都会让定时偏长 / 偏短约 2.4%。

.. _bk7236n_rtc_64bit_view:

64 bit tick 视图
-----------------

``CONFIG_ANA_RTC_SUPPORT_64BIT = 1`` (BK7236N 默认) 下， ``rtc_tick_t`` 是 ``uint64_t`` ， ``bk_aon_rtc_get_current_tick()`` 返回 64 bit tick。其中：

 - 低 36 bit 来自 ``r79`` + ``r78`` 直读；
 - 高位由驱动软件计数：每次发现低 32 bit 比上一次小（绕回）就把软件高位 ``s_high_tick`` 自增 1。

只要应用持续调用 ``bk_aon_rtc_get_us()`` / ``bk_aon_rtc_get_current_tick()`` （或周期 alarm 能保证至少在 36 bit 周期内被触发一次），就不会丢失绕回。

运行机制
==========

alarm 注册与排序
-----------------

应用调用 ``bk_alarm_register(id, &alarm_info)`` 时，驱动按以下步骤处理 (见 ``middleware/driver/rtc/ana_rtc_driver.c``)：

1. 关中断 → 申请节点池中的空闲槽位（最多 ``AON_RTC_MAX_ALARM_CNT = 8`` ）；
2. 拷贝 ``name`` / ``period_tick`` / ``period_cnt`` / ``callback`` / ``param_p`` ；
3. 计算 ``expired_tick = bk_aon_rtc_get_current_tick(id) + period_tick`` ；
4. 拒绝同名 alarm（按 ``ALARM_NAME_MAX_LEN`` 字节比对），按 ``expired_tick`` 升序插入链表；
5. 若新节点是新链表头，立即把它的 ``expired_tick`` 写入硬件比较寄存器 (``ana_rtc_set_tick``)。

ana_rtc_set_tick 同步
-----------------------

向 ana_reg11 / 13 (rtcsel) 写值需要 ~3 个 32 kHz 周期完成跨域同步，驱动循环回读直到读出的值与写入的值一致；超时阈值 ``ANA_RTC_OPS_SAFE_DELAY_US = 125`` ，超时后打印 ``set tick timeout`` 错误。

ISR 处理
---------

``ana_rtc_isr_handler()`` 在 ``INT_SRC_ANA_RTC`` 触发时被调用：

1. 关 RTC 中断 (``sys_hal_disable_ana_rtc_int``)，关闭比较硬件；
2. 遍历链表头 (``alarm_head_p``)：

   - 若 ``expired_tick <= cur_tick + AON_RTC_PRECISION_TICK`` ，则调用其 callback；
   - ``period_cnt == 1`` 的一次性 alarm：从链表移除并归还节点池；
   - 周期 alarm (``period_cnt > 1`` 或 ``ALARM_LOOP_FOREVER``)： ``expired_tick += period_tick`` ，重新插入链表正确位置；

3. 把新链表头的 ``expired_tick`` 写入比较寄存器；若链表为空，写入 ``AON_RTC_ROUND_TICK + 1`` ；
4. 重新使能 RTC 中断 (``sys_hal_enable_ana_rtc_int`` ，发生在 ``ana_rtc_set_tick`` 末尾)。

.. warning::

    alarm callback 在 **ISR 上下文** 执行，必须遵守裸中断约束：

     - 不能调用任何会阻塞的 RTOS 接口 (``sleep`` / ``wait`` / 互斥量等)；
     - 不能在 callback 内调用 ``free`` / ``malloc``\ （FreeRTOS 禁止在 ISR 内分配 / 释放堆）；
     - 不能在 callback 内 unregister 自己；如要"用完即删"请把 ``period_cnt`` 设为 1，驱动会在 ISR 内自动归还节点。

低功耗管理
============

AON 域常驻运行
---------------

RTC 计数器、32 kHz 时钟、比较寄存器都在 AON 域，\ **Low-Voltage 与 Deep-Sleep 模式下 RTC 计数器保持运行、计数值连续、唤醒源能继续工作**\ 。这是 BK7236N 上 RTC 的核心定位 —— 给低功耗模式提供精确定时和唤醒能力。

唤醒源配合
-----------

让 RTC 在指定时刻把系统从低功耗模式唤醒的标准流程见
:doc:`../power_save/bk_rtc_sleep`\ ：

1. 通过 ``bk_alarm_register(AON_RTC_ID_1, &alarm)`` 注册一个一次性 alarm
   (``period_cnt = 1`` )，其 ``period_tick`` 决定睡眠时长。
2. ``bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL)`` 把 RTC 注册成唤醒源。
3. ``bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE)`` 或 ``PM_MODE_DEEP_SLEEP`` 触发睡眠。

驱动在每次写比较寄存器前会调用 ``ana_rtc_wakeup_config()`` 校验下次到期时间至少有 ``AON_RTC_MS_TICK_CNT`` (约 1 ms) 余量，否则返回 ``BK_FAIL`` ，避免错过本次唤醒。

Deep-Sleep 时间持久化
----------------------

``CONFIG_AON_RTC_KEEP_TIME_SUPPORT = 1`` 时驱动通过 ``bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_RTC, ...)`` 注册一对回调：

 - **进入 Deep-Sleep**\ ： ``bk_rtc_keep_time_enter_deepsleep()`` 把当前 ``bk_aon_rtc_get_us()`` 写入 EasyFlash 环境变量 ``rtc_ds_start_us`` ；
 - **唤醒后**\ ： ``aon_rtc_compute_boot_timeofday()`` 在 ``bk_aon_rtc_driver_init()`` 中被调用，根据 ``bk_misc_get_reset_reason()`` 判断是否为 Deep-Sleep 唤醒，若是则用当前 ``bk_aon_rtc_get_us()`` 减去 flash 中的 ``rtc_ds_start_us`` 得到睡眠时长，存入 ``s_rtc_keep_time_offset_us`` ；同时从 ``s_boot_time_us`` 恢复墙钟时间偏移。

应用层用以下接口拿结果：

 - ``bk_rtc_get_deepsleep_duration_seconds(uint32_t *)`` ：返回上一次 Deep-Sleep 持续秒数；
 - ``bk_rtc_gettimeofday()`` ：返回 ``s_boot_time_us + bk_aon_rtc_get_us()`` 形成的墙钟时间。

.. note::

    Deep-Sleep 持久化依赖 ``CONFIG_EASY_FLASH = 1`` 且 EasyFlash 已经 ``bk_set_env`` / ``bk_save_env`` 可用； ``CONFIG_EASY_FLASH = 0`` 时持久化路径直接返回 ``BK_FAIL`` ，墙钟时间在掉电后会丢失。

API 使用示例
==============

``alarm_info_t`` 字段说明
-------------------------

.. note::

    BK7236N 上 ``alarm_info_t`` 全部字段都生效，没有"为兼容老芯片保留"的字段：

    .. list-table::
        :header-rows: 1
        :widths: 20 80

        * - 字段
          - 说明
        * - ``name``
          - alarm 名称，最长 ``ALARM_NAME_MAX_LEN = 7`` 字节，超过部分被截断；同名 alarm
            注册会被拒绝。
        * - ``period_tick``
          - 周期 tick 数（基于 32 kHz）。最小值 ``AON_RTC_PRECISION_TICK = 32`` ，约 1 ms。从毫秒转换请用 ``period_tick = ms * AON_RTC_MS_TICK_CNT`` 。
        * - ``period_cnt``
          - 周期次数。 ``1`` = 一次性 alarm，触发一次后自动从链表移除； ``2`` ~
            ``0xFFFFFFFE`` = 触发 N 次后停止； ``ALARM_LOOP_FOREVER (0xFFFFFFFF)`` =
            永久周期，须显式 ``bk_alarm_unregister`` 才停止。
        * - ``callback``
          - 到期回调函数指针，签名为 ``void cb(aon_rtc_id_t, uint8_t *name, void *param)`` ，运行于 \ **ISR 上下文**\ 。
        * - ``param_p``
          - 透传给 callback 的 ``void *`` 业务参数。

周期 alarm 示例
----------------

下面给出 BK7236N 上注册一个 1 秒周期、永久循环 alarm 的典型流程 (参考 ``middleware/driver/rtc/aon_rtc_test.c``)：

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

一次性唤醒（Low-Voltage / Deep-Sleep 共用）
--------------------------------------------

只把 ``period_cnt`` 改成 ``1`` 即可让 alarm 在触发一次后自动消失，常用于"睡 N 秒后醒来"
场景：

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

读取当前时刻
-------------

.. code-block:: c

    /* Raw 64-bit tick (32 kHz units, range up to 36 bits hardware + software extension) */
    uint64_t tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);

    /* Free-running microseconds since power-on (or since clock-source switch) */
    uint64_t us = bk_aon_rtc_get_us();
    uint64_t ms = bk_aon_rtc_get_ms();

设置 / 读取墙钟时间
--------------------

需要注意 ``bk_rtc_settimeofday()`` 设置的是 ``s_boot_time_us`` ：本质是"当前 RTC us 对应的墙钟时间"，之后任何 ``bk_rtc_gettimeofday()`` 都返回 ``s_boot_time_us + bk_aon_rtc_get_us()`` 。

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
        bk_rtc_settimeofday(&tv, NULL);   /* CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1 时持久化 */

        /* Read it back later */
        struct timeval now;
        bk_rtc_gettimeofday(&now, NULL);
    }

获取上一次 Deep-Sleep 持续秒数
-------------------------------

.. code-block:: c

    uint32_t deep_sleep_sec = 0;
    if (bk_rtc_get_deepsleep_duration_seconds(&deep_sleep_sec) == BK_OK) {
        /* deep_sleep_sec is valid only when this boot followed a Deep-Sleep wake-up */
    }

关键 API 列表
===============

下表只列出 BK7236N 上 **实际可用**\ 的 RTC API。所有声明位于 ``include/driver/aon_rtc.h`` ；实现位于 ``middleware/driver/rtc/ana_rtc_driver.c`` 。

驱动初始化
-----------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - 说明
    * - ``bk_aon_rtc_driver_init``
      - 初始化软件状态、申请节点池、配置 32 kHz 时钟与比较寄存器、注册 ``INT_SRC_ANA_RTC`` ISR； ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1`` 时还会注册 Deep-Sleep keep-time 回调。\ **所有 RTC API 调用前必须先调用此接口**\ 。
    * - ``bk_aon_rtc_driver_deinit``
      - 反初始化驱动，关闭 ROSC、释放节点池、注销 ISR。

alarm 管理
-----------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - 说明
    * - ``bk_alarm_register``
      - 注册一条 alarm (参数见 ``alarm_info_t`` 字段说明表)。
    * - ``bk_alarm_unregister``
      - 按名称移除一条 alarm。

时间读取与换算
---------------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - 说明
    * - ``bk_aon_rtc_get_current_tick``
      - 返回当前 64 bit tick (含软件高位绕回累加)。
    * - ``bk_aon_rtc_get_us``
      - 返回当前累计微秒数 (基于 ``s_time_base_us`` 累加)。
    * - ``bk_aon_rtc_get_ms``
      - ``bk_aon_rtc_get_us() / 1000`` 。
    * - ``bk_aon_rtc_tick_to_us``
      - tick → us 换算。
    * - ``bk_aon_rtc_us_to_tick``
      - us → tick 换算。
    * - ``bk_rtc_get_clock_freq``
      - 返回当前 32 kHz 时钟实测频率 (XTAL 32768 或 ROSC 校准值)。
    * - ``bk_rtc_get_ms_tick_count``
      - ``bk_rtc_get_clock_freq() / 1000`` ，构成宏 ``AON_RTC_MS_TICK_CNT`` 。
    * - ``bk_rtc_set_clock_freq``
      - 通知驱动时钟频率发生变化 (内部仅调用一次 ``bk_aon_rtc_get_us`` 重新基准化时间，不直接写硬件寄存器)。

墙钟时间
---------

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - 说明
    * - ``bk_rtc_settimeofday``
      - 设置墙钟时间 (仅更新 ``s_boot_time_us`` ； ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT=1`` 时持久化到 EasyFlash)。
    * - ``bk_rtc_gettimeofday``
      - 读取墙钟时间。
    * - ``bk_rtc_get_deepsleep_duration_seconds``
      - 读取上一次 Deep-Sleep 的持续秒数。

调试
-----

.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - API
      - 说明
    * - ``bk_aon_rtc_dump``
      - 打印当前 alarm 链表 (仅 ``CONFIG_ANA_RTC_DEBUG`` 下输出 ISR 时间记录)。

.. note::

    ``include/driver/aon_rtc.h`` 中能看到的下列接口在 BK7236N 上 **未编译进二进制**\ ，是为兼容历史芯片保留的接口声明，请勿在 BK7236N 工程中使用，链接时会找不到符号：

     - ``bk_aon_rtc_create`` / ``bk_aon_rtc_destroy`` / ``bk_aon_rtc_tick_init`` / ``bk_aon_rtc_open_rtc_wakeup`` / ``bk_aon_rtc_register_upper_isr``
     - ``bk_rtc_ana_register_wakeup_source`` ：在 ``#if CONFIG_RTC_ANA_WAKEUP_SUPPORT`` 下，BK7236N 默认未启用。
     - ``bk_aon_rtc_register_tick_isr`` ：函数体在 BK7236N 上仅把回调指针写入 ``s_ana_rtc_tick_isr`` ，但驱动 ISR 实际只调用 alarm 链表里的回调，\ **不会回调到这里**\ 。请使用 ``bk_alarm_register`` 实现周期 / 一次性回调，不要依赖这个接口。

CLI 命令
==========

``CONFIG_AON_RTC_TEST = 1`` （BK7236N 默认）时， ``bk_aon_rtc_driver_init()`` 会自动注册一组 CLI 命令 (实现见 ``middleware/driver/rtc/aon_rtc_test.c``)：

.. list-table::
    :header-rows: 1
    :widths: 38 62

    * - 命令
      - 用途
    * - ``aon_rtc_driver {init|deinit}``
      - 手动初始化 / 反初始化 RTC 驱动
    * - ``aon_rtc_get_time {id}``
      - 打印当前 tick（已换算成 ms）
    * - ``aon_rtc_register {id} {name} {period_ms} {period_cnt}``
      - 注册一个 alarm，回调固定打印 ``id, name`` ；注意 CLI 内部会把 ``period_ms * AON_RTC_MS_TICK_CNT`` 转成 tick 再写入。
    * - ``aon_rtc_unregister {id} {name}``
      - 按名称取消注册
    * - ``aon_rtc_auto_test {id} {start|stop}``
      - 启动 / 停止内置 6 条 alarm 的相互注册自检
    * - ``aon_rtc_time_of_day {get|set} [sec usec]``
      - 读取 / 设置墙钟时间； ``get_deepsleep_time`` 子命令打印 ``bk_rtc_get_deepsleep_duration_seconds()`` 的返回值。
    * - ``aon_rtc_dump {id}``
      - 打印 alarm 链表

.. note::

    ``aon_rtc_clock_src {26m|rosc}`` 命令实际操作的是 LPO (低频时钟) source (``aon_pmu_drv_lpo_src_set``)，与本文讨论的 "RTC 32 kHz 时钟源 (XTAL/ROSC)" \ **不是同一回事**\ ，请勿混淆。BK7236N 上 RTC 时钟源的切换通过编译开关 ``CONFIG_EXTERN_32K`` 决定，运行时不可改。

注意事项
==========

1. **使用前必须先调用 bk_aon_rtc_driver_init**\ ：所有 ``bk_alarm_register`` /
   ``bk_aon_rtc_get_us`` / ``bk_rtc_settimeofday`` 等 API 都依赖驱动内部状态
   (``s_ana_rtc.inited`` 、节点池、ISR 注册)；跳过初始化会导致 ISR 不响应或访问空指针。

2. **alarm callback 在 ISR 上下文执行**\ ：禁止阻塞、 ``malloc`` / ``free`` 、
   unregister 自己；如需后台处理，请在 callback 内仅 set flag 或唤醒任务。

3. **alarm 名称最长 7 字节**\ ：``ALARM_NAME_MAX_LEN = 7`` ，多余字节会被截断；同名 alarm
   注册会失败 (``BK_FAIL``)。

4. **周期下限约 1 ms**\ ： ``period_tick < AON_RTC_PRECISION_TICK (32)`` 直接被驱动拒绝；实际 ISR 路径开销也限制了 sub-ms 的稳定性。

5. **唯一硬件实例**\ ： ``SOC_AON_RTC_UNIT_NUM = 1`` ， ``aon_rtc_id_t`` 取值仅 ``AON_RTC_ID_1`` 。所有 API 的 ``id`` 参数都应传 ``AON_RTC_ID_1``\ ；其它值会被驱动以 ``BK_ERR_PARAM`` 拒绝。

6. **32 kHz 时钟源在编译期决定**\ ： ``CONFIG_EXTERN_32K`` 选择 XTAL 或 ROSC，运行时不可切换。若在 ROSC 时钟源下对长期定时精度有要求，请确保 CKMN 校准 (``bk_rosc_32k_calib``) 按需启用。

7. **Deep-Sleep keep-time 依赖 EasyFlash**\ ： ``CONFIG_AON_RTC_KEEP_TIME_SUPPORT = 1`` 下时间持久化路径需要 ``CONFIG_EASY_FLASH = 1`` 才能成功；否则 ``bk_rtc_get_deepsleep_duration_seconds`` 仍可正常使用 (基于 RAM 中的 ``s_rtc_keep_time_offset_us``\ )，但墙钟时间在掉电后丢失。
