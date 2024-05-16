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
#include <driver/otp.h>

#if CONFIG_SOC_BK7236XX

uint32_t findFirstZeroBit(uint8_t num)
{
	uint32_t positition = 0;
	while((num & 1) && (positition < 8)){
		num = num >> 1;
		++positition;
	}
	return positition;
}


static void cli_otp_help(void)
{
	CLI_LOGI("otp_test flow_test\r\n");
	CLI_LOGI("otp_test read_write\r\n");
}
static void cli_otp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern bk_err_t bk_otp_fully_flow_test();
	extern uint32_t bk_otp_apb_read_mask(otp_id_t item);
	extern bk_err_t bk_otp_apb_write_mask(otp_id_t item, uint32_t mask);
	extern uint32_t bk_otp_apb_write_permission(otp_id_t item, uint32_t permission);
	extern uint32_t bk_otp_apb_read_permission(otp_id_t item);

	if (argc < 2) {
		cli_otp_help();
		return;
	}

	if (os_strcmp(argv[1], "flow_test") == 0) {
		bk_otp_init();
		uint32_t ret = bk_otp_fully_flow_test();
		CLI_LOGI("flow test ret = %d\r\n",ret);
	} else if (os_strcmp(argv[1], "read_write") == 0){

		uint8_t* data=(uint8_t*)os_malloc(5*sizeof(uint8_t));
		if(data == NULL)
			CLI_LOGE("no memory\r\n");

		uint32_t ret = bk_otp_apb_read(OTP_APB_TEST,data,5);
		CLI_LOGI("read test ret = %d\r\n",ret);
		for(int i=0;i<5;i++){
			CLI_LOGI("data[%d] = 0x%x\r\n",i,data[i]);
		}

		/*find first zero bit*/
		uint32_t position;
		for(int i=0;i<5;i++){
			position = findFirstZeroBit(data[i]);
			if(data[i] != 0xFF){
				data[i] |= 1 << position;
				CLI_LOGI("data[%d] = 0x%x will be written!\r\n",i,data[i]);
				break;
			}
		}

		ret = bk_otp_apb_update(OTP_APB_TEST,data,5);
		CLI_LOGI("write test ret = %d\r\n",ret);
		ret = bk_otp_apb_read(OTP_APB_TEST,data,5);
		CLI_LOGI("after write, read ret = %d\r\n",ret);
		for(int i=0;i<5;i++){
			CLI_LOGI("data[%d] = 0x%x\r\n",i,data[i]);
		}
	} else if (os_strcmp(argv[1], "init") == 0){
		bk_otp_init();
	}


}

#else /* CONFIG_SOC_BK7236XX */

#define OTP_BANK_SIZE   (0x800)
static void cli_otp_help(void)
{
	CLI_LOGI("otp_test read [addr] [length]\r\n");
}

static void cli_otp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_otp_help();
		return;
	}

	uint32_t addr, length;
	uint8_t *buff_p, *record_p;

	if (os_strcmp(argv[1], "read") == 0) {
		addr   = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 16);

		if((addr > OTP_BANK_SIZE) || ((addr + length) > OTP_BANK_SIZE))
		{			
			CLI_LOGI("\r\n addr or length invalid! \r\n");	
			return ;
		}

		record_p = buff_p = (uint8_t *)malloc(length);
		memset(buff_p, 0x0, length);

		BK_LOG_ON_ERR(bk_otp_read_bytes_nonsecure(buff_p, addr, length));

		for(int i=0; i<length; i++){
			bk_printf("%02x",*buff_p);
			buff_p++;
		}
		free(record_p);
		CLI_LOGI("\r\n read OTP suc! \r\n");
	} else {
		cli_otp_help();
		return;
	}
}
#endif

#define OTP_CMD_CNT (sizeof(s_otp_commands) / sizeof(struct cli_command))
static const struct cli_command s_otp_commands[] = {
	{"otp_test", "otp_test {read}", cli_otp_cmd},
};

int cli_otp_init(void)
{
	return cli_register_commands(s_otp_commands, OTP_CMD_CNT);
}
