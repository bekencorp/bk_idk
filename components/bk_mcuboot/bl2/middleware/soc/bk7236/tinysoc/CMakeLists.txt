set(SOC_PATH "${TINYSOC_PATH}/bk7236")

set(incs
	${SOC_PATH}/
	${SOC_PATH}/config/
	${SOC_PATH}/soc/
	${TINYSOC_PATH}/driver/
	${TINYSOC_PATH}/driver
	${SOC_PATH}/driver/include
	${SOC_PATH}/hal
	${TINYSOC_PATH}/driver/uart
	${TINYSOC_PATH}/driver/gpio
	${TINYSOC_PATH}/driver/sys_ctrl
	${TINYSOC_PATH}/driver/flash
	${TINYSOC_PATH}/driver/spinlock
	${TINYSOC_PATH}/driver/timer
	${TINYSOC_PATH}/driver/efuse
	${TINYSOC_PATH}/driver/log
	${TINYSOC_PATH}/driver/mpc
	${TINYSOC_PATH}/driver/cmsis_drivers
	${TINYSOC_PATH}/driver/nv_otp
	${TINYSOC_PATH}/driver/platform_include
	${TINYSOC_PATH}/include/
)

set(srcs
	${TINYSOC_PATH}/driver/port/os_port.c
	${TINYSOC_PATH}/driver/port/mem_port.c
	${TINYSOC_PATH}/driver/sys_ctrl/sys_driver.c
	${TINYSOC_PATH}/driver/sys_ctrl/sys_driver_common.c
	${TINYSOC_PATH}/driver/sys_ctrl/sys_clock_driver.c
	${TINYSOC_PATH}/driver/sys_ctrl/sys_int_driver.c
	${TINYSOC_PATH}/driver/uart/uart_driver.c
	${TINYSOC_PATH}/driver/uart/uart_statis.c
	${TINYSOC_PATH}/driver/gpio/gpio_driver_base.c
	${TINYSOC_PATH}/driver/gpio/gpio_driver.c
	${TINYSOC_PATH}/driver/spinlock/amp_res_lock.c
	${TINYSOC_PATH}/driver/flash/flash_driver.c
	${TINYSOC_PATH}/driver/flash/flash_partition.c
	${TINYSOC_PATH}/driver/timer/timer_driver.c
	${TINYSOC_PATH}/driver/ppc/prro_driver.c
	${TINYSOC_PATH}/driver/efuse/efuse_driver.c
	${TINYSOC_PATH}/driver/efuse/bk_efuse.c
	${TINYSOC_PATH}/driver/jtag/hal_jtag.c
	${TINYSOC_PATH}/driver/nv_otp/rotpk.c
	${TINYSOC_PATH}/driver/nv_otp/otp_flash.c
	$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_NV_COUNTERS}>>:${TINYSOC_PATH}/driver/nv_otp/nv_counters.c>
	$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_NV_COUNTERS}>>:${TINYSOC_PATH}/driver/nv_otp/nv_counters.c>
	${TINYSOC_PATH}/driver/cmsis_drivers/Driver_Flash.c
	${TINYSOC_PATH}/driver/cmsis_drivers/Driver_USART.c
	${TINYSOC_PATH}/driver/native_drivers/arm_uart_drv.c
	${TINYSOC_PATH}/driver/uart/uart_stdout.c
	${SOC_PATH}/hal/prro_hal_v5.c
	${SOC_PATH}/hal/uart_hal.c
	${SOC_PATH}/hal/timer_hal.c
	${SOC_PATH}/hal/sys_hal.c
	${SOC_PATH}/hal/aon_pmu_hal.c
	${SOC_PATH}/hal/gpio_hal.c
	${SOC_PATH}/hal/uart_hal_debug.c
	${SOC_PATH}/hal/gpio_hal_debug.c
	${SOC_PATH}/hal/flash_hal_debug.c
	${SOC_PATH}/hal/flash_hal.c
	${SOC_PATH}/hal/uart_ll.c
	${SOC_PATH}/hal/timer_ll.c
	${SOC_PATH}/hal/gpio_ll.c
	${SOC_PATH}/hal/flash_ll.c
	${SOC_PATH}/hal/efuse_hal.c
)

target_compile_definitions(platform_bl2 PUBLIC CONFIG_TFM_SOC_PATH_V5)

set(bl2_incs driver)
target_include_directories(platform_bl2 PUBLIC ${incs} ${bl2_incs})
target_sources(platform_bl2 PRIVATE ${srcs} ${bl2_srcs})
target_compile_options(platform_bl2 PUBLIC ${BL2_COMPILER_CP_FLAG})
