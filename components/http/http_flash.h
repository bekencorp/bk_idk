#include "bk_drv_model.h"
#if (CONFIG_SECURITY_OTA)
#include "partitions.h"
#endif
#if CONFIG_FLASH_ORIGIN_API
#include "BkDriverFlash.h"
#else
#include <driver/flash_partition.h>
#endif

#if (CONFIG_SECURITY_OTA)
#undef HTTP_WR_TO_FLASH
#define HTTP_WR_TO_FLASH        0
#else
#define HTTP_WR_TO_FLASH        1
#endif

typedef struct http_data_st {
	UINT32 http_total;
	UINT8 do_data;
#if HTTP_WR_TO_FLASH || CONFIG_DIRECT_XIP || CONFIG_OTA_OVERWRITE
	UINT8 *wr_buf ;
	UINT8 *wr_tmp_buf;
	UINT16 wr_last_len ;
	UINT32 flash_address;
	bk_logic_partition_t *pt;
#endif
	DD_HANDLE flash_hdl;
} HTTP_DATA_ST;

#if CONFIG_UVC_OTA_DEMO
#define TCP_LEN_MAX             1000//896
#else
#define TCP_LEN_MAX             1460
#endif
#define HTTP_FLASH_ADDR         0xff000

extern  void store_block(unsigned block, uint8_t *src, unsigned len);
#define WR_BUF_MAX 1048

extern HTTP_DATA_ST *bk_http_ptr;
