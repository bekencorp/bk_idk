#include "nvs_err.h"
#include "nvs_partition.hpp"
#include "nvs_flash.h"
#include "nvs_internal.h"

#ifndef NVS_PARTITION_LOOKUP_HPP_
#define NVS_PARTITION_LOOKUP_HPP_

namespace nvs {

namespace partition_lookup {

bk_err_t lookup_nvs_partition(const char* label, NVSPartition **p);

#if CONFIG_NVS_ENCRYPTION
bk_err_t lookup_nvs_encrypted_partition(const char* label, nvs_sec_cfg_t* cfg, NVSPartition **p);
#endif // CONFIG_NVS_ENCRYPTION

} // partition_lookup

} // nvs

extern "C" const bk_logic_partition_t *bk_partition_find_first(esp_partition_type_t type,
        esp_partition_subtype_t subtype, const char *label);

#endif // NVS_PARTITION_LOOKUP_HPP_
