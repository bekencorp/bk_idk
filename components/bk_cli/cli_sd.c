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

#include "cli.h"
#include <driver/sd_card.h>

#if ((CONFIG_SDIO_V2P0) || (CONFIG_SDIO_V2P1) || CONFIG_SDIO_V2P2)
#include <sys_driver.h>
#endif

extern uint32_t sdcard_intf_test(void);
extern uint32_t test_sdcard_read(uint32_t blk, uint32_t blk_cnt);
extern uint32_t test_sdcard_write(uint32_t blk, uint32_t blk_cnt, uint32_t wr_val);
extern void sdcard_intf_close(void);

/**
 * Print usage help information for sdtest command
 */
static void sd_print_usage(void)
{
	os_printf("Usage: sdtest <cmd> [params]\r\n");
	os_printf("  I [param]    - Initialize SD card\r\n");
	os_printf("  R <blk> <cnt> - Read blocks\r\n");
	os_printf("  W <blk> <cnt> [val] - Write blocks\r\n");
	os_printf("  C            - Close SD card interface\r\n");
	os_printf("  S <clk>      - Set clock\r\n");
#if ((CONFIG_SDIO_V2P0) || (CONFIG_SDIO_V2P1) || CONFIG_SDIO_V2P2)
	os_printf("  --clk_src <val> --clk_div <val> - Set clock source and divider\r\n");
#endif
}

#if ((CONFIG_SDIO_V2P0) || (CONFIG_SDIO_V2P1) || CONFIG_SDIO_V2P2)
/**
 * Parse and set clock parameters from command line arguments
 * @param argc Number of arguments
 * @param argv Argument array
 * @return 1 if clock parameters were found and set, 0 otherwise
 */
static int sd_parse_clock_params(int argc, char **argv)
{
	int i;
	uint32_t clk_src = 0, clk_div = 0;
	int clk_src_set = 0, clk_div_set = 0;

	/* Parse clock parameters */
	for (i = 1; i < argc; i++) {
		if (os_strcmp(argv[i], "--clk_src") == 0) {
			if (i + 1 < argc) {
				clk_src = os_strtoul(argv[i + 1], NULL, 10);
				clk_src_set = 1;
				i++; /* Skip next parameter */
			}
		} else if (os_strcmp(argv[i], "--clk_div") == 0) {
			if (i + 1 < argc) {
				clk_div = os_strtoul(argv[i + 1], NULL, 10);
				clk_div_set = 1;
				i++; /* Skip next parameter */
			}
		}
	}

	/* Apply clock settings if found */
	if (clk_src_set || clk_div_set) {
		if (clk_div_set) {
			sys_driver_set_sdio_clk_div(clk_div);
			os_printf("set clk_div=%d\r\n", clk_div);
		}
		if (clk_src_set) {
			sys_driver_set_sdio_clk_sel(clk_src);
			os_printf("set clk_src=%d\r\n", clk_src);
		}
		return 1;
	}

	return 0;
}
#endif

static void sd_operate(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t cmd;
	uint32_t blknum = 0, blkcnt = 1, wr_val = 0x12345678;
	uint32_t ret;

	(void)pcWriteBuffer; /* Unused parameter */
	(void)xWriteBufferLen; /* Unused parameter */

	if (argc <= 1) {
		sd_print_usage();
		return;
	}

#if ((CONFIG_SDIO_V2P0) || (CONFIG_SDIO_V2P1) || CONFIG_SDIO_V2P2)
	/* Check if clock setting command */
	if (sd_parse_clock_params(argc, argv)) {
		return;
	}
#endif

	/* Parse traditional command format */
	cmd = argv[1][0];
	if (argc > 2) {
		blknum = os_strtoul(argv[2], NULL, 10);
	}
	if (argc > 3) {
		blkcnt = os_strtoul(argv[3], NULL, 10);
	}
	if (argc > 4) {
		wr_val = os_strtoul(argv[4], NULL, 16);
	}

	switch (cmd) {
	case 'I':
		ret = sdcard_intf_test();
		os_printf("init ret=0x%08x\r\n", ret);
		break;
	case 'R':
		if (argc < 3) {
			os_printf("Error: read command requires block number\r\n");
			return;
		}
		ret = test_sdcard_read(blknum, blkcnt);
		os_printf("read ret=0x%08x, blknum=%d, blkcnt=%d\r\n", ret, blknum, blkcnt);
		break;
	case 'W':
		if (argc < 3) {
			os_printf("Error: write command requires block number\r\n");
			return;
		}
		ret = test_sdcard_write(blknum, blkcnt, wr_val);
		os_printf("write ret=0x%08x, blknum=%d, blkcnt=%d, wr_val=0x%08x\r\n",
			ret, blknum, blkcnt, wr_val);
		break;
	case 'C':
		sdcard_intf_close();
		os_printf("sdtest close\r\n");
		break;
	case 'S':
		if (argc < 3) {
			os_printf("Error: set clock command requires clock value\r\n");
			return;
		}
		ret = bk_sd_card_set_clock(blknum);
		os_printf("set clock ret=0x%08x, clk=%d\r\n", ret, blknum);
		break;
	default:
		os_printf("Error: unknown command '%c'\r\n", cmd);
		os_printf("Use 'sdtest' without arguments for help\r\n");
		break;
	}
}

#define SD_CMD_CNT (sizeof(s_sd_commands) / sizeof(struct cli_command))
static const struct cli_command s_sd_commands[] = {
#if CONFIG_SDCARD
	{"sdtest", "sdtest [I|R|W|C|S|--clk_src <val> --clk_div <val>]", sd_operate},
#endif
};

int cli_sd_init(void)
{
	return cli_register_commands(s_sd_commands, SD_CMD_CNT);
}
