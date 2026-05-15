#include "cli.h"
#include <os/os.h>
#include <driver/pwm.h>
#include "pwm_hal.h"
#include <driver/trng.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#if CONFIG_PWM

#ifndef PWM_CLOCK_SRC_XTAL
#define PWM_CLOCK_SRC_XTAL 26000000
#endif

#if CONFIG_PWM_SCLK_80M
#define PWM_CLOCK_SRC_FREQ 80000000
#else
#define PWM_CLOCK_SRC_FREQ PWM_CLOCK_SRC_XTAL
#endif

#define _PERIOD_2_FREQ(period)    ((period == 0) ? (0) : (PWM_CLOCK_SRC_FREQ / (period)))

static void cli_pwm_help(void)
{
	CLI_LOGI("pwm_driver init\n");
	CLI_LOGI("pwm_driver deinit\n");
	CLI_LOGI("pwm {chan} init {period_v} {duty_v} [duty1_v][duty2_v][duty3_v]\n");
	CLI_LOGI("pwm {chan} {start|stop|deinit}\n");
	CLI_LOGI("pwm {chan} duty {period_v} {duty1_v} [duty2_v] [duty3_v]\n");
	CLI_LOGI("pwm {chan} signal {low|high}\n");
	CLI_LOGI("pwm_group init {chan1} {chan2} {period} {chan1_duty} {chan2_duty}\n");
	CLI_LOGI("pwm_group {start|stop|deinit}\n");
	CLI_LOGI("pwm_group config {group} {period} {chan1_duty} {chan2_duty}\n");
	CLI_LOGI("pwm_int {chan} {reg|enable|disable}\n");
	CLI_LOGI("pwm_capture {chan} {init}\n");
	CLI_LOGI("pwm_capture {chan} {start|stop|deinit}\n");
	CLI_LOGI("pwm_idle_test {idle_start|idle_stop}\n");
}

static void cli_pwm_timer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_counter_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_carrier_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		ret = bk_pwm_driver_init();
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm driver init failed, ret=%#x\n", ret);
			return;
		}
		CLI_LOGI("pwm init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_pwm_driver_deinit();
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm driver deinit failed, ret=%#x\n", ret);
			return;
		}
		CLI_LOGI("pwm deinit\n");
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;
	bk_err_t ret = BK_OK;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		pwm_init_config_t config = {0};

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		config.psc = os_strtoul(argv[7], NULL, 10);

		ret = bk_pwm_init(chan, &config);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm init failed, chan=%d period=%x duty=%x ret=%#x\n",
				chan, config.period_cycle, config.duty_cycle, ret);
			return;
		}
		CLI_LOGI("pwm init, chan=%d period=%x duty=%x\n", chan, config.period_cycle, config.duty_cycle);
	} else if (os_strcmp(argv[2], "start") == 0) {
		ret = bk_pwm_start(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm start failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		ret = bk_pwm_stop(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm stop failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		ret = bk_pwm_deinit(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm deinit failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "signal") == 0) {
		if (argc != 4) {
			cli_pwm_help();
			return;
		}

		if (os_strcmp(argv[3], "low") == 0)
			ret = bk_pwm_set_init_signal_low(chan);
		else
			ret = bk_pwm_set_init_signal_high(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm set signal failed, chan=%d signal=%s ret=%#x\n", chan, argv[3], ret);
			return;
		}
		CLI_LOGI("pwm set signal, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "duty") == 0) {
		pwm_period_duty_config_t config = {0};
		if (argc < 5) {
			cli_pwm_help();
			return;
		}

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		ret = bk_pwm_set_period_duty(chan, &config);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm duty failed, chan=%d period=%d t1=%d t2=%d t3=%d ret=%#x\n",
				chan, config.period_cycle, config.duty_cycle, config.duty2_cycle, config.duty3_cycle, ret);
			return;
		}
		CLI_LOGI("pwm duty, chan=%d period=%d t1=%d t2=%d t3=%d\n", chan, config.period_cycle,
				 config.duty_cycle, config.duty2_cycle, config.duty3_cycle);
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_group_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t group;
	bk_err_t ret = BK_OK;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	group = os_strtoul(argv[2], NULL, 10);

	if (os_strcmp(argv[1], "init") == 0) {
		if (argc != 7) {
			cli_pwm_help();
			return;
		}
		pwm_group_init_config_t config = {0};
		pwm_group_t group = 0;

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		config.period_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[5], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[6], NULL, 10);
		ret = bk_pwm_group_init(&config, &group);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm group init failed, chan1=%d chan2=%d period=%x d1=%x d2=%x ret=%#x\n",
				config.chan1, config.chan2, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle, ret);
			return;
		}
		CLI_LOGI("pwm init, group=%d chan1=%d chan2=%d period=%x d1=%x d2=%x\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_pwm_group_start(group);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm group start failed, group=%d ret=%#x\n", group, ret);
			return;
		}
		CLI_LOGI("pwm start, group=%d\n", group);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_pwm_group_stop(group);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm group stop failed, group=%d ret=%#x\n", group, ret);
			return;
		}
		CLI_LOGI("pwm stop, group=%d\n", group);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_pwm_group_deinit(group);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm group deinit failed, group=%d ret=%#x\n", group, ret);
			return;
		}
		CLI_LOGI("pwm deinit, group=%d\n", group);
	} else if (os_strcmp(argv[1], "config") == 0) {
		pwm_group_config_t config = {0};
		if (argc != 6) {
			cli_pwm_help();
			return;
		}

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[5], NULL, 10);
		ret = bk_pwm_group_set_config(group, &config);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm group config failed, group=%x period=%x chan1_duty=%x chan2_duty=%x ret=%#x\n",
				group, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle, ret);
			return;
		}
		CLI_LOGI("pwm config, group=%x period=%x chan1_duty=%x chan2_duty=%x\n",
				 group, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else {
		cli_pwm_help();
		return;
	}
}


