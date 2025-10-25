Dual BLE APIs
================

:link_to_translation:`en:[English]`

.. important::

   The Dual BLE API v1.0 is the lastest stable Dual BLE APIs. All new applications should use Dual BLE API v1.0.


Dual BLE API Categories
----------------------------

Most of Dual BLE APIs can be categoried as:

Interface specific BLE APIs:
 - BLE gap interface
 - BLE gatt common interface
 - BLE gattc interface
 - BLE gatts interface

Compatibility and Extension
----------------------------------------

The Dual BLE APIs are flexible, easy to be extended and backward compatible. For most of the BLE configurations, we put some reserved fields in the config struct for future extendence. The API users need to make sure the reserved fields are initialized to 0, otherwise the compatibility may be broken as more fields are added.

Programing Principle
----------------------------------------

.. important::
  Here is some general principle for BLE API users:
   - Always init the reserved fields of config stuct to 0
   - Use BK_ERR_CHECK to check the return value of the BLE API
   - If you are not sure how to use BLE APIs, study the BLE example code first
   - If you are not sure how to initialize some fields of config struct, use the default configuration macro to use the config first and then set application specific fields.
   - Don't do too much work in BLE event callback, relay the event to your own application task.

User Development Model
----------------------------------------

Similar as most popular BLE driver, the Beken BLE driver is implemented as event driver. The application call BLE APIs to operate the BLE driver and get notified by BLE event.


User Guide
----------------------------------------
- create att database

::

    static const bk_gatts_attr_db_t s_gatts_attr_db_service_1[] =
    {
        //service
        {
            BK_GATT_PRIMARY_SERVICE_DECL(INTERESTING_SERIVCE_UUID),
        },

        //char 1
        {
            BK_GATT_CHAR_DECL(INTERESTING_CHAR_UUID,
                              sizeof(s_char_buff), s_char_buff,
                              BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                              BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                              BK_GATT_RSP_BY_APP),
        },
        {
            BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                                   sizeof(s_char_desc_buff), (uint8_t *)&s_char_desc_buff,
                                   BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                                   BK_GATT_RSP_BY_APP),
        },
    };

    ret = bk_ble_gatts_create_attr_tab(s_gatts_attr_db_service_1, s_gatts_if, sizeof(s_gatts_attr_db_service_1) / sizeof(s_gatts_attr_db_service_1[0]), 30);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_create_attr_tab err %d", ret);
        return -1;
    }


- start adv

::

    bk_ble_gap_ext_adv_params_t adv_param = {0};

    adv_param = (bk_ble_gap_ext_adv_params_t)
    {
        .type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
        .interval_min = 120 * 1,
        .interval_max = 160 * 1,
        .channel_map = BK_ADV_CHNL_ALL,
        .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        .primary_phy = BK_BLE_GAP_PRI_PHY_1M,
        .secondary_phy = BK_BLE_GAP_PHY_1M,
        .sid = 0,
        .scan_req_notif = 0,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    };

    ret = bk_ble_gap_set_adv_params(ADV_HANDLE, &adv_param);

    bk_ble_adv_data_t adv_data =
    {
        .set_scan_rsp = 0,
        .include_name = 1,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = 0x06,
    };

    ret = bk_ble_gap_set_adv_data((bk_ble_adv_data_t *)&adv_data);

    const bk_ble_gap_ext_adv_t ext_adv =
    {
        .instance = 0,
        .duration = 0,
        .max_events = 0,
    };

    ret = bk_ble_gap_adv_start(1, &ext_adv);



- start a connect

::

    bk_gap_create_conn_params_t param = {0};

    param.scan_interval = 800;
    param.scan_window = param.scan_interval / 2;
    param.initiator_filter_policy = 0;
    param.conn_interval_min = 0x20;
    param.conn_interval_max = 0x20;
    param.conn_latency = 0;
    param.supervision_timeout = 500;
    param.min_ce = 0;
    param.max_ce = 0;
    param.local_addr_type = BLE_ADDR_TYPE_PUBLIC;
    param.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;

    err = bk_ble_gap_connect(&param);

API Reference
----------------------------------------

.. include:: ../../_build/inc/bk_dm_gap_ble.inc
.. include:: ../../_build/inc/bk_dm_gattc.inc
.. include:: ../../_build/inc/bk_dm_gatts.inc
.. include:: ../../_build/inc/bk_dm_gatt_common.inc

API Typedefs
----------------------------------------
.. include:: ../../_build/inc/bk_dm_gap_ble_types.inc
.. include:: ../../_build/inc/bk_dm_gatt_types.inc
.. include:: ../../_build/inc/bk_dm_bluetooth.inc
.. include:: ../../_build/inc/bk_dm_bluetooth_types.inc

Reference link
----------------

    `API Reference : <../../api-reference/bluetooth/index.html>`_ Introduced the Bluetooth API interface

