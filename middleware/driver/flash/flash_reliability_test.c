// Copyright 2020-2025 Beken
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

#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "cli.h"
#include "flash_driver.h"

static const uint32_t crc32_table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * Calculate the CRC32 value of a memory buffer.
 *
 * @param crc accumulated CRC32 value, must be 0 on first call
 * @param buf buffer to calculate CRC32 value for
 * @param size bytes in buffer
 *
 * @return calculated CRC32 value
 */
 static uint32_t my_calc_crc32(uint32_t crc, const void *buf, size_t size)
{
    const uint8_t *p;

    p = (const uint8_t *)buf;
    crc = crc ^ ~0U;

    while (size--) {
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}


#define FLASH_IO_UP(x)                 //(GPIO_UP(x))
#define FLASH_IO_DOWN(x)               //(GPIO_DOWN(x))

#define BOOT_BASE_ADDR                 0x200000
#define FLASH_TEST_LENGTH              4096
#define FLASH_TEST_TOTLE_LENGTH        4096 // len = 0x2f000 = 47 * 4k
#define FLASH_TEST_ADDR BOOT_BASE_ADDR + FLASH_TEST_TOTLE_LENGTH
//#define GPIO_TOGGLE_TEST
#define READ_TEST_LENGTH               4096
//#define COUNT_SAVE_ADDR                (BOOT_BASE_ADDR - 4096)
#define COUNT_SAVE_ADDR                (BOOT_BASE_ADDR)
#define COUNT_SAVE_BAKEUP_ADDR         (BOOT_BASE_ADDR - 4096)

#define MAX_WRITE_ERR_COUNT            (0x0)
#define MAX_ERASE_ERR_COUNT            (0x0)

uint32_t flash_bypass_operate_sr_init(void);
struct ft_save_st
{
    uint32_t total_test_count;
    uint32_t total_psram_init_error_count;
    uint32_t total_psram_error_count;
    uint32_t total_easer_error_count;
    uint32_t total_write_error_count;
    uint32_t total_sys_dump_count;
    uint32_t crc;
};
struct ft_save_st g_ft_save = {0};
static beken_thread_t  s_flash_pressue_task_handle = NULL;
uint8 read_buf[FLASH_TEST_LENGTH];
uint8 write_buf[FLASH_TEST_LENGTH];
uint32_t s_flash_pressure_start = 0;

void flash_pressure_show_info(void)
{
    os_printf("\r\nBuild_info:%s_%s\r\n", __DATE__, __TIME__); //Jul 03 2018
    os_printf("BK7236N");
    os_printf(" flash pressure test case, base_addr:0x%08x,total_len:%d,len:%d\r\n", BOOT_BASE_ADDR, FLASH_TEST_TOTLE_LENGTH, FLASH_TEST_LENGTH);
    os_printf("flash id:%lx\r\n", bk_flash_get_id());

    bk_flash_read_bytes(COUNT_SAVE_ADDR, (uint8_t*)&g_ft_save, sizeof(g_ft_save));
    uint32 crc = 0;
    crc = my_calc_crc32(crc, &g_ft_save, sizeof(g_ft_save)-4);

    os_printf("\r\nfound history info: \r\n");
    os_printf("  total_test_count: %d\r\n", g_ft_save.total_test_count);
    os_printf("  total_psram_init_error_count: %d\r\n", g_ft_save.total_psram_init_error_count);
    os_printf("  total_psram_error_count: %d\r\n", g_ft_save.total_psram_error_count);
    os_printf("  total_erase_error_count: %d\r\n", g_ft_save.total_easer_error_count);
    os_printf("  total_write_error_count: %d\r\n", g_ft_save.total_write_error_count);
    os_printf("  total_sys_dump_count: %d\r\n", g_ft_save.total_sys_dump_count);
    os_printf("  curent crc: %x - temp:%x\r\n", g_ft_save.crc, crc);

    if (g_ft_save.crc == 0xffffffff)
    {
        os_printf("found history info not effected, use all 0 for init\r\n");
    }
    else if (crc != g_ft_save.crc)
    {
        os_printf("found history info crc err!\r\n");
    }
    else
    {
        os_printf("found history info effected\r\n");
    }
}

void flash_pressure_show_bakup_info(void)
{
    os_printf("\r\nbakeup info\r\n");

    struct ft_save_st ft_save = {0};
    bk_flash_read_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t*)&ft_save, sizeof(ft_save));
    uint32 crc = 0;
    crc = my_calc_crc32(crc, &ft_save, sizeof(ft_save)-4);

    os_printf("  total_test_count: %d\r\n", ft_save.total_test_count);
    os_printf("  total_psram_init_error_count: %d\r\n", ft_save.total_psram_init_error_count);
    os_printf("  total_psram_error_count: %d\r\n", ft_save.total_psram_error_count);
    os_printf("  total_erase_error_count: %d\r\n", ft_save.total_easer_error_count);
    os_printf("  total_write_error_count: %d\r\n", ft_save.total_write_error_count);
    os_printf("  total_sys_dump_count: %d\r\n", ft_save.total_sys_dump_count);
    os_printf("  curent crc: %x - temp:%x\r\n", ft_save.crc, crc);

    if (ft_save.crc == 0xffffffff)
    {
        os_printf("found history info not effected\r\n");
    }
    else if (crc != ft_save.crc)
    {
        os_printf("found history info crc err!\r\n");
    }
    else
    {
        os_printf("found history info effected\r\n");
    }
}

