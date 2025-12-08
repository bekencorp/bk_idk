/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 /*
* Change Logs:
* Date			 Author 	  Notes
* 2024-04-11	 Beken	  adapter to Beken sdk
*/
#include <string.h>
#include "soc/soc.h"
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "nvs_internal.h"

extern "C" const bk_logic_partition_t * pt_get_partition_info(bk_partition_t partition);
extern "C" uint32_t pt_get_partition_index(const bk_logic_partition_t* partition);
extern "C" uint32_t pt_get_partition_count(void);

extern "C" uint32_t flash_partition_get_index(const bk_logic_partition_t *partition_ptr)
{
	uint32_t id;

	id = pt_get_partition_index(partition_ptr);

	return id;
}

extern "C" const bk_logic_partition_t *bk_partition_find_first(esp_partition_type_t type,
        esp_partition_subtype_t subtype, const char *label)
{
	uint32_t i;
	uint32_t partitions_count = pt_get_partition_count();

	const bk_logic_partition_t *res = NULL, *tmp;

	for(i = 0; i < partitions_count; ++i){
		tmp = pt_get_partition_info((bk_partition_t)i);
		if (tmp){
			if((NULL == label) || (NULL == tmp->partition_description))
			{
				continue;
			}

			if(0 == strcmp(label, tmp->partition_description))
			{
				res = tmp;
				break;
			}
		}else{
			continue;
		}		
	}
	
    return res;
}
// eof