static void cli_pwm_isr(pwm_chan_t chan)
{
	CLI_LOGI("isr(%d)\n", chan);
}

static void cli_pwm_capture_isr(pwm_chan_t chan)
{
	CLI_LOGI("capture(%d), value=%x\n", chan, bk_pwm_capture_get_value(chan));
}

static void cli_pwm_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;
	bk_err_t ret = BK_OK;

	if (argc != 3) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "reg") == 0) {
		ret = bk_pwm_register_isr(chan, cli_pwm_isr);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm chan%d register interrupt isr failed, ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm chan%d register interrupt isr\n", chan);
	} else if (os_strcmp(argv[2], "enable") == 0) {
		ret = bk_pwm_enable_interrupt(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm chan%d enable interrupt failed, ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm chan%d enable interrupt\n", chan);
	} else if (os_strcmp(argv[2], "disable") == 0) {
		ret = bk_pwm_disable_interrupt(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm chan%d disable interrupt failed, ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm chan%d disable interrupt\n", chan);
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_capture_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;
	bk_err_t ret = BK_OK;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 6);
		pwm_capture_init_config_t config = {0};
		if (os_strcmp(argv[3], "pos") == 0)
			config.edge = PWM_CAPTURE_POS;
		else if (os_strcmp(argv[3], "neg") == 0)
			config.edge = PWM_CAPTURE_NEG;
		else if (os_strcmp(argv[3], "edge") == 0)
			config.edge = PWM_CAPTURE_EDGE;
		else {
			cli_pwm_help();
			return;
		}

		if (os_strcmp(argv[4], "count") == 0)
			config.capture_mode = PWM_CAPTURE_MODE_PULSE_COUNT;
		else
			config.capture_mode = PWM_CAPTURE_MODE_PULSE_WIDTH;

		config.pulse_count_threshold = os_strtoul(argv[5], NULL, 10);
		config.isr = cli_pwm_capture_isr;
		ret = bk_pwm_capture_init(chan, &config);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm_capture init failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm_capture init, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "start") == 0) {
		ret = bk_pwm_capture_start(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm_capture start failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm_capture start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		ret = bk_pwm_capture_stop(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm_capture stop failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm_capture stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		ret = bk_pwm_capture_deinit(chan);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm_capture deinit failed, chan=%d ret=%#x\n", chan, ret);
			return;
		}
		CLI_LOGI("pwm_capture deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "get_value") == 0) {
		CLI_LOGI("capture(%d), value=%x\n", chan, bk_pwm_capture_get_value(chan));
	} else if (os_strcmp(argv[2], "capture_example") == 0) {
		pwm_capture_init_config_t config = {0};
		float duty_ratio = 0;

		config.edge = PWM_CAPTURE_POS;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));

		uint32_t period_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		CLI_LOGI("pwm_capture period_cycle:%d, freq:%dHz\n", period_cycle, _PERIOD_2_FREQ(period_cycle));
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));

		config.edge = PWM_CAPTURE_EDGE;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));
		uint32_t duty_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		if (period_cycle == 0) {
			duty_ratio = 0;
		} else {
			duty_ratio = (float)duty_cycle / (float)period_cycle;
		}
		CLI_LOGI("pwm_capture duty_cycle:%d, duty_ratio:%f\r\n", duty_cycle, duty_ratio);
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
	} else {
		cli_pwm_help();
		return;
	}
}

