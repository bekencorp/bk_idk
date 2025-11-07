// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include "cli.h"
#include <driver/sdio_host.h>
#include <driver/sd_card.h>

#define SD_CMD_GO_IDLE_STATE 0
#define SD_BLOCK_SIZE 512
#define SD_CARD_READ_BUFFER_SIZE 512
#define CMD_TIMEOUT_200K	5000	//about 5us per cycle (25ms)
#define DATA_TIMEOUT_13M	6000000 //450ms

static void cli_sdio_host_help(void)
{
	CLI_LOGI("sdio_host_driver init\r\n");
	CLI_LOGI("sdio_host driver deinit\r\n");
	CLI_LOGI("sdio send_cmd Index Arg(hex-decimal) RSP_Type Timeout_Value\r\n");
}

static void cli_sdio_host_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_sdio_host_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_sdio_host_driver_init());
		CLI_LOGI("sdio_host driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_sdio_host_driver_deinit());
		CLI_LOGI("sdio_host driver deinit\n");
	} else {
		cli_sdio_host_help();
		return;
	}
}

static void cli_sdio_host_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_sdio_host_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		sdio_host_config_t sdio_cfg = {0};

#if (CONFIG_SDIO_V2P0)
		sdio_cfg.clock_freq = SDIO_HOST_CLK_3_2M; //xtal div:128
#else
		sdio_cfg.clock_freq = CONFIG_SDIO_HOST_DEFAULT_CLOCK_FREQ;
