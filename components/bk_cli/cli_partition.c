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
#include "argtable3.h"
#include "cli_common.h"
#include <driver/flash_partition.h>


#if (CONFIG_PARTITION_TEST)

extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_partition_cmd_handler(void **argtable)
{
	struct arg_lit *erase 				= (struct arg_lit *)argtable[1];
    struct arg_lit *read 				= (struct arg_lit *)argtable[2];
    struct arg_lit *write 				= (struct arg_lit *)argtable[3];
    struct arg_str *arg_parti_name 		= (struct arg_str *)argtable[4];
    struct arg_int *arg_offset 			= (struct arg_int *)argtable[5];
    struct arg_int *arg_size 			= (struct arg_int *)argtable[6];
    struct arg_str *arg_value 			= (struct arg_str *)argtable[7];
	bk_err_t err;
    if (erase->count > 0) {
		err = bk_flash_partition_erase_by_name((const char* )arg_parti_name->sval[0],(uint32_t)arg_offset->ival[0],(uint32_t)arg_size->ival[0]);
		CLI_LOGI(err ? "Erase partition fail, err = %x\r\n" : "Erase partition pass\r\n", err);
    }
	else if (read->count > 0)
	{
		uint8_t *read_buffer = os_malloc((uint32_t)arg_size->ival[0]);
		if (read_buffer) {
			err = bk_flash_partition_read_by_name((const char* )arg_parti_name->sval[0], read_buffer, (uint32_t)arg_offset->ival[0], (uint32_t)arg_size->ival[0]);
			if (err == BK_OK) {
				CLI_LOGI("Value (partition): \r\n");
				for (int i = 0; i < arg_size->ival[0]; i++) {
					BK_DUMP_OUT("%02x ", read_buffer[i]);
					if ((i + 1) % 16 == 0) {
						BK_DUMP_OUT("\r\n");
					}
				}
				if (arg_size->ival[0] % 16 != 0) {
					BK_DUMP_OUT("\r\n");
				}
			}
			os_free(read_buffer);
		} else {
			err = BK_ERR_NO_MEM;
		}
		CLI_LOGI(err ? "Read partition fail, err = %x\r\n" : "Read partition pass\r\n", err);
	}
	else if (write->count > 0)
	{
		const char *data_to_write = arg_value->sval[0];
		uint32_t data_len = strlen(data_to_write);

		if (data_len > (uint32_t)arg_size->ival[0]) {
			CLI_LOGE("Error: Data length exceeds specified size\n");
			return;
		}

		err = bk_flash_partition_write_by_name((const char* )arg_parti_name->sval[0], (uint8_t *)data_to_write, (uint32_t)arg_offset->ival[0], data_len);
		CLI_LOGI(err ? "Write partition fail, err = %x\r\n" : "Write partition pass\r\n", err);
	}
	else
	{
		print_help(argtable[0],argtable);
	}
	
}

static void cli_partition_erase_cmd_handler(void **argtable)
{
    struct arg_str *arg_parti_name 		= (struct arg_str *)argtable[1];
	bk_err_t err = bk_flash_partition_erase_all((const char* )arg_parti_name->sval[0]);
	CLI_LOGI(err ? "Erase all partition fail, err = %x\r\n" : "Erase all partition pass\r\n", err);
}

static void cli_partition_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	struct arg_lit *help            = arg_lit0("h", "help",    		"Display this help message");
	struct arg_lit *erase       	= arg_lit0("e", "erase", 		"Erase partition");
	struct arg_lit *read       		= arg_lit0("r", "read", 		"Read partition");
	struct arg_lit *write       	= arg_lit0("w", "write", 		"Write partition");
	struct arg_str *arg_parti_name  = arg_str1("n", "name",			"<partition_name>", 	"Partition name");
	struct arg_int *arg_offset      = arg_int1("o", "offset",     	"<offset>",      		"Partition offset");
	struct arg_int *arg_size        = arg_int1("s", "size",      	"<size>",       		"Partition size");
	struct arg_str *arg_value       = arg_str0("v", "value",    	"<value>",     			"Partition value (Required for -w/--write)");
	struct arg_end *end             = arg_end(20);

    void* argtable[] = { help, erase, read, write, arg_parti_name, arg_offset,arg_size, arg_value, end };
	int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_partition_cmd_handler);
}

static void cli_partition_erase_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	struct arg_lit *help            = arg_lit0("h", "help",    		"Display this help message");
	struct arg_str *arg_parti_name  = arg_str1("n", "name",			"<partition_name>", 	"Partition name");
	struct arg_end *end             = arg_end(20);

    void* argtable[] = { help, arg_parti_name, end };
	int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_partition_erase_cmd_handler);
}

#define PARTITION_CMD_CNT (sizeof(s_partition_commands) / sizeof(struct cli_command))
static const struct cli_command s_partition_commands[] = {
	{"partition", 			"partition { erase | read | write }", 			cli_partition_cmd			},
	{"partition_erase", 	"partition_erase { partition_name }", 			cli_partition_erase_cmd		},
};

int cli_partition_init(void)
{
	return cli_register_commands(s_partition_commands, PARTITION_CMD_CNT);
}
#endif