#define PWM_FREQ           (16000)
static uint32_t s_period_cycle = PWM_CLOCK_SRC_FREQ / PWM_FREQ;

static void pwm_init_and_start_all_chan_test(uint32_t period_cycle, uint32_t duty_cycle)
{
	BK_LOG_ON_ERR(bk_pwm_driver_init());

	pwm_init_config_t init_config = {0};

	for(pwm_chan_t chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
		init_config.period_cycle = period_cycle;
		init_config.duty_cycle = duty_cycle;
		BK_LOG_ON_ERR(bk_pwm_init(chan, &init_config));
		BK_LOG_ON_ERR(bk_pwm_start(chan));
	}
	s_period_cycle = period_cycle;
	rtos_delay_milliseconds(1000);

	CLI_LOGI("pwm_init_and_start period=%d duty=%d\r\n", init_config.period_cycle, init_config.duty_cycle);
}

static void pwm_update_all_chan_test(void)
{
	pwm_period_duty_config_t pwm_config = {0};
	uint32_t pwm_step = s_period_cycle / 10;
	uint32_t i = 0;
	uint32_t j = 0;
	pwm_chan_t chan = 0;

	for (j = 0; j < 5; j++) {
		CLI_LOGI("turn on\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * i;
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
			chan = 0;
		}

		CLI_LOGI("turn off\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * (10 - i);
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
		}
	}
}

#if CONFIG_PWM_PHASE_SHIFT
#if CONFIG_PWM_V1PX
#define PWM_CH_0        PWM_ID_1
#define PWM_CH_1        PWM_ID_3
#define PWM_CH_2        PWM_ID_4
#define PWM_CH_3        PWM_ID_6
#define PWM_CH_4        PWM_ID_8
#define PWM_CH_5        PWM_ID_10
#else
#define PWM_CH_0        PWM_ID_0
#define PWM_CH_1        PWM_ID_1
#define PWM_CH_2        PWM_ID_2
#define PWM_CH_3        PWM_ID_3
#define PWM_CH_4        PWM_ID_4
#define PWM_CH_5        PWM_ID_5
#endif

#define PHASE_SHIFT_CONFIG { \
	.psc = 0, \
	.chan_num = 6,\
	.period_cycle = s_period_cycle, \
	.duty_config[0] = { \
		.chan = PWM_CH_0, \
		.duty_cycle = 0, \
	}, \
	.duty_config[1] = { \
		.chan = PWM_CH_1, \
 		.duty_cycle = 0, \
	}, \
	.duty_config[2] = { \
		.chan = PWM_CH_2, \
		.duty_cycle = 0, \
	}, \
	.duty_config[3] = { \
		.chan = PWM_CH_3, \
		.duty_cycle = 0, \
	}, \
	.duty_config[4] = { \
		.chan = PWM_CH_4, \
		.duty_cycle = 0, \
	}, \
	.duty_config[5] = { \
		.chan = PWM_CH_5, \
		.duty_cycle = 0, \
	}, \
} \

