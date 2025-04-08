/* BK_CSI Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/system.h>
#include <modules/raw_link.h>
#include <os/mem.h>
#include <os/os.h>
#include "bk_csi_demo.h"
#include "FreeRTOS.h"
#include "timers.h"

/* previous definition */
static void turn_on_light(uint8_t color, bool flicker);

beken_queue_t bk_csi_demo_queue;
static bool bk_csi_demo_started = false;
static TimerHandle_t flicker_timer = NULL;
static bool light_status = false;
static uint8_t current_color = 0;

__maybe_unused static uint8_t gpio_array[BK_CSI_DEMO_GPIO_NUM] = {
	BK_CSI_DEMO_GPIO_LEFT_RED_LIGHT,
	BK_CSI_DEMO_GPIO_LEFT_GREEN_LIGHT,
	BK_CSI_DEMO_GPIO_LEFT_BLUE_LIGHT,
	BK_CSI_DEMO_GPIO_RIGHT_RED_LIGHT,
	BK_CSI_DEMO_GPIO_RIGHT_GREEN_LIGHT,
	BK_CSI_DEMO_GPIO_RIGHT_BLUE_LIGHT
};

/*
 * light color <- map -> gpio status
 * 8 lines   for 7 colors and light off
 * 6 columns for 6 gpios
 * left Red    34
 * left Green  32
 * left Blue   36
 * right Red   18
 * right Green 24
 * right Blue  19
 */
__maybe_unused static uint8_t color_gpio_map[BK_CSI_DEMO_LIGHT_COLOR_NUM][BK_CSI_DEMO_GPIO_NUM] = {
   //R  G  B  R  G  B
	{0, 0, 0, 0, 0, 0}, //light off
	{1, 1, 1, 1, 1, 1}, //white
	{0, 0, 1, 0, 0, 1}, //blue
	{0, 1, 0, 0, 1, 0}, //green
	{1, 0, 0, 1, 0, 0}, //red
	{1, 0, 1, 1, 0, 1}, //fuchsia(PinHong)
	{1, 1, 0, 1, 1, 0}, //yellow
	{0, 1, 1, 0, 1, 1}, //cyan
};

static void down_all_gpio() {
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7236)
	for(int i = 0; i < BK_CSI_DEMO_GPIO_NUM; i++) {
		GPIO_DOWN(gpio_array[i]);
	}
#endif
}

static void bk_csi_demo_light_flicker(TimerHandle_t xTimer) {
	if (light_status) {
		down_all_gpio();
		light_status = false;
	} else {
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7236)
		for(int i = 0; i < BK_CSI_DEMO_GPIO_NUM; i++) {
			uint8_t gpio = gpio_array[i];
			if (color_gpio_map[current_color][i]) {
				GPIO_UP(gpio);
			} else {
				GPIO_DOWN(gpio);
			}
		}
#endif
		light_status = true;
	}

	if (xTimerReset(flicker_timer, pdMS_TO_TICKS(BK_CSI_DEMO_LIGHT_ON_DURATION)) != pdPASS) {
		os_printf("reset timer fail, check please\n");
	}
	return;
}

static void turn_on_light(uint8_t color, bool flicker) {
	if (color >= BK_CSI_DEMO_LIGHT_COLOR_NUM) {
		os_printf("invalid color,check please\n");
		return;
	}

	if (xTimerStop(flicker_timer, 0) != pdPASS) {
		os_printf("stop flick timer fail, check please\n");
		return;
	}
	down_all_gpio();
#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7236)
	for(int i = 0; i < BK_CSI_DEMO_GPIO_NUM; i++) {
		uint8_t gpio = gpio_array[i];
		if (color_gpio_map[color][i]) {
			GPIO_UP(gpio);
		} else {
			GPIO_DOWN(gpio);
		}
	}
#endif
	current_color = color;

	if (flicker) {
		if (flicker_timer != NULL) {
			if (xTimerStart(flicker_timer, 0) != pdPASS) {
				os_printf("start flicker timer fail\n");
				xTimerDelete(flicker_timer, 0);
			}
		} else {
			os_printf("something wrong with flicker timer, check please\n");
			return;
		}
	}
}

static void bk_csi_demo_task(void *pvParameter) {
	bk_csi_demo_event_t evt;

	while (rtos_pop_from_queue(&bk_csi_demo_queue, &evt, BEKEN_WAIT_FOREVER) == kNoErr) {
		switch (evt.id) {
			case BK_CSI_DEMO_TURN_ON_LIGHT_EVENT:
			{
				uint8_t color = evt.color;
				bool flicker  = evt.flicker;
				//os_printf("BK_CSI_DEMO_TURN_ON_LIGHT_EVENT color:%u, flicker:%u\n", color, flicker);
				turn_on_light(color, flicker);
				break;
			}
			case BK_CSI_DEMO_EXIT_EVENT:
				goto exit;

			default:
				os_printf("event error: %d\n", evt.id);
				goto exit;
		}
	}

exit:
	rtos_delete_thread(NULL);
}

static void bk_csi_demo_init() {
	flicker_timer = xTimerCreate(
		"flicker_timer",
		pdMS_TO_TICKS(BK_CSI_DEMO_LIGHT_ON_DURATION),
		pdFALSE,
		(void*)0,
		bk_csi_demo_light_flicker
	);
	if (flicker_timer == NULL) {
		os_printf("create flicker timer fail, now exit\n");
		return;
	}

	rtos_init_queue(&bk_csi_demo_queue, "bk_csi_demo_queue", sizeof(bk_csi_demo_event_t), BK_CSI_DEMO_QUEUE_SIZE);
	beken_thread_t thd;
	rtos_create_thread(&thd, BEKEN_APPLICATION_PRIORITY, "bk_csi_demo_task",
				(beken_thread_function_t)bk_csi_demo_task,
				2048, NULL);

	rtos_delay_milliseconds(1);
	turn_on_light(1, 0);
	light_status = true;
	current_color = 1;

	bk_csi_demo_started = true;
}

static void bk_csi_demo_deinit() {
	bk_csi_demo_event_t evt;
	evt.id = BK_CSI_DEMO_EXIT_EVENT;
	if (rtos_push_to_queue(&bk_csi_demo_queue, &evt, BEKEN_WAIT_FOREVER) != kNoErr) {
		os_printf("Exit bk csi demo failed!\n");
	}

	rtos_deinit_queue(&bk_csi_demo_queue);
	
	xTimerDelete(flicker_timer, 0);

	bk_csi_demo_started = false;
}

void bk_csi_demo_main() {
	if (bk_csi_demo_started) {
		os_printf("Demo already started!\n");
		return;
	}
    bk_csi_demo_init();
}

void bk_csi_demo_stop() {
	if (!bk_csi_demo_started) {
		os_printf("Demo already stopped!\n");
		return;
	}  
	bk_csi_demo_deinit();
}
