#include <stdint.h>
#include "partitions.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
	PARTITION_PRIMARY_ALL = 0,
	PARTITION_SECONDARY_ALL,
	PARTITION_OTA,
	PARTITION_PARTITION,
	PARTITION_SPE,
	PARTITION_TFM_NS,
	PARTITION_NSPE,
	PARTITION_OTP_NV,
	PARTITION_PS,
	PARTITION_ITS,
	PARTITION_CNT,
};

#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
#define PARTITION_PPC_OFFSET             0x440
#define PARTITION_NAME_LEN               24
#define PARTITION_ENTRY_LEN              32
#define PARTITION_OFFSET_OFFSET          24
#define PARTITION_SIZE_OFFSET            28

#define FLASH_PHY2VIRTUAL(phy_addr)      ((((phy_addr) / 34) << 5) + ((phy_addr) % 34))
#define SOC_FLASH_BASE_ADDR 0x02000000

int partition_init(void);
uint32_t partition_get_phy_offset(uint32_t id);
uint32_t partition_get_phy_size(uint32_t id);
void dump_partition(void);