static int pwm_phase_shift_test(void)
{
	bk_err_t ret = BK_OK;
	pwm_phase_shift_config_t config = PHASE_SHIFT_CONFIG;

	ret = bk_pwm_driver_init();
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) {
		CLI_LOGE("phase_shift driver init failed, ret=%#x\r\n", ret);
		return ret;
	}

	ret = bk_pwm_phase_shift_init(&config);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) {
		CLI_LOGE("phase_shift init failed, ret=%#x\r\n", ret);
		return ret;
	}

	ret = bk_pwm_phase_shift_start();
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) {
		CLI_LOGE("phase_shift start failed, ret=%#x\r\n", ret);
		return ret;
	}

	uint32_t rand_num = 0;
	time_t time_num;

	srand((unsigned)time(&time_num));

	for (int j = 0; j < 10; j++) {
		CLI_LOGI("phase_shift test_num:%d\r\n", j);
		for (int i = 0; i < config.chan_num; i++) {
			rand_num = rand() % 11;
			CLI_LOGI("phase_shift rand_num:%d\r\n", rand_num);
			ret = bk_pwm_phase_shift_set_duty(config.duty_config[i].chan, s_period_cycle * 0.1f * rand_num);
			BK_LOG_ON_ERR(ret);
			if (ret != BK_OK) {
				CLI_LOGE("phase_shift set duty failed, chan=%d ret=%#x\r\n", config.duty_config[i].chan, ret);
				return ret;
			}
		}
		ret = bk_pwm_phase_shift_update_duty();
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("phase_shift update duty failed, ret=%#x\r\n", ret);
			return ret;
		}
		rtos_delay_milliseconds(10);
		os_printf("\r\n");
	}

	ret = bk_pwm_phase_shift_set_duty(PWM_CH_0, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_set_duty(PWM_CH_1, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_set_duty(PWM_CH_2, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_set_duty(PWM_CH_3, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_set_duty(PWM_CH_4, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_set_duty(PWM_CH_5, 0);
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) return ret;
	ret = bk_pwm_phase_shift_update_duty();
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) {
		CLI_LOGE("phase_shift finalize update failed, ret=%#x\r\n", ret);
		return ret;
	}

	CLI_LOGI("phase_shift test over\r\n");

	return 0;
}
#endif

#if CONFIG_PWM_FADE
static void cli_pwm_fade_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 6) {
		cli_pwm_help();
		return;
	}

	pwm_fade_mode_t fade_mode = 0;

	if (os_strcmp(argv[1], "inc") == 0) {
		fade_mode = PWM_DUTY_DIR_INCREASE;
	} else {
		fade_mode = PWM_DUTY_DIR_DECREASE;
	}

	uint32_t chan = os_strtoul(argv[2], NULL, 10);
	uint32_t fade_scale = os_strtoul(argv[3], NULL, 10);
	uint32_t fade_intv_cycle = os_strtoul(argv[4], NULL, 10);
	uint32_t fade_num = os_strtoul(argv[5], NULL, 10);

	// bk_pwm_fade_init(0, 100, 1000, 10);
	bk_pwm_fade_init(chan, fade_scale, fade_intv_cycle, fade_num);
	bk_pwm_fade_start(chan, fade_mode);

	CLI_LOGI("pwm fade test done\r\n");
}
#endif

