I2S APIs
================

:link_to_translation:`en:[English]`

.. important::

   The I2S API v1.0 is the lastest stable I2S APIs. All new applications should use I2S API v1.0.

I2S Interface
----------------------------

The BK I2S Driver supports following sample rates:

 - 8K
 - 12K
 - 16K
 - 24K
 - 32K
 - 48K
 - 96K
 - 8.0182K
 - 11.025K
 - 22.050K
 - 44.1K
 - 88.2K

The BK I2S Driver supports following work modes:

I2S (Philips)
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 1`` :

.. figure:: ../../../_static/i2s_Philips.png
    :align: center
    :alt: Philips_mode_time_sequence_diagram
    :figclass: align-center

    Figure 1. Philips mode time sequence diagram

Left Justified
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 2`` :

.. figure:: ../../../_static/i2s_Left_Justified.png
    :align: center
    :alt: Left Justified mode time sequence diagram
    :figclass: align-center

    Figure 2. Left Justified mode time sequence diagram

Right Justified
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 3`` :

.. figure:: ../../../_static/i2s_RightJustified.png
    :align: center
    :alt: Right Justified mode time sequence diagram
    :figclass: align-center

    Figure 3. Right Justified mode time sequence diagram

Short Frame Sync
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 4`` :

.. figure:: ../../../_static/i2s_ShortSyncFrame.png
    :align: center
    :alt: Short Frame Sync mode time sequence diagram
    :figclass: align-center

    Figure 4. Short Frame Sync mode time sequence diagram

Long Frame Sync
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 5`` :

.. figure:: ../../../_static/i2s_LongSyncFrame.png
    :align: center
    :alt: Long Frame Sync mode time sequence diagram
    :figclass: align-center

    Figure 5. Long Frame Sync mode time sequence diagram

Normal 2B+D
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 6 and Figure 7`` :

.. figure:: ../../../_static/i2s_Normal2BD_D0.png
    :align: center
    :alt: Normal 2B+D mode time sequence diagram
    :figclass: align-center

    Figure 6. Normal 2B+D mode time sequence diagram

.. figure:: ../../../_static/i2s_Normal2BD_Dx.png
    :align: center
    :alt: Normal 2B+D mode time sequence diagram
    :figclass: align-center

    Figure 7. Normal 2B+D mode time sequence diagram

Delay 2B+D
	In this working mode, the sequence diagram is shown in the sequence transmission as shown in ``Figure 8 and Figure 9`` :

.. figure:: ../../../_static/i2s_Delay2BD_D0.png
    :align: center
    :alt: Delay 2B+D mode time sequence diagram
    :figclass: align-center

    Figure 8. Delay 2B+D mode time sequence diagram

.. figure:: ../../../_static/i2s_Delay2BD_Dx.png
    :align: center
    :alt: Delay 2B+D mode time sequence diagram
    :figclass: align-center

    Figure 9. Delay 2B+D mode time sequence diagram

Each function can be operated independently. However both function share the same resources,  only one function can exit and work at a time.

I2S API Categories
----------------------------

Most of Audio APIs can be categoried as:

 - Client Use I2S APIs

   The Client Use I2S APIs depends on DMA. The APIs send and recevice data via DMA not CPU.

 - Other I2S APIs.

   The other I2S APIs provides control of the I2S driver.

.. important::
  Here is some general principle for BLE API users:
   - In general, it is recommended that customers use the convenient "Client Use I2S APIs" for application development.
   - If you have special needs, you can also use the "Other I2S APIs" to achieve the transmission and reception of I2S data.

Client Use I2S APIs:
 - :cpp:func:`bk_i2s_driver_init` - init i2s module driver
 - :cpp:func:`bk_i2s_driver_deinit` - deinit i2s module driver
 - :cpp:func:`bk_i2s_init` - init i2s module
 - :cpp:func:`bk_i2s_deinit` - deinit i2s module
 - :cpp:func:`bk_i2s_chl_init` - i2s channel init
 - :cpp:func:`bk_i2s_chl_deinit` - i2s channel deinit
 - :cpp:func:`bk_i2s_start` - start i2s transfer
 - :cpp:func:`bk_i2s_stop` - stop i2s transfer

Other I2S APIs:
 - :cpp:func:`bk_i2s_get_read_ready` - get the i2s Rx fifo status
 - :cpp:func:`bk_i2s_get_write_ready` - get the i2s Tx fifo status
 - :cpp:func:`bk_i2s_enable` - enable i2s
 - :cpp:func:`bk_i2s_int_enable` - config i2s interrupt
 - :cpp:func:`bk_i2s_set_role` - config i2s work role
 - :cpp:func:`bk_i2s_set_work_mode` - config i2s work mode
 - :cpp:func:`bk_i2s_set_lrck_invert` - config i2s lrck invert
 - :cpp:func:`bk_i2s_set_sck_invert` - config i2s sck invert
 - :cpp:func:`bk_i2s_set_lsb_first` - config i2s lsb first
 - :cpp:func:`bk_i2s_set_sync_len` - config i2s sync length
 - :cpp:func:`bk_i2s_set_data_len` - config i2s data length
 - :cpp:func:`bk_i2s_set_pcm_dlen` - config i2s pcm D length of 2B+D
 - :cpp:func:`bk_i2s_set_store_mode` - config i2s store mode
 - :cpp:func:`bk_i2s_clear_rxfifo` - clear i2s rx fifo
 - :cpp:func:`bk_i2s_clear_txfifo` - clear i2s tx fifo
 - :cpp:func:`bk_i2s_set_txint_level` - config i2s tx interrupt level
 - :cpp:func:`bk_i2s_set_rxint_level` - config i2s rx interrupt level
 - :cpp:func:`bk_i2s_write_data` - write data to i2s tx fifo
 - :cpp:func:`bk_i2s_read_data` - read data from i2s rx fifo
 - :cpp:func:`bk_i2s_get_data_addr` - get i2s data address
 - :cpp:func:`bk_i2s_set_samp_rate` - config i2s sample rate
 - :cpp:func:`bk_i2s_register_i2s_isr` - register i2s isr


API Reference
----------------------------------------

.. include:: ../../_build/inc/i2s.inc

API Typedefs
----------------------------------------
.. include:: ../../_build/inc/i2s_types.inc

Reference link
----------------------------------------

    `API Reference : <../../api-reference/peripheral/bk_i2s.html>`_ Introduced the I2S API interface

    `User and Developer Guide : <../../developer-guide/peripheral/bk_i2s.html>`_ Introduced common usage scenarios of I2S

    `Samples and Demos: <../../examples/peripheral/bk_i2s.html>`_ Introduced the use and operation of I2S samples
