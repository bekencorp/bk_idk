// Copyright 2024-2025 Beken
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

#include <components/log.h>
#include <driver/spi.h>
#include <driver/dma.h>
#include <os/mem.h>
#include <os/os.h>
#include <string.h>

#define TAG "spi_rz"
#define RZ_BUFFER_LEN 72

static void rgb_to_zero_code_8(uint8_t r, uint8_t g, uint8_t b, uint8_t *output)
{
	int bit_index = 0;
	int byte_index = 0;

	output[byte_index] = 0;

	for (int color = 0; color < 3; color++) {
		uint8_t value = (color == 0) ? r : (color == 1) ? g : b;

		for (int i = 7; i >= 0; i--) {
			if ((value >> i) & 1) {
				output[byte_index] |= (0x3 << (6 - bit_index));
			} else {
				output[byte_index] |= (0x2 << (6 - bit_index));
			}
			bit_index += 3;
			if (bit_index >= 9) {
				bit_index = 0;
				byte_index++;
				if(byte_index < 8)
					output[byte_index] = 0;
			}
		}
	}
}

static void rgb_to_zero_code_48(uint8_t r, uint8_t g, uint8_t b, uint16_t *output)
{
	int byte_index = 0;

	for (int color = 0; color < 3; color++) {
		uint8_t value = (color == 0) ? r : (color == 1) ? g : b;

		for (int i = 7; i >= 0; i--) {
			if ((value >> i) & 1) {
				output[byte_index] = (0xFF80);
			} else {
				output[byte_index] = (0xF000);
			}
			byte_index++;
		}
	}
}

int bk_spi_rz_demo(spi_id_t spi_id)
{
	bk_err_t ret = BK_OK;
	uint32_t buf_len = 32 * 48;
	uint8_t *send_data = NULL;
	uint16_t rz_buffer[RZ_BUFFER_LEN];
	int j = 0;
	spi_config_t config = {0};

	send_data = (uint8_t *)os_zalloc(buf_len);
	if (send_data == NULL) {
		BK_LOGE(TAG, "spi_rz_demo malloc failed\n");
		return BK_ERR_NO_MEM;
	}

	config.role = SPI_ROLE_MASTER;
	config.bit_width = SPI_BIT_WIDTH_16BITS;
	config.polarity = 0;
	config.phase = 1;
	config.wire_mode = SPI_3WIRE_MODE;
	config.baud_rate = 13000000;
	config.bit_order = SPI_MSB_FIRST;
#if CONFIG_SPI_BYTE_INTERVAL
	config.byte_interval = 0;
#endif
#if CONFIG_SPI_DMA
	config.dma_mode = 1;
	config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
	config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
	config.spi_tx_dma_width = SPI_BIT_WIDTH_16BITS;
	config.spi_rx_dma_width = SPI_BIT_WIDTH_16BITS;
#endif

	BK_LOG_ON_ERR(bk_spi_driver_init());
	BK_LOG_ON_ERR(bk_spi_init(spi_id, &config));
	rgb_to_zero_code_48(99, 0, 0, rz_buffer);//green
	rgb_to_zero_code_48(0, 99, 0, rz_buffer + 24);//red
	rgb_to_zero_code_48(0, 0, 99, rz_buffer + 48);//blue
	BK_LOGI(TAG, "RZ Code:\r\n");
	for(int i = 0; i < RZ_BUFFER_LEN; i++) {
		BK_LOG_RAW("%X ", rz_buffer[i]);
	}
	BK_LOG_RAW("\r\n");

	for (int i = 0; i < 15; i++) {
		BK_LOGI(TAG, "spi_rz test cnt:%d\r\n", i);
		if(j == 0)
			rgb_to_zero_code_48(0, 99, 0, rz_buffer);//RED
		else if(j == 1)
			rgb_to_zero_code_48(99, 0, 0, rz_buffer);//GREEN
		else if(j == 2)
			rgb_to_zero_code_48(0, 0, 99, rz_buffer);//BLUE
		j++;
		if (j > 2) j = 0;
		for (int i = 0; i < buf_len; i += 48) {
			memcpy(send_data + i, (uint8_t *)rz_buffer, 48);
		}
		ret = bk_spi_dma_write_bytes(spi_id, send_data, buf_len);
		if (ret != buf_len) {
			BK_LOGE(TAG, "bk_spi_dma_write_bytes failed, ret:%d\r\n", ret);
		}
		memset(send_data, 0x00, buf_len);
		memset(rz_buffer, 0x00, 144);
		rtos_delay_milliseconds(500);
	}

	if (send_data) {
		os_free(send_data);
	}
	send_data = NULL;
	return 0;
}
