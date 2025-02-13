## Project：wifi wapi project

## Application：

    Armino Wi-Fi wapi example

## How to use example

1、Register follow API
	._wapi_wpi_encrypt = NULL,
	._wapi_wpi_decrypt = NULL,
2、Open follow macros
    CONFIG_LWIP_RESV_TLEN_ENABLE=y
    CONFIG_WAPI_INTERFACE=y

## Special Macro Configuration Description：

CONFIG_LWIP_RESV_TLEN_ENABLE=y
CONFIG_WAPI_INTERFACE=y

| Supported Targets | BK7236 |  BK7258 |
| ----------------- | ------ |  ------ |

## Complie Command:
1、make bk7236 PROJECT=wifi/wapi

## WIFI: WAPI