#endif
		sdio_cfg.bus_width = SDIO_HOST_BUS_WIDTH_4LINE;
		sdio_cfg.dma_tx_en = 1;
		sdio_cfg.dma_rx_en = 1;

		BK_LOG_ON_ERR(bk_sdio_host_init(&sdio_cfg));
		CLI_LOGI("sdio host init\r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_sdio_host_deinit());
		CLI_LOGI("sdio host deinit\r\n");
	} else if (os_strcmp(argv[1], "send_cmd") == 0) {
		bk_err_t error_state = BK_OK;
		sdio_host_cmd_cfg_t cmd_cfg = {0};

		//modify to send cmd by parameter,then we can easy to debug any cmds.
		cmd_cfg.cmd_index = os_strtoul(argv[2], NULL, 10);			//CMD0,CMD-XXX,SD_CMD_GO_IDLE_STATE
		cmd_cfg.argument = os_strtoul(argv[3], NULL, 16);			//0x123456xx
		cmd_cfg.response = os_strtoul(argv[4], NULL, 10);			//SDIO_HOST_CMD_RSP_NONE,SHORT,LONG
		cmd_cfg.wait_rsp_timeout = os_strtoul(argv[5], NULL, 10);	//CMD_TIMEOUT_200K;

		bk_sdio_host_send_command(&cmd_cfg);
		error_state = bk_sdio_host_wait_cmd_response(cmd_cfg.cmd_index);
		if (error_state != BK_OK) {
			CLI_LOGW("sdio:cmd %d err:-%x\r\n", cmd_cfg.cmd_index, -error_state);
		}
	} else if (os_strcmp(argv[1], "config_data") == 0) {
		sdio_host_data_config_t data_config = {0};

		data_config.data_timeout = DATA_TIMEOUT_13M;
		data_config.data_len = SD_BLOCK_SIZE * 1;
		data_config.data_block_size = SD_BLOCK_SIZE;
		data_config.data_dir = SDIO_HOST_DATA_DIR_RD;

		BK_LOG_ON_ERR(bk_sdio_host_config_data(&data_config));
		CLI_LOGI("sdio host config data ok\r\n");
	} else if (os_strcmp(argv[1], "read_data") == 0) {
		uint8_t *buf = os_zalloc(SD_BLOCK_SIZE);
		sdio_host_data_config_t data_config = {0};
		bk_err_t error_state = BK_OK;
		sdio_host_cmd_cfg_t cmd_cfg = {0};
		if (buf == NULL) {
			CLI_LOGE("sdio host buf malloc failed\r\n");
			return;
		}
		os_memset(buf, 0xff, SD_BLOCK_SIZE);
		for (int i = 0; i < SD_BLOCK_SIZE; i++) {
			CLI_LOGI("buf[%d]=%x\r\n", i, buf[i]);
		}

		data_config.data_timeout = DATA_TIMEOUT_13M;
		data_config.data_len = SD_BLOCK_SIZE * 1;
		data_config.data_block_size = SD_BLOCK_SIZE;
		data_config.data_dir = SDIO_HOST_DATA_DIR_RD;
		BK_LOG_ON_ERR(bk_sdio_host_config_data(&data_config));

		//modify to send cmd by parameter,then we can easy to debug any cmds.
		cmd_cfg.cmd_index = 53;			//CMD53
		/*
			位31:    rw = 0
			位30-28: func_num = 1 = 001
			位27:    block_mode = 1:block mode, 0:byte mode
			位26:    op_mode = 0
			位25-9:  addr = 1 = 00000000001000000
			位8-0:   count = 1 = 1
		*/
		cmd_cfg.argument = 0x18000201;
		cmd_cfg.response = SDIO_HOST_CMD_RSP_SHORT;
		cmd_cfg.wait_rsp_timeout = 5000;
		bk_sdio_host_send_command(&cmd_cfg);
		error_state = bk_sdio_host_wait_cmd_response(cmd_cfg.cmd_index);
		if (error_state != BK_OK) {
			CLI_LOGW("sdio:cmd %d err:-%x\r\n", cmd_cfg.cmd_index, -error_state);
		}

		bk_sdio_host_read_blks_fifo(buf, SD_BLOCK_SIZE);
		CLI_LOGI("sdio host read data ok\r\n");
		for (int i = 0; i < SD_BLOCK_SIZE; i++) {
			CLI_LOGI("buf[%d]=%x\r\n", i, buf[i]);
		}
		if (buf) {
			os_free(buf);
			buf = NULL;
		}
	} else if (os_strcmp(argv[1], "write_data") == 0) {
		#define WRITE_BLOCK_CNT  (4)
		#define WRITE_BLOCK_SIZE (SD_BLOCK_SIZE * WRITE_BLOCK_CNT)

		uint8_t *buf = os_zalloc(WRITE_BLOCK_SIZE);
		sdio_host_data_config_t data_config = {0};
		bk_err_t error_state = BK_OK;
		sdio_host_cmd_cfg_t cmd_cfg = {0};

		if (buf == NULL) {
			CLI_LOGE("sdio host buf malloc failed\r\n");
			return;
		}
		os_memset(buf, 0x55, WRITE_BLOCK_SIZE);
		for (int i = 0; i < WRITE_BLOCK_SIZE; i++) {
			CLI_LOGI("buf[%d]=%x\r\n", i, buf[i]);
		}

		data_config.data_timeout = DATA_TIMEOUT_13M * WRITE_BLOCK_CNT;
		data_config.data_len = WRITE_BLOCK_SIZE * 1;
		data_config.data_block_size = SD_BLOCK_SIZE;
		data_config.data_dir = SDIO_HOST_DATA_DIR_WR;
		BK_LOG_ON_ERR(bk_sdio_host_config_data(&data_config));

		//modify to send cmd by parameter,then we can easy to debug any cmds.
		cmd_cfg.cmd_index = 53; //CMD53
		cmd_cfg.argument = 0x98000200 + WRITE_BLOCK_CNT;
		cmd_cfg.response = SDIO_HOST_CMD_RSP_SHORT;
		cmd_cfg.wait_rsp_timeout = 5000;
		bk_sdio_host_send_command(&cmd_cfg);
		error_state = bk_sdio_host_wait_cmd_response(cmd_cfg.cmd_index);
		if (error_state != BK_OK) {
			CLI_LOGW("sdio:cmd %d err:-%x\r\n", cmd_cfg.cmd_index, -error_state);
		}

		bk_sdio_host_write_fifo(buf, WRITE_BLOCK_SIZE);
		CLI_LOGI("sdio host write data ok\r\n");
		if (buf) {
			os_free(buf);
			buf = NULL;
		}
	} else {
		cli_sdio_host_help();
		return;
	}
}

