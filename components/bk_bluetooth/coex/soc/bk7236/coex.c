#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "coex.h"
#include "sys_hal.h"
#include "gpio_driver.h"
#include "timer.h"
#include <driver/gpio.h>
#include "coex_internal.h"

#define COEX_VERSION              0x00010000

extern void sys_hal_rf_ctrl(uint8_t type);
extern uint8_t sys_hal_rf_ctrl_type_get(void);

static void rf_select_wifi(void)
{
    sys_hal_rf_ctrl(1);
}

static void rf_select_bt(void)
{
    sys_hal_rf_ctrl(2);
}

static void rf_select_pta(void)
{
    sys_hal_rf_ctrl(0);
}

static uint8_t rf_get_type(void)
{
    return sys_hal_rf_ctrl_type_get();
}

static void wifi_pta_enable_set(bool en)
{

}

static uint32_t coex_get_time(void)
{
    return rtos_get_time();
}

static void gpio(uint32_t io,uint8_t type)
{
    if (type) {
        bk_gpio_set_output_high(io);
    } else {
        bk_gpio_set_output_low(io);
    }
}

static void gpio_init(uint32_t io)
{
    gpio_dev_unmap(io);
    bk_gpio_enable_output(io);
}

static void *malloc_wrapper(size_t size)
{
    return os_malloc(size);
}

static void free_wrapper(void *ptr)
{
    os_free(ptr);
}

static uint8_t timer_id = TIMER_ID1;

static void timer_start(uint32_t ms, void *cb)
{
    bk_timer_start(timer_id,ms,(timer_isr_t)cb);
}

static void timer_stop(void)
{
    bk_timer_stop(timer_id);
}

static uint32_t disable_int(void)
{
    return rtos_enter_critical();
}

static void enable_int(uint32_t int_level)
{
    rtos_exit_critical(int_level);
}

static struct coex_osi_funcs_t coex_osi_funcs =
{
    ._version = COEX_VERSION,
    ._log = bk_printf_ext,
    ._log_raw = bk_printf_raw,
    ._rf_select_wifi = rf_select_wifi,
    ._rf_select_bt = rf_select_bt,
    ._rf_select_pta = rf_select_pta,
    ._rf_get_type = rf_get_type,
    ._wifi_pta_enable_set = wifi_pta_enable_set,
    ._coex_get_time = coex_get_time,
    ._gpio = gpio,
    ._gpio_init = gpio_init,
    ._malloc = malloc_wrapper,
    ._free = free_wrapper,
    ._timer_start = timer_start,
    ._timer_stop = timer_stop,
    ._disable_int = disable_int,
    ._enable_int = enable_int,
};

void bk_coex_init(void)
{
    softcoex_init(&coex_osi_funcs);
}

void bk_coex_deinit(void)
{
    softcoex_deinit();
}

