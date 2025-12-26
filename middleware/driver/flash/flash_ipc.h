// Copyright 2020-2026 Beken
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

#ifndef _FLASH_IPC_H_
#define _FLASH_IPC_H_

#include <common/bk_include.h>

enum
{
	FLASH_CMD_ERASE_SECTOR = 0,
	FLASH_CMD_ERASE_32k,
	FLASH_CMD_ERASE_BLOCK,
	FLASH_CMD_READ,
	FLASH_CMD_READ_DONE,
	FLASH_CMD_WRITE,
	FLASH_CMD_FAST_ERASE,
} ;

typedef struct
{
	u32     part_id  : 8;
	u32     addr     : 24;
	u8    * buff;
	u16     len;
	int16   ret_status;
	u32     crc;
} flash_cmd_t;

#define FLASH_IPC_READ_SIZE     0x200
#define FLASH_IPC_WRITE_SIZE    0x200

#endif //_FLASH_IPC_H_
// eof