static void cli_sd_card_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_sdio_host_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_sd_card_init());
		CLI_LOGI("sd card init ok\r\n");
	} else if(os_strcmp(argv[1], "deinit") == 0){
		BK_LOG_ON_ERR(bk_sd_card_deinit());
		CLI_LOGI("sd card deinit ok\r\n");
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint32_t block_num = os_strtoul(argv[2], NULL, 10);
		uint8_t *buf = os_malloc(SD_CARD_READ_BUFFER_SIZE * block_num);
		if (buf == NULL) {
			CLI_LOGE("sd card buf malloc failed\r\n");
			return;
		}
		BK_LOG_ON_ERR(bk_sd_card_read_blocks(buf, 0, block_num));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);
		for (int i = 0; i < SD_CARD_READ_BUFFER_SIZE * block_num; i++) {
			CLI_LOGI("buf[%d]=%x\r\n", i, buf[i]);
		}
		if (buf) {
			os_free(buf);
			buf = NULL;
		}
		CLI_LOGI("sd card read ok\r\n");
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint32_t block_num = os_strtoul(argv[2], NULL, 10);
		uint8_t *buf = os_malloc(SD_CARD_READ_BUFFER_SIZE * block_num);
		if (buf == NULL) {
			CLI_LOGE("sd card buf malloc failed\r\n");
			return;
		}
		for (int i = 0; i < SD_CARD_READ_BUFFER_SIZE * block_num; i++) {
			buf[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_sd_card_write_blocks(buf, 0, block_num));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);
		if (buf) {
			os_free(buf);
			buf = NULL;
		}
		CLI_LOGI("sd card write ok\r\n");
	} else if (os_strcmp(argv[1], "erase") == 0) {
		uint32_t block_num = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_sd_card_erase(0, block_num));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);
		CLI_LOGI("sd card erase ok\r\n");
	} else if (os_strcmp(argv[1], "cmp") == 0) {
		BK_LOG_ON_ERR(bk_sd_card_erase(0, 2));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);

		uint8_t *write_buf = os_malloc(SD_CARD_READ_BUFFER_SIZE * 2);
		if (write_buf == NULL) {
			CLI_LOGE("sd card write_buf malloc failed\r\n");
			return;
		}
		for (int i = 0; i < SD_CARD_READ_BUFFER_SIZE * 2; i++) {
			write_buf[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_sd_card_write_blocks(write_buf, 0, 2));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);

		uint8_t *read_buf = os_malloc(SD_CARD_READ_BUFFER_SIZE * 2);
		if (read_buf == NULL) {
			CLI_LOGE("sd card read_buf malloc failed\r\n");
			return;
		}
		BK_LOG_ON_ERR(bk_sd_card_read_blocks(read_buf, 0, 2));
		while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER);

		int ret = os_memcmp(write_buf, read_buf, SD_CARD_READ_BUFFER_SIZE * 2);
		if (ret == 0) {
			CLI_LOGI("sd card test ok\r\n");
		}

		if (write_buf) {
			os_free(write_buf);
			write_buf = NULL;
		}

		if (read_buf) {
			os_free(read_buf);
			read_buf = NULL;
		}
	} else {
		cli_sdio_host_help();
		return;
	}
}

#define SDIO_HOST_CMD_CNT (sizeof(s_sdio_host_commands) / sizeof(struct cli_command))
static const struct cli_command s_sdio_host_commands[] = {
	{"sdio_host_driver", "sdio_host_driver {init|deinit}", cli_sdio_host_driver_cmd},
	{"sdio", "sdio {init|deinit|send_cmd|config_data}", cli_sdio_host_cmd},
	{"sd_card", "sd_card {init|deinit|read|write|erase|cmp|}", cli_sd_card_cmd},
};

int cli_sdio_host_init(void)
{
	BK_LOG_ON_ERR(bk_sdio_host_driver_init());
	return cli_register_commands(s_sdio_host_commands, SDIO_HOST_CMD_CNT);
}

