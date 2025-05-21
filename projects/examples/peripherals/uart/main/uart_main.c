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

#include <common/sys_config.h>
#include <components/log.h>
#include <os/mem.h>
#include <driver/uart.h>
#include "bk_private/bk_init.h"
#define TAG "uart_example"
#define UART_EXAMPLE_BUF_LEN       (128)
#define UART_EXAMPLE_RX_TIMEOUT    BEKEN_WAIT_FOREVER
#define USING_UART_SW_FIFO         (0)

static const uart_config_t s_config = {
	.baud_rate = CONFIG_UART_EXAMPLE_BAUD_RATE,
	.data_bits = CONFIG_UART_EXAMPLE_DATA_BITS,
	.parity = CONFIG_UART_EXAMPLE_PARITY,
	.stop_bits = CONFIG_UART_EXAMPLE_STOP_BITS,
	.flow_ctrl = CONFIG_UART_EXAMPLE_FLOW_CTRL,
	.src_clk = CONFIG_UART_EXAMPLE_SRC_CLK,
};

static beken_thread_t uart_start_thread= NULL;
static uint8_t uart_start_task_priority = 4;
static uint32_t uart_start_task_size = 1024;

static beken_thread_t uart_tx_thread= NULL;
static uint8_t uart_tx_task_priority = 4;
static uint32_t uart_tx_task_size = 1024;

static beken_thread_t uart_rx_thread= NULL;
static uint8_t uart_rx_task_priority = 4;
static uint32_t uart_rx_task_size = 1024;
static beken_semaphore_t uart_process_sema =NULL;

#if (!USING_UART_SW_FIFO)
static uint8_t s_rx_data_buff[UART_EXAMPLE_BUF_LEN] = {0};
static uint8_t s_rx_data_idx = 0;
#endif
extern bk_err_t uart_read_ready(uart_id_t id);
extern int uart_read_byte_ex(uart_id_t id, uint8_t *ch);

static void uart_example_rx_isr(uart_id_t id, void *param)
{
#if USING_UART_SW_FIFO
	{
		rtos_set_semaphore(&uart_process_sema); 
		BK_LOGI(TAG, "uart(%d) enter uart_example_rx_isr\r\n", id);
	}
#else
	int	ret_val = 0;
	uint8_t rec_data = 0;

	while(1)
	{
		ret_val = uart_read_byte_ex(CONFIG_UART_EXAMPLE_UART_ID, &rec_data);
		if(ret_val == -1)
		{
			BK_LOGI(TAG, "over  ..... \r\n");
			break;
		}
		os_memcpy(&s_rx_data_buff[s_rx_data_idx], &rec_data, 1);
		s_rx_data_idx++;
	}
	rtos_set_semaphore(&uart_process_sema); 
#endif
}

static bk_err_t uart_example_send_data(void)
{
	uint8_t *data = (uint8_t *)os_malloc(UART_EXAMPLE_BUF_LEN);
	BK_RETURN_ON_NULL(data);
	os_memset(data, 0, UART_EXAMPLE_BUF_LEN);

	BK_LOGI(TAG, "uart send data:\r\n");
	for (int i = 0; i < UART_EXAMPLE_BUF_LEN; i++) {
		data[i] = i & 0xff;
		BK_LOGI(TAG, "send_data[%d]=0x%x\r\n", i, data[i]);
	}
	BK_LOG_ON_ERR(bk_uart_write_bytes(CONFIG_UART_EXAMPLE_UART_ID, data, UART_EXAMPLE_BUF_LEN));
	if (!data) {
		os_free(data);
	}

	return BK_OK;
}

static bk_err_t uart_example_process_data(void)
{
#if USING_UART_SW_FIFO
	uint8_t *data = (uint8_t *)os_malloc(UART_EXAMPLE_BUF_LEN);
	BK_RETURN_ON_NULL(data);
	os_memset(data, 0, UART_EXAMPLE_BUF_LEN);

	int len = bk_uart_read_bytes(CONFIG_UART_EXAMPLE_UART_ID, data, UART_EXAMPLE_BUF_LEN, UART_EXAMPLE_RX_TIMEOUT);
	if (len < 0) {
		BK_LOGE(TAG, "uart read failed, ret:-0x%x\r\n", -len);
		goto exit;
	}

	BK_LOGI(TAG, "uart recv data:\r\n");
	for (int i = 0; i < len; i++) {
		BK_LOGI(TAG, "recv_data[%d]=0x%x\r\n", i, data[i]);
	}

exit:
	if (!data) {
		os_free(data);
	}
	data = NULL;

	return BK_OK;
#else
	BK_LOGI(TAG, "uart recv data:\r\n");
	for (int i = 0; i < s_rx_data_idx; i++) {
		BK_LOGI(TAG, "s_rx_data_buff[%d]=0x%x\r\n", i, s_rx_data_buff[i]);
	}
	s_rx_data_idx = 0;
	return BK_OK;
#endif
}

static void uart_tx_task(beken_thread_arg_t arg)
{	
	(void)arg;

	while(1)
	{
		if(uart_example_send_data() == BK_OK)
		{
			BK_LOGI(TAG, "uart_example_send_data over:\r\n");
		}
		rtos_delay_milliseconds(10000);
	}

}

static void uart_rx_task(beken_thread_arg_t arg)
{
	(void)arg;

	while(1)
	{
		if(rtos_get_semaphore(&uart_process_sema, BEKEN_WAIT_FOREVER) == BK_OK)
		{
			BK_LOGI(TAG, "uart_rx_task enter:\r\n");
			uart_example_process_data();
			BK_LOGI(TAG, "uart_example_process_data over:\r\n");
		}
		rtos_delay_milliseconds(10000);
	}
}

static void uart_start_task(beken_thread_arg_t arg)
{
	(void)arg;

	BK_LOG_ON_ERR(bk_uart_deinit(CONFIG_UART_EXAMPLE_UART_ID));
	BK_LOG_ON_ERR(bk_uart_init(CONFIG_UART_EXAMPLE_UART_ID, &s_config));
#if (!USING_UART_SW_FIFO)
	BK_LOG_ON_ERR(bk_uart_disable_sw_fifo(CONFIG_UART_EXAMPLE_UART_ID));
#endif
	BK_LOG_ON_ERR(bk_uart_register_rx_isr(CONFIG_UART_EXAMPLE_UART_ID, uart_example_rx_isr, NULL));
	BK_LOG_ON_ERR(bk_uart_enable_rx_interrupt(CONFIG_UART_EXAMPLE_UART_ID));

	rtos_init_semaphore(&uart_process_sema, 1);
	rtos_create_thread(&uart_tx_thread,
					   uart_tx_task_priority,
					   "uart_tx_task",
					   uart_tx_task,
					   uart_tx_task_size,
					   NULL
	);
	rtos_create_thread(&uart_rx_thread,
					   uart_rx_task_priority,
					   "uart_rx_task",
					   uart_rx_task,
					   uart_rx_task_size,
					   NULL
	);

	rtos_delete_thread(&uart_start_thread);
}

void uart_example_test_task(void)
{
	rtos_create_thread(&uart_start_thread,
					   uart_start_task_priority,
					   "uart_start_task",
					   uart_start_task,
					   uart_start_task_size,
					   NULL
	);
}
int main(void)
{
	bk_init();
	BK_LOG_ON_ERR(bk_uart_driver_init());

	uart_example_test_task();
	return 0;
}
