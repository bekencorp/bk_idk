// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bk7286xx.h"
#include "os/os.h"
#include "common/bk_assert.h"
#include "sdkconfig.h"

/*
* #define ARM_MPU_RBAR(BASE, SH, RO, NP, XN)
*
* \brief Region Base Address Register value
* \param BASE The base address bits [31:5] of a memory region. The value is zero extended. Effective address gets 32 byte aligned.
* \param SH Defines the Shareability domain for this memory region.
* \param RO Read-Only: Set to 1 for a read-only memory region.
* \param NP Non-Privileged: Set to 1 for a non-privileged memory region.
* \oaram XN eXecute Never: Set to 1 for a non-executable memory region.
*/
ARM_MPU_Region_t mpu_regions[] = {
    /* MPU region 0，
    	itcm_s:  0x0000 0000-----------0x0000 3FFF
    	itcm_ns: 0x1000 0000-----------0x1000 3FFF
    	dtcm_s:  0x2000 0000-----------0x2000 3FFF
    	dtcm_ns: 0x3000 0000-----------0x3000 3FFF
     */
    { ARM_MPU_RBAR(0x00000000UL, ARM_MPU_SH_NON, 0, 1, 0),
      ARM_MPU_RLAR(0x27FFFFE0UL, 0) },

    /* MPU region 1，
        shared memory(smem0) 0x2800 0000-----------0x2800 FFFF   0x3800 0000-----------0x3800 FFFF 
        shared memory(smem1) 0x2801 0000-----------0x2801 FFFF   0x3801 0000-----------0x3801 FFFF
        shared memory(smem2) 0x2802 0000-----------0x2802 FFFF   0x3802 0000-----------0x3802 FFFF
        shared memory(smem3) 0x2804 0000-----------0x2804 FFFF   0x3804 0000-----------0x3804 FFFF
        shared memory(smem4) 0x2806 0000-----------0x2806 FFFF   0x3806 0000-----------0x3806 FFFF
     */
    { ARM_MPU_RBAR(0x28000000UL, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x3FFFFFE0UL, 1) },

    /* MPU region 2，periphral, device memory
        device memory is shareable, and must not be cached.
        please refer to the document:star_user_guide_reference_material.pdf page50
	 */
    { ARM_MPU_RBAR(0x40000000UL, ARM_MPU_SH_INNER, 0, 1, 1),
      ARM_MPU_RLAR(0x5FFFFFE0UL, 2) },

    /* MPU region 3，psram*/
    { ARM_MPU_RBAR(0x60000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x63FFFFE0UL, 3) },

    /* MPU region 4，qspi0*/
    { ARM_MPU_RBAR(0x64000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x67FFFFE0UL, 3) },

     /* MPU region 5，qspi1*/
     { ARM_MPU_RBAR(0x68000000UL, ARM_MPU_SH_NON, 0, 1, 1),
       ARM_MPU_RLAR(0x6BFFFFE0UL, 3) },

     /* MPU region 6， ppb and other */
     { ARM_MPU_RBAR(0x6C000000UL, ARM_MPU_SH_NON, 0, 1, 1),
       ARM_MPU_RLAR(0xEFFFFFE0UL, 2) }
};

/*
 For the star processor, only two combinations of these attributes are valid:Device-nGnRnE/Device-nGnRE
 please refer to the document:star_user_guide_reference_material.pdf page50
 */
uint8_t mpu_attrs[] = {
    ARM_MPU_ATTR(0xb, 0xb), // Normal memory, cacheable write through, read allocate, write allocate
    ARM_MPU_ATTR(0x4, 0x4), // Normal memory, non-cacheable
    ARM_MPU_ATTR(0x0, 0x0), // Device memory, bit[3:4]:nGnRnE-00,nGnRE-01
    ARM_MPU_ATTR(0xf, 0xf)  // Normal memory, cacheable write back, read allocate, write allocate
};
	
void mpu_register_regions(ARM_MPU_Region_t *regions, uint32_t region_cnt);
void mpu_register_attrs(uint8_t *attrs, uint32_t cnt);

void mpu_soc_cfg(void)
{
	mpu_register_regions(mpu_regions, sizeof(mpu_regions)/sizeof(mpu_regions[0]));
	mpu_register_attrs(mpu_attrs, sizeof(mpu_attrs)/sizeof(mpu_attrs[0]));
}
// eof