void flash_pressure_generate_data(void)
{
    uint32 write_cnt = g_ft_save.total_test_count;
    uint32 i, crc = 0;

    crc = my_calc_crc32(crc, &g_ft_save, sizeof(g_ft_save)-4);
    g_ft_save.crc = crc;

    for (i = 0; i < FLASH_TEST_LENGTH; i++)
    {
        write_buf[i] = (uint8)((i + write_cnt) & 0xFF);
    }

    memcpy(write_buf, &g_ft_save, sizeof(g_ft_save));
}

uint32_t flash_pressure_test(void)
{
    uint32 current_addr = 0;
    uint32 write_err_cnt = 0, erase_err_cnt = 0;
    uint32 flag = 0, i;

    current_addr = BOOT_BASE_ADDR;
    FLASH_IO_UP(2);
    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    do {
        // check erase
        FLASH_IO_UP(3);
        if((current_addr & 0xFFF) == 0)
        {
            bk_flash_erase_sector(current_addr);

            memset(read_buf, 0, FLASH_TEST_LENGTH);
            bk_flash_read_bytes(current_addr, (uint8_t*)read_buf, FLASH_TEST_LENGTH);
            for (i = 0; i < FLASH_TEST_LENGTH; i++)
            {
                if (read_buf[i] != 0xFF)
                {
                    if (erase_err_cnt > MAX_ERASE_ERR_COUNT)
                    {
                        os_printf("\r\n");
                        break;
                    }
                    os_printf("%x - rd[%d]:%x\r\n", current_addr+i, i, read_buf[i]);
                    erase_err_cnt++;
                }
            }

            if (erase_err_cnt > MAX_ERASE_ERR_COUNT)
            {
                os_printf("earse fail:%d\r\n", erase_err_cnt);
                g_ft_save.total_easer_error_count++;
                FLASH_IO_DOWN(3);
                break;
            }
        }
        FLASH_IO_DOWN(3);

        FLASH_IO_UP(4);
        // check write
        {
            flash_pressure_generate_data();
            memset(read_buf, 0, FLASH_TEST_LENGTH);

            bk_flash_write_bytes(current_addr, write_buf, FLASH_TEST_LENGTH);
            bk_flash_read_bytes(current_addr, (uint8_t*)read_buf, FLASH_TEST_LENGTH);
            for (i = 0; i < FLASH_TEST_LENGTH; i++)
            {
                if (write_buf[i] != read_buf[i])
                {
                    if (write_err_cnt > MAX_WRITE_ERR_COUNT)
                    {
                        os_printf("\r\n");
                        break;
                    }
                    os_printf("%x - w[%d]:%x  , r[%d]:%x\r\n", current_addr + i, i, write_buf[i], i, read_buf[i]);
                    write_err_cnt++;
                }
            }
            if (write_err_cnt > MAX_WRITE_ERR_COUNT)
            {
                os_printf("write fail:%d\r\n", write_err_cnt);
                g_ft_save.total_write_error_count++;
                FLASH_IO_DOWN(4);
                break;
            }
        }
        FLASH_IO_DOWN(4);
        flag = 1;
    } while(0);
    FLASH_IO_DOWN(2);

    return flag;
}

void flash_psram_save_result(uint32 flag, uint32 is_both)
{
    if((flag) || (is_both))
    {
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        bk_flash_erase_sector(COUNT_SAVE_ADDR);
        flash_pressure_generate_data();
        bk_flash_write_bytes(COUNT_SAVE_ADDR, (uint8_t *)&g_ft_save, sizeof(g_ft_save));
        bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    }

    if(((g_ft_save.total_test_count % 100) == 0) || (is_both))
    {
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        bk_flash_erase_sector(COUNT_SAVE_BAKEUP_ADDR);
        flash_pressure_generate_data();
        bk_flash_write_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t *)&g_ft_save, sizeof(g_ft_save));
        bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    }
}

void flash_psram_save_result_in_dump(void)
{
    if(s_flash_pressure_start == 0)
        return;
    g_ft_save.total_sys_dump_count++;
    flash_psram_save_result(1, 1);
}

