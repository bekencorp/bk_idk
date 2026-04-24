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
#include <driver/pwm.h>
#include <os/os.h>

#define TAG "pwm_test"
#define PWM_CLOCK_SOURCE (26000000)
#define DUTY_MAX         (2047)

#define PWM_CH_1        PWM_ID_3

#define PWM_STEP        (69)
#define LOOP_CYCLE      (30)

static uint32_t s_period = PWM_CLOCK_SOURCE / 1000;

static void pwm_init_channel(pwm_chan_t channel)
{
	pwm_init_config_t init_config = {0};
	init_config.psc = 0,
	init_config.period_cycle = s_period;
	init_config.duty_cycle = 2;
	init_config.duty2_cycle = 0;
	init_config.duty3_cycle = 0;
	BK_LOG_ON_ERR(bk_pwm_init(channel, &init_config));
	BK_LOG_ON_ERR(bk_pwm_start(channel));
}

static void pwm_update_duty(pwm_chan_t channel, uint32_t duty)
{
	if (duty > DUTY_MAX) {
		duty = DUTY_MAX;
	}

	pwm_period_duty_config_t pwm_config = {
		.psc = 0,
		.period_cycle = s_period,
		.duty_cycle = duty * s_period / DUTY_MAX,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
	};

	BK_LOGD(TAG, "chan:%d period=%d duty=%d\n", channel, pwm_config.period_cycle, pwm_config.duty_cycle);
	BK_LOG_ON_ERR(bk_pwm_set_period_duty(channel, &pwm_config));
}

#if CONFIG_HTOL_RELIABILITY_TEST
#define PWM_BREATHING_LED_FREQ         (1000)
#define PWM_BREATHING_LED_CH_X         PWM_CH_1
int bk_pwm_breathing_led_self_test(void)
{
	s_period = PWM_CLOCK_SOURCE / PWM_BREATHING_LED_FREQ;
	BOOL pwm_flag = false;
	BK_LOG_ON_ERR(bk_pwm_driver_init());
	pwm_init_channel(PWM_BREATHING_LED_CH_X);

	while(1){
		for (uint32_t loop = 0; loop < LOOP_CYCLE; loop++) {
			if(pwm_flag)
				pwm_update_duty(PWM_BREATHING_LED_CH_X, (loop + 1) * PWM_STEP);
			else
				pwm_update_duty(PWM_BREATHING_LED_CH_X, (29 - loop) * PWM_STEP);
			rtos_delay_milliseconds(100);
		}
		rtos_delay_milliseconds(100);
		pwm_flag = !pwm_flag;
	}

	return 0;
}

extern bool htol_is_enabled(void);

static void bk_pwm_breathing_led_self_test_thread(void *arg)
{
	bk_pwm_breathing_led_self_test();
	rtos_delete_thread(NULL);
}

void start_bk_pwm_breathing_led_self_test_thread(void)
{
	if (!htol_is_enabled()) {
		return;
	}
	BK_LOGI(TAG, "start pwm_breathing_led thread.\r\n");
	rtos_create_thread(
		NULL,
		BEKEN_APPLICATION_PRIORITY,
		"breathing_led",
		(beken_thread_function_t)bk_pwm_breathing_led_self_test_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}
#endif
