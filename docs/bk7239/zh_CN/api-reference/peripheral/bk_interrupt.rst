Interrupt Control Unit
===========================

Beken chip supports interrupt service function.


Interrupt API Status
----------------------

+--------------------------------------------+---------+
| API                                        | BK7239  |
+============================================+=========+
| :cpp:func:`bk_int_isr_register`            | Y       |
+--------------------------------------------+---------+
| :cpp:func:`bk_int_isr_unregister`          | Y       |
+--------------------------------------------+---------+
| :cpp:func:`bk_int_set_priority`            | Y       |
+--------------------------------------------+---------+
| :cpp:func:`bk_int_set_group`               | Y       |
+--------------------------------------------+---------+
| :cpp:func:`bk_get_int_statis`              | Y       |
+--------------------------------------------+---------+
| :cpp:func:`bk_dump_int_statis`             | Y       |
+--------------------------------------------+---------+

Interrupt API Reference
--------------------------

.. include:: ../../_build/inc/int.inc

Interrupt API Typedefs
------------------------

.. include:: ../../_build/inc/int_types.inc