void flash_psram_pressure_test(beken_thread_arg_t arg)
{
    uint32 crc, flag = 0;

    FLASH_IO_UP(15);
    GPIO_UP(15);
    bk_flash_read_bytes(COUNT_SAVE_ADDR, (uint8_t*)&g_ft_save, sizeof(g_ft_save));
    crc = 0;
    crc = my_calc_crc32(crc, &g_ft_save, sizeof(g_ft_save)-4);

    if (g_ft_save.crc == 0xffffffff)
    {
        os_printf("found history info not effected, use all 0 for init\r\n");
        os_memset(&g_ft_save, 0, sizeof(g_ft_save));
    }
    else if (crc != g_ft_save.crc)
    {
        os_printf("found history info crc err\r\n");

        struct ft_save_st ft_save = {0};
        bk_flash_read_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t*)&ft_save, sizeof(ft_save));
        uint32 crc = 0;
        crc = my_calc_crc32(crc, &ft_save, sizeof(ft_save)-4);

        if ((crc != ft_save.crc) || (ft_save.crc == 0xffffffff))
        {
            os_printf("found bakup history info not effected, use all 0 for init\r\n");
            os_memset(&g_ft_save, 0, sizeof(g_ft_save));
        }
        else
        {
            os_printf("found bakeup history info effected, use bakeup\r\n");
            memcpy(&g_ft_save, &ft_save, sizeof(ft_save));
        }
    }

    s_flash_pressure_start = 1;
    g_ft_save.total_test_count++;

#if CONFIG_PSRAM_PRESSURE_TEST
    uint32_t cli_psram_test_handle(uint32_t *number);
    uint32_t error_num = 0;

    FLASH_IO_UP(19);
    uint32_t ret = cli_psram_test_handle(&error_num);
    bk_printf("cli_psram_test_handle :%d, %d\r\n", ret, error_num);
    FLASH_IO_DOWN(19);

    if(ret == 0)
    {
        g_ft_save.total_psram_init_error_count++;
    }
    else if((ret == 1) && (error_num))
    {
        g_ft_save.total_psram_error_count++;
    }
    else
    {
        flag |= 0x1;
    }
#endif // CONFIG_PSRAM_PRESSURE_TEST

#if CONFIG_FLASH_PRESSURE_TEST
    uint32 t_flag = flash_pressure_test();
    if(t_flag == 1)
        flag |= 0x2;
#endif

    uint32 err_happened = 0, need_flag = 0;
#if CONFIG_PSRAM_PRESSURE_TEST
    if((flag & 0x1) == 0)
    {
        os_printf(" psram pressure test failed, total cnt: %d, total failed cnt: %d, %d\r\n",
            g_ft_save.total_test_count, g_ft_save.total_psram_init_error_count, g_ft_save.total_psram_error_count);
        err_happened = 1;
    }
#endif

#if CONFIG_FLASH_PRESSURE_TEST
    if((flag & 0x2) == 0)
    {
        os_printf(" flash pressure test failed, total cnt: %d, total write cnt: %d, total easer cnt: %d\r\n",
            g_ft_save.total_test_count, g_ft_save.total_write_error_count, g_ft_save.total_easer_error_count);
        err_happened = 1;
    }
#endif

    if(err_happened == 0)
        os_printf(" flash/psram pressure test done, total cnt: %d\r\n", g_ft_save.total_test_count);

    if((err_happened) || (COUNT_SAVE_ADDR != BOOT_BASE_ADDR))
    {
        need_flag = 1;
    }

    flash_psram_save_result(need_flag, 0);
    if((g_ft_save.total_test_count % 100) == 0)
    {
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        bk_flash_erase_sector(COUNT_SAVE_BAKEUP_ADDR);
        flash_pressure_generate_data();
        bk_flash_write_bytes(COUNT_SAVE_BAKEUP_ADDR, (uint8_t *)&g_ft_save, sizeof(g_ft_save));
        bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    }

    bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    s_flash_pressue_task_handle = NULL;
    s_flash_pressure_start = 0;
    GPIO_DOWN(15);
    FLASH_IO_DOWN(15);
    rtos_delete_thread(NULL);
}

void flash_pressue_test_start(void)
{
    if(s_flash_pressue_task_handle == NULL)
    {
        rtos_create_thread(&s_flash_pressue_task_handle,
            4,
            "flash_test",
            (beken_thread_function_t)flash_psram_pressure_test,
            1536,
            NULL);
    }
}

static void cli_flash_pressue_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    flash_pressue_test_start();
}

static void cli_flash_pressue_show_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    flash_pressure_show_info();
    flash_pressure_show_bakup_info();
}