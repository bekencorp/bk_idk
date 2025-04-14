Dual BT
==========

:link_to_translation:`en:[English]`



.. important::

   The Dual BT API v1.0 is the lastest staBT Dual BT APIs. All new applications should use Dual BT API v1.0.


Dual BT API Categories
----------------------------

Most of Dual BT APIs can be categoried as:

Interface specific BT APIs:
 - BT gap interface
 - BT a2dp interface
 - BT avrcp interface
 - BT hfp interface
 - BT hid interface
 - BT l2cap interface
 - BT spp interface

Compatibility and Extension
----------------------------------------

The Dual BT APIs are flexiBT, easy to be extended and backward compatiBT. For most of the BT configurations, we put some reserved fields in the config struct for future extendence. The API users need to make sure the reserved fields are initialized to 0, otherwise the compatibility may be broken as more fields are added.

Programing Principle
----------------------------------------

.. important::
  Here is some general principle for BT API users:
   - Always init the reserved fields of config stuct to 0
   - Use BK_ERR_CHECK to check the return value of the BT API
   - If you are not sure how to use BT APIs, study the BT example code first
   - If you are not sure how to initialize some fields of config struct, use the default configuration macro to use the config first and then set application specific fields.
   - Don't do too much work in BT event callback, relay the event to your own application task.

User Development Model
----------------------------------------

Similar as most popular BT driver, the Beken BT driver is implemented as event driver. The application call BT APIs to operate the BT driver and get notified by BT event.


API Reference
----------------------------------------

.. include:: ../../_build/inc/bk_dm_gap_bt.inc
.. include:: ../../_build/inc/bk_dm_a2dp.inc
.. include:: ../../_build/inc/bk_dm_avrcp.inc
.. include:: ../../_build/inc/bk_dm_hfp.inc
.. include:: ../../_build/inc/bk_dm_hidd.inc
.. include:: ../../_build/inc/bk_dm_hidh.inc
.. include:: ../../_build/inc/bk_dm_l2cap.inc
.. include:: ../../_build/inc/bk_dm_spp.inc



API Typedefs
----------------------------------------

.. include:: ../../_build/inc/bk_dm_a2dp_types.inc
.. include:: ../../_build/inc/bk_dm_avrcp_types.inc
.. include:: ../../_build/inc/bk_dm_hfp_types.inc
.. include:: ../../_build/inc/bk_dm_gap_bt_types.inc


Reference link
----------------

    `API Reference : <../../api-reference/bluetooth/index.html>`_ Introduced the Bluetooth API interface