static void cli_pwm_idle_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	pwm_group_t group = 0;

	if (os_strcmp(argv[1], "idle_init") == 0) {
		if (argc < 4) {
			cli_pwm_help();
			return;
		}
		uint32_t freq = os_strtoul(argv[2], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_FREQ / freq;
		uint32_t duty_cycle = period_cycle * os_strtoul(argv[3], NULL, 10) / 100;

		// duty_cycle should test: 0, period_cycle, 1, period_cycle - 1
		CLI_LOGI("period_cycle=%d duty_cycle=%d\r\n", period_cycle, duty_cycle);
		pwm_init_and_start_all_chan_test(period_cycle, duty_cycle);
		CLI_LOGI("pwm idle_init done\r\n");
	} else if (os_strcmp(argv[1], "idle_start") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				BK_LOG_ON_ERR(bk_pwm_start(chan));
		}
	} else if (os_strcmp(argv[1], "idle_stop") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
			BK_LOG_ON_ERR(bk_pwm_stop(chan));
		}
	} else if (os_strcmp(argv[1], "idle_update") == 0) {
		pwm_update_all_chan_test();
	} else if (os_strcmp(argv[1], "group_init") == 0) {
		if (argc < 5) {
			cli_pwm_help();
			return;
		}
		ret = bk_pwm_driver_init();
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm driver init failed in idle test, ret=%#x\n", ret);
			return;
		}
		pwm_group_init_config_t config = {0};

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		uint32_t freq = os_strtoul(argv[4], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_FREQ / freq;

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * os_strtoul(argv[5], NULL, 10) / 100;
		config.chan2_duty_cycle = period_cycle * os_strtoul(argv[6], NULL, 10) / 100;
		ret = bk_pwm_group_init(&config, &group);
		BK_LOG_ON_ERR(ret);
		if (ret != BK_OK) {
			CLI_LOGE("pwm idle group init failed, chan1=%d chan2=%d period=%d d1=%d d2=%d ret=%#x\n",
				config.chan1, config.chan2, config.period_cycle,
				config.chan1_duty_cycle, config.chan2_duty_cycle, ret);
			return;
		}

		CLI_LOGI("pwm group=%d chan1=%d chan2=%d period=%d d1=%d d2=%d\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "group_start") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_start(start_group));
	} else if (os_strcmp(argv[1], "group_stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_stop(group));
	} else if (os_strcmp(argv[1], "group_update") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		uint32_t freq = os_strtoul(argv[3], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_FREQ / freq;

		pwm_group_config_t config = {0};

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.3f;
		config.chan2_duty_cycle = period_cycle * 0.3f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
	}
#if CONFIG_PWM_PHASE_SHIFT
	else if(os_strcmp(argv[1], "phase_shift") == 0) {
		pwm_phase_shift_test();
	}
#endif
	else {
		cli_pwm_help();
		return;
	}
}

#define PWM_CMD_CNT (sizeof(s_pwm_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_pwm_commands[] = {
	{"pwm_driver", "{init|deinit}}", cli_pwm_driver_cmd},
	{"pwm", "pwm {chan} {config|start|stop|init|deinit|signal} [...]", cli_pwm_cmd},
	{"pwm_int", "pwm_int {chan} {reg|enable|disable}", cli_pwm_int_cmd},
	//{"pwm_duty", "pwm_duty {chan} {period} {d1} [d2] [d3]", cli_pwm_cmd},
	{"pwm_capture", "pwm_capture {chan} {config|start|stop|init|deinit}", cli_pwm_capture_cmd},
	{"pwm_group", "pwm_group {init|deinit|config|start|stop} [...]", cli_pwm_group_cmd},
	{"pwm_timer", "pwm_timer ", cli_pwm_timer_cmd},
	{"pwm_counter", "pwm_counter", cli_pwm_counter_cmd},
	{"pwm_carrier", "pwm_carrier", cli_pwm_carrier_cmd},
	{"pwm_idle_test", "{idle_init|idle_start|idle_stop|phase_shift}", cli_pwm_idle_test_cmd},
#if CONFIG_PWM_FADE
	{"pwm_fade", "pwm_fade", cli_pwm_fade_cmd},
#endif
};

int cli_pwm_init(void)
{
	bk_err_t ret = bk_pwm_driver_init();
	BK_LOG_ON_ERR(ret);
	if (ret != BK_OK) {
		CLI_LOGE("pwm driver init failed during cli init, ret=%#x\n", ret);
		return ret;
	}
	return cli_register_commands(s_pwm_commands, PWM_CMD_CNT);
}

#endif
