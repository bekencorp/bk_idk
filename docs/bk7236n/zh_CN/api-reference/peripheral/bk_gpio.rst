IOMUX
================

The BK7236N has an I/O multiplexer (IOMUX) that allows the use of I/O pins as either GPIOs or I/Os for peripherals.

GPIO API Status
------------------


+----------------------------------------------+---------+
| API                                          | BK7236N |
+==============================================+=========+
| :cpp:func:`bk_gpio_driver_init`              | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_driver_deinit`            | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_enable_output`            | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_disable_output`           | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_enable_input`             | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_disable_input`            | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_enable_pull`              | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_disable_pull`             | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_pull_up`                  | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_pull_down`                | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_set_output_high`          | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_set_output_low`           | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_get_input`                | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_set_config`               | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_register_isr`             | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_unregister_isr`           | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_enable_interrupt`         | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_disable_interrupt`        | Y       |
+----------------------------------------------+---------+
| :cpp:func:`bk_gpio_set_interrupt_type`       | Y       |
+----------------------------------------------+---------+


GPIO API Reference
---------------------

.. include:: ../../_build/inc/gpio.inc

GPIO API Typedefs
---------------------
.. include:: ../../_build/inc/gpio_types.inc


