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
#include <bk_wifi_adapter.h>
#include <bk_phy_internal.h>
#ifdef CONFIG_USE_MBEDTLS
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>
#else
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/ecc.h>
#endif
#include <crypto/sha1.h>
#include <crypto/aes_wrap.h>
#include <crypto/crypto.h>
#include <crypto/md5.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha384.h>
#include <crypto/sha512.h>
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
#include "bk_wifi.h"
#include "bk_net.h"
#include "bk_misc.h"
#include <sys_ll.h>
#include "sys_driver.h"
#include "gpio_driver.h"
#include "bk_ps.h"
#include <modules/pm.h>
#include "bk_mcu_ps.h"
#include "bk_feature.h"
#include "bk_general_dma.h"
#include "bk_ps_time.h"
#include "bk_rf_internal.h"
#include <components/ate.h>
#include <driver/aon_rtc.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "lwip/sockets.h"
#include "cache.h"
#include "arch_interrupt.h"
#include "stack_base.h"
#include "command_line.h"
#include "bk_phy_adapter.h"
#if (CONFIG_CKMN)
#include <driver/ckmn.h>
#endif

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define TAG "wifi_adapter"

#ifdef CONFIG_USE_MBEDTLS
struct crypto_ecdh {
    mbedtls_ecdh_context ctx;
    size_t length;
};
#else
struct crypto_ec {
	ecc_key key;
	mp_int a;
	mp_int prime;
	mp_int order;
	mp_digit mont_b;
	mp_int b;
};
struct crypto_ecdh {
	struct crypto_ec *ec;
	WC_RNG rng;
};
#endif

static void _crypto_ecdh_deinit_wrapper(void *ecdh) {
	crypto_ecdh_deinit((struct crypto_ecdh *)ecdh);
}

static void * _crypto_ecdh_init_wrapper(int group) {
	return (struct crypto_ecdh *)crypto_ecdh_init(group);
}

static struct wpabuf * _crypto_ecdh_set_peerkey_wrapper(void *ecdh, int inc_y, const u8 *key, size_t len) {
	return crypto_ecdh_set_peerkey((struct crypto_ecdh *)ecdh, inc_y, key, len);
}

static struct wpabuf * _crypto_ecdh_get_pubkey_wrapper(void *ecdh, int inc_y) {
	return crypto_ecdh_get_pubkey((struct crypto_ecdh *)ecdh, inc_y);
}

void * _crypto_bignum_init_wrapper(void) {
	return (struct crypto_bignum *)crypto_bignum_init();
}

int _crypto_bignum_rand_wrapper(void *r, const void *m) {
	return crypto_bignum_rand((struct crypto_bignum *)r, (const struct crypto_bignum *)m);
}

void _crypto_bignum_deinit_wrapper(void *n, int clear) {
	return crypto_bignum_deinit((struct crypto_bignum *)n, clear);
}

void * _crypto_bignum_init_set_wrapper(const u8 *buf, size_t len) {
	return (struct crypto_bignum *)crypto_bignum_init_set(buf, len);
}

void * _crypto_bignum_init_uint_wrapper(unsigned int val) {
	return (struct crypto_bignum *)crypto_bignum_init_uint(val);
}

int _crypto_bignum_to_bin_wrapper(const void *a, u8 *buf, size_t buflen, size_t padlen) {
	return crypto_bignum_to_bin((struct crypto_bignum *)a, buf, buflen, padlen);
}

int _crypto_bignum_add_wrapper(const void *a, const void *b, void *c) {
	return crypto_bignum_add((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (struct crypto_bignum *)c);
}

int _crypto_bignum_sub_wrapper(const void *a, const void *b, void *c) {
	return crypto_bignum_sub((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (struct crypto_bignum *)c);
}

int _crypto_bignum_mod_wrapper(const void *a, const void *b, void *c) {
	return crypto_bignum_mod((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (struct crypto_bignum *)c);
}

int _crypto_bignum_cmp_wrapper(const void *a, const void *b) {
	return crypto_bignum_cmp((const struct crypto_bignum *)a, (const struct crypto_bignum *)b);
}

int _crypto_bignum_mulmod_wrapper(const void *a, const void *b, const void *c, void *d){
	return crypto_bignum_mulmod((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (const struct crypto_bignum *)c,
					  (struct crypto_bignum *)d);
}

int _crypto_bignum_inverse_wrapper(const void *a, const void *b, void *c){
	return crypto_bignum_inverse((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (struct crypto_bignum *)c);
}

int _crypto_bignum_addmod_wrapper(const void *a, const void *b, const void *c, void *d){
	return crypto_bignum_addmod((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (const struct crypto_bignum *)c,
					  (struct crypto_bignum *)d);
}

int _crypto_bignum_sqrmod_wrapper(const void *a, const void *b, void *c){
	return crypto_bignum_sqrmod((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (struct crypto_bignum *)c);
}

int _crypto_bignum_exptmod_wrapper(const void *a, const void *b, const void *c, void *d){
	return crypto_bignum_exptmod((const struct crypto_bignum *)a,
					  (const struct crypto_bignum *)b,
					  (const struct crypto_bignum *)c,
					  (struct crypto_bignum *)d);
}

int _crypto_bignum_is_zero_wrapper(const void *a) {
	return crypto_bignum_is_zero((const struct crypto_bignum *)a);
}

int _crypto_bignum_is_one_wrapper(const void *a) {
	return crypto_bignum_is_one((const struct crypto_bignum *)a);
}

int _crypto_bignum_is_odd_wrapper(const void *a) {
	return crypto_bignum_is_odd((const struct crypto_bignum *)a);
}

int _crypto_bignum_rshift_wrapper(const void *a, int n, void *r) {
	return crypto_bignum_rshift((const struct crypto_bignum *)a, n, (struct crypto_bignum *)r);
}

int _crypto_bignum_legendre_wrapper(const void *a, const void *p) {
	return crypto_bignum_legendre((const struct crypto_bignum *)a, (const struct crypto_bignum *)p);
}

#ifdef CONFIG_USE_MBEDTLS
struct crypto_ec {
	mbedtls_ecp_group group;
};
#endif

static void * _crypto_ec_init_wrapper(int group) {
	return (struct crypto_ec *)crypto_ec_init(group);
}

static void _crypto_ec_deinit_wrapper(void *e) {
	crypto_ec_deinit((struct crypto_ec *)e);
}

static void * _crypto_ec_point_init_wrapper(void *e) {
	return (struct crypto_ec_point *)crypto_ec_point_init((struct crypto_ec *)e);
}

static void _crypto_ec_point_deinit_wrapper(void *p, int clear) {
	crypto_ec_point_deinit((struct crypto_ec_point *)p, clear);
}

static size_t _crypto_ec_prime_len_wrapper(void *e) {
	return crypto_ec_prime_len((struct crypto_ec *)e);
}

static const void *_crypto_ec_get_prime_wrapper(void *e) {
	return (const struct crypto_bignum *)crypto_ec_get_prime((struct crypto_ec *)e);
}

static const void *_crypto_ec_get_order_wrapper(void *e) {
	return (const struct crypto_bignum *)crypto_ec_get_order((struct crypto_ec *)e);
}

static size_t _crypto_ec_order_len_wrapper(void *e) {
	return crypto_ec_order_len((struct crypto_ec *)e);
}

static int _crypto_ec_point_mul_wrapper(void *e, const void *p, const void *b, void *res) {
	return crypto_ec_point_mul((struct crypto_ec *)e, (const struct crypto_ec_point *)p,
		(const struct crypto_bignum *)b, (struct crypto_ec_point *)res);
}

static int _crypto_ec_point_to_bin_wrapper(void *e, const void *point, u8 *x, u8 *y) {
	return crypto_ec_point_to_bin((struct crypto_ec *)e, (const struct crypto_ec_point *)point, x, y);
}

static void * _crypto_ec_point_from_bin_wrapper(void *e, const u8 *val) {
	return (struct crypto_ec_point *)crypto_ec_point_from_bin((struct crypto_ec *)e, val);
}

static int _crypto_ec_point_add_wrapper(void *e, const void *a, const void *b, void *c) {
	return crypto_ec_point_add((struct crypto_ec *)e, (const struct crypto_ec_point *)a,
		(const struct crypto_ec_point *)b, (struct crypto_ec_point *)c);
}

static int _crypto_ec_point_invert_wrapper(void *e, void *point) {
	return crypto_ec_point_invert((struct crypto_ec *)e, (struct crypto_ec_point *)point);
}

static int _crypto_ec_point_cmp_wrapper(const void *e, const void *a, const void *b) {
	return crypto_ec_point_cmp((const struct crypto_ec *)e, (const struct crypto_ec_point *)a,
		(const struct crypto_ec_point *)b);
}

static const void *_crypto_ec_get_a_wrapper(void *e) {
	return (const struct crypto_bignum *)crypto_ec_get_a((struct crypto_ec *)e);
}

static const void *_crypto_ec_get_b_wrapper(void *e) {
	return (const struct crypto_bignum *)crypto_ec_get_b((struct crypto_ec *)e);
}

static void *_crypto_ec_point_compute_y_sqr_wrapper(void *e, const void *x) {
	return (struct crypto_bignum *)crypto_ec_point_compute_y_sqr((struct crypto_ec *)e, (const struct crypto_bignum *)x);
}

static int _crypto_ec_point_is_at_infinity_wrapper(void *e, void *point) {
	return crypto_ec_point_is_at_infinity((struct crypto_ec *)e, (struct crypto_ec_point *)point);
}

static int _crypto_ec_point_is_on_curve_wrapper(void *e, void *point) {
	return crypto_ec_point_is_on_curve((struct crypto_ec *)e, (struct crypto_ec_point *)point);
}

static size_t _crypto_ec_prime_len_bits_wrapper(void *e) {
	return crypto_ec_prime_len_bits((struct crypto_ec *)e);
}

void *
bk_pbuf_alloc_wrapper(int layer, uint16_t length, int type)
{
	pbuf_layer layer_tmp = 0;
	pbuf_type type_tmp = 0;

	switch (layer) {
		case BK_PBUF_RAW_TX:
			layer_tmp = PBUF_RAW_TX;
			break;
		case BK_PBUF_RAW:
			layer_tmp = PBUF_RAW;
			break;
		default: return NULL;
	}

	switch (type) {
		case BK_PBUF_RAM:
			type_tmp = PBUF_RAM;
			break;
		case BK_PBUF_RAM_RX:
			type_tmp = PBUF_RAM_RX;
			break;
		case BK_PBUF_POOL:
			type_tmp = PBUF_POOL;
			break;
		default: return NULL;
	}
	return pbuf_alloc(layer_tmp, length, type_tmp);
}

void bk_pbuf_free_wrapper(void *p)
{
	pbuf_free((struct pbuf *)p);
}

void bk_pbuf_ref_wrapper(void *p)
{
	pbuf_ref((struct pbuf*)p);
}

void bk_pbuf_header_wrapper(void *p, int16_t len)
{
	pbuf_header((struct pbuf *)p, len);
}

void *bk_pbuf_coalesce_wrapper(void *p)
{
	return pbuf_coalesce(p, PBUF_RAW);
}

void bk_pbuf_cat_wrapper(void *p, void *q)
{
	pbuf_cat(p, q);
}

int bk_get_rx_pbuf_type_wrapper()
{
#if MEM_TRX_DYNAMIC_EN
	return BK_PBUF_RAM_RX;
#else
	return BK_PBUF_POOL;
#endif
}

int bk_get_pbuf_pool_size_wrapper()
{
#if MEM_TRX_DYNAMIC_EN
	return 0;
#else
	return PBUF_POOL_BUFSIZE;
#endif
}

void *bk_get_netif_hostname_wrapper(void *netif)
{
	const char*  hostname = ((struct netif *)netif)->hostname;
	return (void *)hostname;
}

int bk_save_net_info_wrapper(int item, u8 *ptr0, u8 *ptr1, u8 *ptr2)
{
	return save_net_info(item,ptr0,ptr1,ptr2);
}

int bk_get_net_info_wrapper(int item, u8 *ptr0, u8 *ptr1, u8 *ptr2)
{
	return get_net_info(item,ptr0,ptr1,ptr2);
}

void *bk_inet_ntoa_wrapper(void *addr)
{
	return (UINT8 *)inet_ntoa(addr);
}

uint32_t bk_lwip_ntohl_wrapper(uint32_t val)
{
	return lwip_ntohl(val);
}

uint16_t bk_lwip_htons_wrapper(uint16_t val)
{
	return lwip_htons(val);
}

void bk_sta_start()
{
	sta_ip_start();
}

void bk_sta_ip_down_wrapper()
{
	sta_ip_down();
}

void bk_uap_ip_start_wrapper()
{
	uap_ip_start();
}

void bk_uap_ip_down_wrapper()
{
	uap_ip_down();
}

int bk_net_wlan_add_netif_wrapper(void *mac)
{
	return net_wlan_add_netif(mac);
}

int bk_net_wlan_remove_netif_wrapper(void *mac)
{
	return net_wlan_remove_netif(mac);
}

extern uint8_t* dhcp_lookup_mac(uint8_t *chaddr);
uint32_t bk_lookup_ipaddr_wrapper(void *addr)
{
	char *ipstr;

	ipstr = (char *)dhcp_lookup_mac(addr);
	if (ipstr) {
		ip_addr_t ipaddr,*ptr;
		ptr = &ipaddr;
		ipaddr_aton(ipstr, ptr);
		return ip_addr_get_ip4_u32(ptr);
	}
	return 0;
}

void bk_net_begin_send_arp_reply_wrapper(int is_send_arp, int is_allow_send_req)
{
	net_begin_send_arp_reply(is_send_arp, is_allow_send_req);
}

void bk_set_sta_status_wrapper(void *info)
{
	mhdr_set_station_status(*(wifi_linkstate_reason_t *)info);
}
static void delay_wrapper(INT32 num)
{
    delay(num);
}

static void delay_us_wrapper(UINT32 us)
{
	delay_us(us);
}

static uint32_t sys_drv_modem_bus_clk_ctrl_wrapper(bool clk_en)
{
	return sys_drv_modem_bus_clk_ctrl(clk_en);
}

static uint32_t sys_drv_modem_clk_ctrl_wrapper(bool clk_en)
{
	return sys_drv_modem_clk_ctrl(clk_en);
}

static int32 sys_drv_int_enable_wrapper(uint32 param)
{
    return sys_drv_int_enable(param);
}

static int32 sys_drv_int_disable_wrapper(uint32 param)
{
    return sys_drv_int_disable(param);
}

static int32 sys_drv_int_group2_enable_wrapper(uint32 param)
{
    return sys_drv_int_group2_enable(param);
}

static int32 sys_drv_int_group2_disable_wrapper(uint32 param)
{
    return sys_drv_int_group2_disable(param);
}

static int32 sys_drv_module_power_state_get_wrapper(uint32_t module)
{
    return sys_drv_module_power_state_get(module);
}

static void rwnx_cal_set_channel_wrapper(UINT32 freq)
{
	#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
	rwnx_cal_set_channel(freq);
	#else
	return;
	#endif
}

static void bk7011_update_by_rx_wrapper(int8_t rssi, int8_t freq_offset)
{
	#if (CONFIG_SOC_BK7236XX)
	bk7011_update_by_rx(rssi, freq_offset);
	#else
	return;
	#endif
}

static void sys_ll_set_cpu_power_sleep_wakeup_pwd_ofdm_wrapper(uint32_t v)
{
    #if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
	sys_ll_set_cpu_power_sleep_wakeup_pwd_ofdm(v);
	#endif
}

static uint32_t sys_ll_get_cpu_power_sleep_wakeup_pwd_ofdm_wrapper(void)
{
	#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
	return sys_ll_get_cpu_power_sleep_wakeup_pwd_ofdm();
	#else
	return 0;
	#endif
}

static uint32_t sys_ll_get_cpu_device_clk_enable_mac_cken_wrapper(void)
{
	return sys_ll_get_cpu_device_clk_enable_mac_cken();
}

static uint32_t sys_ll_get_cpu_device_clk_enable_phy_cken_wrapper(void)
{
	return sys_ll_get_cpu_device_clk_enable_phy_cken();
}
static void power_save_delay_sleep_check_wrapper(void)
{
	//power_save_delay_sleep_check();
}

static void power_save_wake_mac_rf_if_in_sleep_wrapper(void)
{
	//power_save_wake_mac_rf_if_in_sleep();
}

static void power_save_wake_mac_rf_end_clr_flag_wrapper(void)
{
	//power_save_wake_mac_rf_end_clr_flag();
}

static UINT8 power_save_if_ps_rf_dtim_enabled_wrapper(void)
{
    //return power_save_if_ps_rf_dtim_enabled();
    return 0;
}

static UINT16 power_save_forbid_trace_wrapper(UINT16 forbid)
{
    //return power_save_forbid_trace((PS_FORBID_STATUS)forbid);
    return 0;
}

static bk_err_t bk_pm_module_vote_sleep_ctrl_wrapper(uint32_t module, uint32_t sleep_state, uint32_t sleep_time)
{
    return bk_pm_module_vote_sleep_ctrl((pm_sleep_module_name_e)module, sleep_state, sleep_time);
}

static uint32_t bk_pm_lpo_src_get_wrapper(void)
{
	return (uint32_t)bk_pm_lpo_src_get();
}

static int32 bk_pm_module_power_state_get_wrapper(unsigned int module)
{
    return bk_pm_module_power_state_get((pm_power_module_name_e)module);
}

static bk_err_t bk_pm_module_vote_power_ctrl_wrapper(unsigned int             module, uint32_t power_state)
{
    return bk_pm_module_vote_power_ctrl((pm_power_module_name_e)module, (pm_power_module_state_e)power_state);
}

static bk_err_t bk_pm_module_vote_cpu_freq_wrapper(uint32_t module, uint32_t cpu_freq)
{
    return bk_pm_module_vote_cpu_freq((pm_dev_id_e)module, (pm_cpu_freq_e)cpu_freq);
}

static bool bk_pm_phy_reinit_flag_get_wrapper(void)
{
    return bk_pm_phy_reinit_flag_get();
}

static void bk_pm_phy_reinit_flag_clear_wrapper(void)
{
	bk_pm_phy_reinit_flag_clear();
}

static bk_err_t bk_pm_sleep_register_wrapper(void *config_cb)
{
    pm_cb_conf_t enter_config_wifi = {NULL, NULL};

    enter_config_wifi.cb = config_cb;

#if !CONFIG_PM_SUPER_DEEP_SLEEP
    return bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP,PM_DEV_ID_MAC,&enter_config_wifi, NULL);
#else
    bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP,PM_DEV_ID_MAC,&enter_config_wifi, NULL);
    bk_pm_sleep_register_cb(PM_MODE_SUPER_DEEP_SLEEP,PM_DEV_ID_MAC,&enter_config_wifi, NULL);
    return BK_OK;
#endif
}


static bk_err_t bk_pm_low_voltage_register_wrapper(void *config_cb)
{
    pm_cb_conf_t wifi_exit_config = {NULL, NULL};

    wifi_exit_config.cb = config_cb;

	return bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE,PM_DEV_ID_MAC,NULL, &wifi_exit_config);
}

static void wifi_vote_rf_ctrl_wrapper(uint8_t cmd)
{
    rf_module_vote_ctrl(cmd,RF_BY_WIFI_BIT);
}

static void wifi_mac_phy_power_on_wrapper(void)
{
	//enable mac and phy power
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_WIFIP_MAC, PM_POWER_MODULE_STATE_ON);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_PHY, PM_POWER_MODULE_STATE_ON);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_PHY_WIFI,PM_POWER_MODULE_STATE_ON);

	//enable mac and phy clock
	bk_pm_clock_ctrl(PM_CLK_ID_MAC, PM_CLK_CTRL_PWR_UP);
	bk_pm_clock_ctrl(PM_CLK_ID_PHY, PM_CLK_CTRL_PWR_UP);
}

static bk_err_t bk_ckmn_driver_get_rc32k_ppm_wrapper(void)
{
	#if (CONFIG_CKMN)
	return bk_ckmn_driver_get_rc32k_ppm();
	#else
	return BK_OK;
	#endif
}

pm_cb_notify wifi_32k_src_switch_ready_notify_cb = NULL;

static void wifi_32k_src_switch_ready_notify_wrapper(void *cb)
{
    wifi_32k_src_switch_ready_notify_cb = (pm_cb_notify)cb;
}

static void mac_ps_exc32_init_wrapper(void *cb)
{
    pm_cb_extern32k_cfg_t cfg;

    cfg.cb_func = cb;
	cfg.cb_module = PM_32K_MODULE_WIFI;

	////ex32k change register
	pm_extern32k_register_cb(&cfg);
}

static void mac_ps_exc32_cb_notify_wrapper(void)
{
    if (wifi_32k_src_switch_ready_notify_cb)
        wifi_32k_src_switch_ready_notify_cb();
}

static void mac_ps_bcn_callback_wrapper(uint8_t *data, int len)
{
    //mac_ps_bcn_callback(data, len);
}

static UINT8 mac_sleeped_wrapper(void)
{
    //return mac_sleeped();
    return 0;
}

static void rwnx_cal_mac_sleep_rc_clr_wrapper(void)
{
	rwnx_cal_mac_sleep_rc_clr();
}

static void rwnx_cal_mac_sleep_rc_recover_wrapper(void)
{
	rwnx_cal_mac_sleep_rc_recover();
}

static void mac_printf_encode_wrapper(char *txt, size_t maxlen, const u8 *data, size_t len)
{
    char *end = txt + maxlen;
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (txt + 4 >= end)
            break;

        switch (data[i])
        {
        case '\"':
            *txt++ = '\\';
            *txt++ = '\"';
            break;
        case '\\':
            *txt++ = '\\';
            *txt++ = '\\';
            break;
        case '\033':
            *txt++ = '\\';
            *txt++ = 'e';
            break;
        case '\n':
            *txt++ = '\\';
            *txt++ = 'n';
            break;
        case '\r':
            *txt++ = '\\';
            *txt++ = 'r';
            break;
        case '\t':
            *txt++ = '\\';
            *txt++ = 't';
            break;
        default:
            if (data[i] >= 32 && data[i] <= 126)
                *txt++ = data[i];
            else
                txt += snprintf(txt, end - txt, "\\x%02x", data[i]);
            break;
        }
    }

    *txt = '\0';
}

static void dbg_enable_debug_gpio_wrapper(void)
{
    // 2 -> 9
    gpio_dev_unmap(GPIO_2);              // BK7256 EVB    BK7236 EVB      BIT
    gpio_dev_map(GPIO_2, GPIO_DEV_DEBUG0);   //CLK(H6)    CLK(H6)          0
    gpio_dev_unmap(GPIO_3);
    gpio_dev_map(GPIO_3, GPIO_DEV_DEBUG1);   //CMD(H6)    CMD(H6)          1
    gpio_dev_unmap(GPIO_4);
    gpio_dev_map(GPIO_4, GPIO_DEV_DEBUG2);   //D0(H6)     D0(H6)           2
    gpio_dev_unmap(GPIO_5);
    gpio_dev_map(GPIO_5, GPIO_DEV_DEBUG3);   //D1(H6)     D1(H6)           3
    gpio_dev_unmap(GPIO_6);
    gpio_dev_map(GPIO_6, GPIO_DEV_DEBUG4);   //CD(H6)     CD(H6)           4
    gpio_dev_unmap(GPIO_7);
    gpio_dev_map(GPIO_7, GPIO_DEV_DEBUG5);   //P7(H5)     P7(H5)           5
    gpio_dev_unmap(GPIO_8);
    gpio_dev_map(GPIO_8, GPIO_DEV_DEBUG6);   //P8(CON4)   P8(CON4)         6
    gpio_dev_unmap(GPIO_9);
    gpio_dev_map(GPIO_9, GPIO_DEV_DEBUG7);   //P9(CON4)   P9(CON4)         7

    // 14 -> 19
    gpio_dev_unmap(GPIO_14);
    gpio_dev_map(GPIO_14, GPIO_DEV_DEBUG8);  //P14(CON3)  P14(H3)          8
    gpio_dev_unmap(GPIO_15);
    gpio_dev_map(GPIO_15, GPIO_DEV_DEBUG9);  //P15(CON3)  P15(H3)          9
    gpio_dev_unmap(GPIO_16);
    gpio_dev_map(GPIO_16, GPIO_DEV_DEBUG10); //S14(H3)    P16(H3)         10
    gpio_dev_unmap(GPIO_17);
    gpio_dev_map(GPIO_17, GPIO_DEV_DEBUG11); //           P17(H3)         11
    gpio_dev_unmap(GPIO_18);
    gpio_dev_map(GPIO_18, GPIO_DEV_DEBUG12); //           P18(H5)         12
    gpio_dev_unmap(GPIO_19);
    gpio_dev_map(GPIO_19, GPIO_DEV_DEBUG13); //           P19(H5)         13

    // 24 -> 39
    gpio_dev_unmap(GPIO_24);
    gpio_dev_map(GPIO_24, GPIO_DEV_DEBUG14); //           P24(H8)         14
    gpio_dev_unmap(GPIO_25);
    gpio_dev_map(GPIO_25, GPIO_DEV_DEBUG15); //           P25(H8)         15
    gpio_dev_unmap(GPIO_26);
    gpio_dev_map(GPIO_26, GPIO_DEV_DEBUG16); //           P26(H8)         16
    gpio_dev_unmap(GPIO_27);
    gpio_dev_map(GPIO_27, GPIO_DEV_DEBUG17); //           MCLK(H14)       17
    gpio_dev_unmap(GPIO_28);
    gpio_dev_map(GPIO_28, GPIO_DEV_DEBUG18); //           P28/ADC4(H5)    18
    gpio_dev_unmap(GPIO_29);
    gpio_dev_map(GPIO_29, GPIO_DEV_DEBUG19); //           PCLK(H14)       19
    gpio_dev_unmap(GPIO_30);
    gpio_dev_map(GPIO_30, GPIO_DEV_DEBUG20); //           HSYNC(H14)      20
    gpio_dev_unmap(GPIO_31);
    gpio_dev_map(GPIO_31, GPIO_DEV_DEBUG21); //           VSYNC(H14)      21
    gpio_dev_unmap(GPIO_32);
    gpio_dev_map(GPIO_32, GPIO_DEV_DEBUG22); //           PXD0(H14)       22
    gpio_dev_unmap(GPIO_33);
    gpio_dev_map(GPIO_33, GPIO_DEV_DEBUG23); //           PXD1(H14)       23
    gpio_dev_unmap(GPIO_34);
    gpio_dev_map(GPIO_34, GPIO_DEV_DEBUG24); //           PXD2(H14)       24
    gpio_dev_unmap(GPIO_35);
    gpio_dev_map(GPIO_35, GPIO_DEV_DEBUG25); //           PXD3(H14)       25
    gpio_dev_unmap(GPIO_36);
    gpio_dev_map(GPIO_36, GPIO_DEV_DEBUG26); //           PXD4(H14)       26
    gpio_dev_unmap(GPIO_37);
    gpio_dev_map(GPIO_37, GPIO_DEV_DEBUG27); //           PXD5(H14)       27
    gpio_dev_unmap(GPIO_38);
    gpio_dev_map(GPIO_38, GPIO_DEV_DEBUG28); //           PXD6(H14)       28
    gpio_dev_unmap(GPIO_39);
    gpio_dev_map(GPIO_39, GPIO_DEV_DEBUG29); //           PXD7(H14)       29

    // 42 -> 43
    gpio_dev_unmap(GPIO_42);
    gpio_dev_map(GPIO_42, GPIO_DEV_DEBUG30); //           P42(CONN4)      30
    gpio_dev_unmap(GPIO_43);
    gpio_dev_map(GPIO_43, GPIO_DEV_DEBUG31); //           P43(CONN4)      31
}

static bk_err_t gpio_dev_unprotect_unmap_wrapper(uint32_t gpio_id)
{
    return gpio_dev_unprotect_unmap(gpio_id);
}

static bk_err_t gpio_dev_unprotect_map_wrapper( uint32_t gpio_id, uint32_t dev)
{
    return gpio_dev_unprotect_map(gpio_id,dev);
}

static int ps_need_pre_process_wrapper( UINT32 arg )
{
     return 0;
}

static bool power_save_rf_sleep_check_wrapper(void)
{
     return false;
}

static UINT32 mcu_ps_machw_cal_wrapper(void)
{
    //return mcu_ps_machw_cal();
    return 0;
}

static UINT32 mcu_ps_machw_reset_wrapper(void)
{
    //return mcu_ps_machw_reset();
    return 0;
}

static UINT32 mcu_ps_machw_init_wrapper(void)
{
    //return mcu_ps_machw_init();
    return 0;
}

static void mcu_ps_bcn_callback_wrapper(uint8_t *data, int len)
{
	//mcu_ps_bcn_callback(data, len);
}

static int bk_feature_receive_bcmc_enable_wrapper(void)
{
    return bk_feature_receive_bcmc_enable();
}

static int bk_feature_bssid_connect_enable_wrapper(void)
{
    return bk_feature_bssid_connect_enable();
}

static int bk_feature_fast_connect_enable_wrapper(void)
{
    return bk_feature_fast_connect_enable();
}

static int bk_feature_send_deauth_before_connect_wrapper(void)
{
    return bk_feature_send_deauth_before_connect();
}

static int bk_feature_fast_dhcp_enable_wrapper(void)
{
    return bk_feature_fast_dhcp_enable();
}

static int bk_feature_not_check_ssid_enable_wrapper(void)
{
    return bk_feature_not_check_ssid_enable();
}

static int bk_feature_config_cache_enable_wrapper(void)
{
    return bk_feature_config_cache_enable();
}

static int bk_feature_sta_vsie_enable_wrapper(void)
{
    return bk_feature_sta_vsie_enable();
}

static int bk_feature_ap_statype_limit_enable_wrapper(void)
{
    return bk_feature_ap_statype_limit_enable();
}

static int bk_feature_ckmn_enable_wrapper(void)
{
    return bk_feature_ckmn_enable();
}
static int bk_feature_config_wifi_csi_enable_wrapper(void)
{
    return bk_feature_config_wifi_csi_enable();
}

static int bk_feature_close_coexist_csa_wrapper(void)
{
    return bk_feature_close_coexist_csa();
}

static int bk_feature_get_scan_speed_level_wrapper(void)
{
    return bk_feature_get_scan_speed_level();
}

static void flush_all_dcache_wrapper(void)
{
#if CONFIG_CACHE_ENABLE
	// SOC_SMP will set cache to no-cachable
#ifndef CONFIG_SOC_SMP
	flush_all_dcache();
#endif
#endif
}

static unsigned long bk_ms_to_ticks_wrapper(unsigned long ms)
{
	return BK_MS_TO_TICKS(ms);
}

static bk_err_t dma_memcpy_wrapper(void *out, const void *in, uint32_t len)
{
    return dma_memcpy(out, in, len);
}

static bk_err_t bk_wifi_interrupt_init_wrapper(void)
{
	sys_drv_enable_mac_gen_int();
	sys_drv_enable_mac_prot_int();
	sys_drv_enable_mac_tx_trigger_int();
	sys_drv_enable_mac_rx_trigger_int();

#if (!CONFIG_SOC_BK7236A)
	// TX_RX_MISC no need on bk7236, for debug purpuse on bk7236, changed temporarily
	sys_drv_enable_mac_txrx_misc_int();
#endif

#if (CONFIG_SOC_BK7236A)
	// open FIQ_RC for phy on bk7236, for debug purpuse on bk7236, changed temporarily
	sys_drv_enable_modem_rc_int();
#endif

#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
	// enable HSU interrupt
#if CFG_HSU
	sys_drv_enable_hsu_int();
#endif
#endif

	sys_drv_enable_mac_txrx_timer_int();
	sys_drv_enable_modem_int();

	return BK_OK;
}

static void bk_wifi_stop_rf_wrapper(void)
{
	rf_module_vote_ctrl(RF_CLOSE,RF_BY_WIFI_BIT);
}

static uint32_t rtos_get_ms_per_tick_wrapper(void)
{
    return rtos_get_ms_per_tick();
}

static UINT32 rf_pll_ctrl_wrapper(UINT32 cmd, UINT32 param)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
    return rf_pll_ctrl(cmd, param);
#else
   return 0;
#endif
}

static uint32_t rtos_disable_int_wrapper(void)
{
	return rtos_enter_critical();
}

static void rtos_enable_int_wrapper(uint32_t int_level)
{
	rtos_exit_critical(int_level);
}

static uint32_t rtos_enter_critical_wrapper(void)
{
	return rtos_enter_critical();
}

static void rtos_exit_critical_wrapper(uint32_t irq_level)
{
	rtos_exit_critical(irq_level);
}

static uint32_t rtos_get_time_wrapper(void)
{
    return rtos_get_time();
}

static void *os_malloc_wrapper(const char *func_name, int line, size_t size)
{
#if (CONFIG_MEM_DEBUG)
    return (void *)os_malloc_debug(func_name, line, size, 0);
#else
    return (void *)os_malloc(size);
#endif
}

static void os_free_wrapper(void *mem_ptr)
{
    os_free(mem_ptr);
}


static void rtos_assert_wrapper(uint32_t exp)
{
    BK_ASSERT(exp);
}

static INT32 os_memcmp_wrapper(const void *s, const void *s1, UINT32 n)
{
	return os_memcmp(s, s1, (unsigned int)n);
}

static void *os_memset_wrapper(void *b, int c, UINT32 len)
{
	return (void *)os_memset(b, c, (unsigned int)len);
}

static void *os_zalloc_wrapper(const char *func_name, int line, size_t size)
{
#if (CONFIG_MEM_DEBUG)
	return (void *)os_malloc_debug(func_name, line, size, 1);
#else
	return os_zalloc(size);
#endif
}

static void *rtos_realloc_wrapper(void *ptr, size_t size)
{
	return (void *)os_realloc(ptr, size);
}
#if 0
static int shell_assert_out_wrapper(bool bContinue, char * format, ...)
{
	return shell_assert_out(bContinue, format, ...);
}

static void bk_printf_wrapper(const char *fmt, ...)
{
	bk_printf(fmt, ...);
}

static void bk_null_printf_wrapper(const char *fmt, ...)
{
	bk_null_printf(fmt, ...);
}
#endif

static bool wifi_ate_is_enabled_wrapper(void)
{
	return ate_is_enabled();
}

static bk_err_t rtos_push_to_queue_wrapper( void** queue, void* message, uint32_t timeout_ms )
{
    return rtos_push_to_queue(queue, message, timeout_ms );
}

static bool rtos_is_queue_full_wrapper(void** queue )
{
    return rtos_is_queue_full(queue);
}

static bool rtos_is_queue_empty_wrapper(void** queue )
{
    return rtos_is_queue_empty(queue);
}

static bk_err_t rtos_pop_from_queue_wrapper( void** queue, void* message, uint32_t timeout_ms )
{
    return rtos_pop_from_queue( queue, message, timeout_ms );
}

static bk_err_t rtos_push_to_queue_front_wrapper( void** queue, void* message, uint32_t timeout_ms )
{
     return rtos_push_to_queue_front(queue, message, timeout_ms);
}

static uint64_t bk_aon_rtc_get_current_tick_wrapper(uint32_t id)
{
	return bk_aon_rtc_get_current_tick(id);
}

static bk_err_t bk_int_isr_register_wrapper(uint32_t src, void* isr_callback, void*arg)
{

	return bk_int_isr_register(src, isr_callback, arg);
}

static float bk_rtc_get_ms_tick_count_wrapper(void)
{
	return (float)bk_rtc_get_ms_tick_count();
}

static char *os_strdup_wrapper(const char *s)
{
	return (char *)os_strdup(s);
}

static INT32 os_strcmp_wrapper(const char *s1, const char *s2)
{
    return os_strcmp(s1, s2);
}

static char *os_strchr_wrapper(const char *s, int c)
{
    return (char *)os_strchr(s, c);
}

static UINT32 os_strlen_wrapper(const char *str)
{
    return os_strlen(str);
}

static void *os_memcpy_wrapper(void *out, const void *in, UINT32 n)
{
	return (void *)os_memcpy(out, in, n);
}

static void *os_memmove_wrapper(void *out, const void *in, UINT32 n)
{
	return (void *)os_memmove(out, in, n);
}

static unsigned int os_strlcpy_wrapper(char *dest, const char *src, size_t siz)
{

    return os_strlcpy(dest,src,siz);
}

static INT32 os_strncmp_wrapper(const char *s1, const char *s2, const UINT32 n)
{
	return os_strncmp(s1, s2, n);
}

static char *os_strcpy_wrapper(char *out, const char *in)
{
    return (char *)os_strcpy(out, in);
}

static bk_err_t rtos_delay_milliseconds_wrapper(uint32_t num_ms)
{
	return rtos_delay_milliseconds(num_ms);
}
#if 0
static INT32 os_snprintf_wrapper(char *buf, UINT32 size, const char *fmt, ...)
{

	return os_snprintf(buf, size, fmt, ...);
}
#endif
static INT32 os_vsnprintf_wrapper(char *buf, UINT32 size, const char *fmt, va_list ap)
{
	return os_vsnprintf(buf, size, fmt, ap);
}

static char *os_strncpy_wrapper(char *out, const char *in, const UINT32 n)
{
	return os_strncpy(out, in, n);
}

static UINT32 os_strtoul_wrapper(const char *nptr, char **endptr, int base)
{
	return os_strtoul(nptr, endptr, base);
}

static int os_strcasecmp_wrapper(const char *s1, const char *s2)
{
	return os_strcasecmp(s1, s2);
}

static int os_strncasecmp_wrapper(const char *s1, const char *s2, size_t n)
{

	return os_strncasecmp(s1, s2, n);
}

static char *os_strrchr_wrapper(const char *s, int c)
{
	return os_strrchr(s, c);
}

static char *os_strstr_wrapper(const char *haystack, const char *needle)
{
	return os_strstr(haystack, needle);
}

static bk_err_t rtos_init_semaphore_wrapper( void **semaphore, int maxCount )
{

    return rtos_init_semaphore(semaphore,maxCount);
}

static bk_err_t rtos_get_semaphore_wrapper(void **semaphore, uint32_t timeout_ms)
{
     return rtos_get_semaphore(semaphore, timeout_ms);
}

static bk_err_t rtos_deinit_semaphore_wrapper(void **semaphore)
{
     return rtos_deinit_semaphore(semaphore);
}

static int rtos_set_semaphore_wrapper(void **semaphore)
{
     return rtos_set_semaphore(semaphore);
}

static bk_err_t rtos_create_thread_wrapper(void** thread, uint8_t priority, const char* name,
                        void* function, uint32_t stack_size, void* arg )
{
    return rtos_create_thread(thread, priority, name, function, stack_size, arg);
}


static bk_err_t rtos_delete_thread_wrapper(void** thread)
{
	return rtos_delete_thread(thread);

}

static size_t rtos_get_free_heap_size_wrapper(void)
{
	return rtos_get_free_heap_size();
}

static bk_err_t rtos_deinit_queue_wrapper(void** queue)
{
	return rtos_deinit_queue(queue);
}

static bk_err_t rtos_init_queue_wrapper(void** queue, const char *name, uint32_t message_size, uint32_t number_of_messages)
{
	return rtos_init_queue(queue, name, message_size, number_of_messages);
}

static bk_err_t rtos_deinit_mutex_wrapper(void **mutex)
{
	return rtos_deinit_mutex(mutex);
}

static bk_err_t rtos_init_mutex_wrapper(void **mutex)
{
	return rtos_init_mutex(mutex);
}

static bk_err_t rtos_lock_mutex_wrapper(void **mutex)
{
	return rtos_lock_mutex(mutex);
}
static bk_err_t rtos_unlock_mutex_wrapper(void ** mutex )
{
	return rtos_unlock_mutex(mutex);
}

static bk_err_t rtos_reload_timer_wrapper(void *timer)
{
	return rtos_reload_timer(timer);
}

static bool rtos_is_timer_running_wrapper(void *timer)
{
	return rtos_is_timer_running(timer);
}

static bk_err_t rtos_stop_timer_wrapper(void *timer)
{
	return rtos_stop_timer(timer);
}
static bk_err_t rtos_init_timer_wrapper(void *timer, uint32_t time_ms, void *function, void *arg)
{
	return rtos_init_timer(timer, time_ms, function, arg);
}

static bool rtos_is_current_thread_wrapper(void** thread)
{
	return rtos_is_current_thread(thread);
}

static void register_wifi_dump_hook_wrapper(void *wifi_func)
{
	rtos_regist_wifi_dump_hook(wifi_func);
}

static void bk_airkiss_start_udp_boardcast_wrapper(u8 random_data)
{
	int err, i;
	int udp_broadcast_fd = -1;
	struct sockaddr_in remote_skt;

	BK_LOGI(TAG, "start_udp_boardcast\n");
	udp_broadcast_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_broadcast_fd == -1) {
		BK_LOGE(TAG, "Socket failed\r\n");
		return;
	}
	os_memset(&remote_skt, 0, sizeof(struct sockaddr_in));
	remote_skt.sin_family = AF_INET;
	remote_skt.sin_addr.s_addr = INADDR_BROADCAST;//INADDR_ANY;
	remote_skt.sin_port = htons(10000);

	i = 20;
	while (i --) {
		BK_LOGI(TAG, "udp-sendto:%d\r\n", i);

		err = sendto(udp_broadcast_fd, &random_data, 1, 0, (struct sockaddr *)&remote_skt, sizeof(remote_skt));
		rtos_delay_milliseconds(2);

		if (err == -1)
			BK_LOGE(TAG, "send udp boardcast failed\r\n");
	}

	BK_LOGI(TAG, "close socket\r\n");
	close(udp_broadcast_fd);
}

static void tx_verify_test_call_back_wrapper(void)
{
#if CONFIG_ATE_TEST
	tx_verify_test_call_back();
#endif
}

static void sys_hal_enter_low_analog_wrapper(void)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
	sys_hal_enter_low_analog();
#endif
}

static void sys_hal_exit_low_analog_wrapper(void)
{
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
	sys_hal_exit_low_analog();
#endif
}

#ifndef CONFIG_SHELL_ASYNCLOG
static int sync_assert_out(bool bContinue, char *format, ...)
{
	u32         int_mask;
	va_list     args;
	char string[CONFIG_PRINTF_BUF_SIZE];

	va_start(args, format);

	int_mask = rtos_disable_int();

	vsnprintf(string, sizeof(string) - 1, format, args);
	uart_write_string(bk_get_printf_port(), string);

	va_end(args);

	if (bContinue) {
		rtos_enable_int(int_mask);
		return 1;
	}

	// Halt here
	for (;;);

	return 0;
}
#endif

__attribute__((section(".dtcm_sec_data "))) wifi_os_funcs_t g_wifi_os_funcs = {
	._version = BK_WIFI_OS_ADAPTER_VERSION,
	._manual_cal_rfcali        = manual_cal_rfcali_status,
	._rc_drv_get_rf_en = rc_drv_get_rf_en,
	._cal_set_rfconfig_BTPLL   = rwnx_cal_set_rfconfig_BTPLL,
	._cal_set_rfconfig_WIFIPLL = rwnx_cal_set_rfconfig_WIFIPLL,
	._calibration_init = calibration_init,
	._sys_is_enable_hw_tpc     = rwnx_sys_is_enable_hw_tpc,
	._evm_via_mac_continue     = evm_via_mac_continue,
	._evm_via_mac_evt          = evm_via_mac_evt,
	._do_evm                   = do_evm,
	._do_rx_sensitivity        = do_rx_sensitivity,
	._rwnx_cal_save_trx_rcbekn_reg_val = rwnx_cal_save_trx_rcbekn_reg_val,
	._rc_drv_set_agc_manual_en = rc_drv_set_agc_manual_en,
	._rc_drv_set_rx_mode_enrxsw = rc_drv_set_rx_mode_enrxsw,
	._rc_drv_get_rx_mode_enrxsw = rc_drv_get_rx_mode_enrxsw,
	._bk7011_max_rxsens_setting = bk7011_max_rxsens_setting,
	._bk7011_default_rxsens_setting = bk7011_default_rxsens_setting,
	._tx_evm_rate_get = tx_evm_rate_get,
	._tx_evm_pwr_idx_get = tx_evm_pwr_idx_get,
	._tx_evm_modul_format_get = tx_evm_modul_format_get,
	._tx_evm_guard_i_tpye_get = tx_evm_guard_i_tpye_get,
	._tx_evm_bandwidth_get = tx_evm_bandwidth_get,
	._tx_evm_mode_get      = tx_evm_mode_get,
	._rwnx_tpc_get_pwridx_by_rate = rwnx_tpc_get_pwridx_by_rate,
	._rwnx_is_enable_pwr_change_by_rssi = rwnx_is_enable_pwr_change_by_rssi,
	._tpc_auto_change_pwr_by_rssi = tpc_auto_change_pwr_by_rssi,
	._rwnx_setting_for_single_rate = rwnx_setting_for_single_rate,
	._delay05us = delay05us,
	._rwnx_cal_recover_rf_setting = rwnx_cal_recover_rf_setting,
	._rwnx_cal_recover_wifi_setting = rwnx_cal_recover_wifi_setting,
	._rwnx_cal_set_40M_setting = rwnx_cal_set_40M_setting,
	._rwnx_cal_set_20M_setting = rwnx_cal_set_20M_setting,
	._rwnx_cal_recover_rcbeken_reg_val = rwnx_cal_recover_rcbeken_reg_val,
	._rwnx_cal_recover_trx_reg_val = rwnx_cal_recover_trx_reg_val,
	._rwnx_cal_set_channel = rwnx_cal_set_channel_wrapper,
	._bk7011_cal_pll = bk7011_cal_pll,
	._bk7011_update_by_rx = bk7011_update_by_rx_wrapper,
	._rwnx_cal_load_trx_rcbekn_reg_val = rwnx_cal_load_trx_rcbekn_reg_val,
	._manual_get_epa_flag = manual_get_epa_flag,
	._rxsens_start_flag_get = rxsens_start_flag_get,

	._pbkdf2_sha1 = pbkdf2_sha1,
	._sha1_prf = sha1_prf,
	._omac1_aes_128_vector = omac1_aes_128_vector,
	._aes_wrap = aes_wrap,
	._rc4_skip = rc4_skip,
	._hmac_md5 = hmac_md5,
	._hmac_sha1 = hmac_sha1,
	._aes_unwrap= aes_unwrap,

//crypto_mbedtls.c
	.__crypto_ecdh_deinit = _crypto_ecdh_deinit_wrapper,
	.__crypto_ecdh_init = _crypto_ecdh_init_wrapper,
	.__crypto_ecdh_set_peerkey = _crypto_ecdh_set_peerkey_wrapper,
	.__crypto_ecdh_get_pubkey = _crypto_ecdh_get_pubkey_wrapper,
	._sha256_vector = sha256_vector,
	._sha384_vector = sha384_vector,
	._sha512_vector = sha512_vector,
	._hmac_sha256 = hmac_sha256,
	._hmac_sha384 = hmac_sha384,
	._hmac_sha512 = hmac_sha512,
	._hmac_sha256_kdf = hmac_sha256_kdf,
	._hmac_sha384_kdf = hmac_sha384_kdf,
	._hmac_sha512_kdf = hmac_sha512_kdf,
	.__crypto_bignum_init = _crypto_bignum_init_wrapper,
	._omac1_aes_128 = omac1_aes_128,
	._sha256_prf = sha256_prf,
	._sha384_prf = sha384_prf,
	._sha512_prf = sha512_prf,
	._hmac_sha384_vector = hmac_sha384_vector,
	._hmac_sha256_vector = hmac_sha256_vector,
	._hmac_sha1_vector = hmac_sha1_vector,
	._hmac_sha512_vector = hmac_sha512_vector,
	.__crypto_bignum_rand = _crypto_bignum_rand_wrapper,
	.__crypto_bignum_deinit = _crypto_bignum_deinit_wrapper,
	.__crypto_bignum_init_set = _crypto_bignum_init_set_wrapper,
	.__crypto_bignum_init_uint = _crypto_bignum_init_uint_wrapper,
	.__crypto_bignum_to_bin = _crypto_bignum_to_bin_wrapper,
	.__crypto_bignum_add = _crypto_bignum_add_wrapper,
	.__crypto_bignum_sub = _crypto_bignum_sub_wrapper,
	.__crypto_bignum_mod = _crypto_bignum_mod_wrapper,
	.__crypto_bignum_cmp = _crypto_bignum_cmp_wrapper,
	.__crypto_bignum_mulmod = _crypto_bignum_mulmod_wrapper,
	.__crypto_bignum_inverse = _crypto_bignum_inverse_wrapper,
	.__crypto_bignum_addmod = _crypto_bignum_addmod_wrapper,
	.__crypto_bignum_sqrmod = _crypto_bignum_sqrmod_wrapper,
	.__crypto_bignum_exptmod = _crypto_bignum_exptmod_wrapper,
	.__crypto_bignum_is_zero = _crypto_bignum_is_zero_wrapper,
	.__crypto_bignum_is_one = _crypto_bignum_is_one_wrapper,
	.__crypto_bignum_is_odd = _crypto_bignum_is_odd_wrapper,
	.__crypto_bignum_rshift = _crypto_bignum_rshift_wrapper,
	.__crypto_bignum_legendre = _crypto_bignum_legendre_wrapper,
	.__crypto_ec_init= _crypto_ec_init_wrapper,
	.__crypto_ec_deinit = _crypto_ec_deinit_wrapper,
	._sha256_prf_bits = sha256_prf_bits,
	.__crypto_ec_point_init = _crypto_ec_point_init_wrapper,
	.__crypto_ec_point_deinit = _crypto_ec_point_deinit_wrapper,
	.__crypto_ec_prime_len = _crypto_ec_prime_len_wrapper,
	.__crypto_ec_get_prime = _crypto_ec_get_prime_wrapper,
	.__crypto_ec_get_order = _crypto_ec_get_order_wrapper,
	.__crypto_ec_order_len = _crypto_ec_order_len_wrapper,
	.__crypto_ec_point_mul = _crypto_ec_point_mul_wrapper,
	.__crypto_ec_point_to_bin = _crypto_ec_point_to_bin_wrapper,
	.__crypto_ec_point_from_bin = _crypto_ec_point_from_bin_wrapper,
	.__crypto_ec_point_add = _crypto_ec_point_add_wrapper,
	.__crypto_ec_point_invert = _crypto_ec_point_invert_wrapper,
	.__crypto_ec_point_cmp = _crypto_ec_point_cmp_wrapper,
	.__crypto_ec_get_a = _crypto_ec_get_a_wrapper,
	.__crypto_ec_get_b = _crypto_ec_get_b_wrapper,
	.__crypto_ec_point_compute_y_sqr = _crypto_ec_point_compute_y_sqr_wrapper,
	.__crypto_ec_point_is_at_infinity = _crypto_ec_point_is_at_infinity_wrapper,
	.__crypto_ec_point_is_on_curve = _crypto_ec_point_is_on_curve_wrapper,
	.__crypto_ec_prime_len_bits = _crypto_ec_prime_len_bits_wrapper,
	._pbuf_alloc = bk_pbuf_alloc_wrapper,
	._pbuf_free = bk_pbuf_free_wrapper,
	._pbuf_ref = bk_pbuf_ref_wrapper,
	._pbuf_header = bk_pbuf_header_wrapper,
	._pbuf_coalesce = bk_pbuf_coalesce_wrapper,
	._pbuf_cat = bk_pbuf_cat_wrapper,
	._get_rx_pbuf_type = bk_get_rx_pbuf_type_wrapper,
	._get_pbuf_pool_size = bk_get_pbuf_pool_size_wrapper,
	._get_netif_hostname = bk_get_netif_hostname_wrapper,
	._save_net_info = bk_save_net_info_wrapper,
	._get_net_info = bk_get_net_info_wrapper,
	._inet_ntoa = bk_inet_ntoa_wrapper,
	._lwip_ntohl = bk_lwip_ntohl_wrapper,
	._lwip_htons = bk_lwip_htons_wrapper,
	._sta_ip_down = bk_sta_ip_down_wrapper,
	._uap_ip_start = bk_uap_ip_start_wrapper,
	._uap_ip_down = bk_uap_ip_down_wrapper,
	._net_wlan_add_netif = bk_net_wlan_add_netif_wrapper,
	._net_wlan_remove_netif = bk_net_wlan_remove_netif_wrapper,
	._lookup_ipaddr = bk_lookup_ipaddr_wrapper,
	._set_sta_status = bk_set_sta_status_wrapper,
	._sta_ip_mode_set = sta_ip_mode_set,
	._net_begin_send_arp_reply = bk_net_begin_send_arp_reply_wrapper,
	._delay_us = delay_us_wrapper,
	._sys_drv_modem_bus_clk_ctrl = sys_drv_modem_bus_clk_ctrl_wrapper,
	._sys_drv_modem_clk_ctrl = sys_drv_modem_clk_ctrl_wrapper,
	._sys_drv_int_enable = sys_drv_int_enable_wrapper,
	._sys_drv_int_disable = sys_drv_int_disable_wrapper,
	._sys_drv_int_group2_enable = sys_drv_int_group2_enable_wrapper,
	._sys_drv_int_group2_disable = sys_drv_int_group2_disable_wrapper,
	._sys_ll_set_cpu_power_sleep_wakeup_pwd_ofdm = sys_ll_set_cpu_power_sleep_wakeup_pwd_ofdm_wrapper,
	._sys_ll_get_cpu_power_sleep_wakeup_pwd_ofdm = sys_ll_get_cpu_power_sleep_wakeup_pwd_ofdm_wrapper,
	._sys_ll_get_cpu_device_clk_enable_mac_cken = sys_ll_get_cpu_device_clk_enable_mac_cken_wrapper,
	._sys_ll_get_cpu_device_clk_enable_phy_cken = sys_ll_get_cpu_device_clk_enable_phy_cken_wrapper,
	._sys_drv_module_power_state_get = sys_drv_module_power_state_get_wrapper,
	._power_save_delay_sleep_check = power_save_delay_sleep_check_wrapper,
	._power_save_wake_mac_rf_if_in_sleep = power_save_wake_mac_rf_if_in_sleep_wrapper,
	._power_save_wake_mac_rf_end_clr_flag = power_save_wake_mac_rf_end_clr_flag_wrapper,
	._power_save_if_ps_rf_dtim_enabled = power_save_if_ps_rf_dtim_enabled_wrapper,
	._power_save_forbid_trace = power_save_forbid_trace_wrapper,
	._bk_pm_module_vote_sleep_ctrl = bk_pm_module_vote_sleep_ctrl_wrapper,
	._bk_pm_lpo_src_get = bk_pm_lpo_src_get_wrapper,
	._ps_need_pre_process = ps_need_pre_process_wrapper,
	._power_save_rf_sleep_check = power_save_rf_sleep_check_wrapper,
	._bk_pm_module_power_state_get = bk_pm_module_power_state_get_wrapper,
	._bk_pm_module_vote_power_ctrl = bk_pm_module_vote_power_ctrl_wrapper,
	._bk_pm_module_vote_cpu_freq = bk_pm_module_vote_cpu_freq_wrapper,
	._bk_pm_phy_reinit_flag_get = bk_pm_phy_reinit_flag_get_wrapper,
	._bk_pm_phy_reinit_flag_clear = bk_pm_phy_reinit_flag_clear_wrapper,
	._bk_pm_sleep_register = bk_pm_sleep_register_wrapper,
	._bk_pm_low_voltage_register = bk_pm_low_voltage_register_wrapper,
	._wifi_vote_rf_ctrl = wifi_vote_rf_ctrl_wrapper,
	._wifi_mac_phy_power_on = wifi_mac_phy_power_on_wrapper,
	._mac_ps_exc32_cb_notify = mac_ps_exc32_cb_notify_wrapper,
	._mac_ps_exc32_init = mac_ps_exc32_init_wrapper,
	._wifi_32k_src_switch_ready_notify = wifi_32k_src_switch_ready_notify_wrapper,
	._mac_ps_bcn_callback = mac_ps_bcn_callback_wrapper,
	._mac_sleeped = mac_sleeped_wrapper,
	._rwnx_cal_mac_sleep_rc_clr = rwnx_cal_mac_sleep_rc_clr_wrapper,
	._rwnx_cal_mac_sleep_rc_recover = rwnx_cal_mac_sleep_rc_recover_wrapper,
	._bk_ckmn_driver_get_rc32k_ppm = bk_ckmn_driver_get_rc32k_ppm_wrapper,
	._mac_printf_encode = mac_printf_encode_wrapper,
	._dbg_enable_debug_gpio = dbg_enable_debug_gpio_wrapper,
	._gpio_dev_unprotect_unmap = gpio_dev_unprotect_unmap_wrapper,
	._gpio_dev_unprotect_map = gpio_dev_unprotect_map_wrapper,
	._mcu_ps_machw_cal = mcu_ps_machw_cal_wrapper,
	._mcu_ps_machw_reset = mcu_ps_machw_reset_wrapper,
	._mcu_ps_machw_init = mcu_ps_machw_init_wrapper,
	._mcu_ps_bcn_callback = mcu_ps_bcn_callback_wrapper,
	._bk_feature_config_wifi_csi_enable = bk_feature_config_wifi_csi_enable_wrapper,
	._bk_feature_receive_bcmc_enable = bk_feature_receive_bcmc_enable_wrapper,
	._bk_feature_bssid_connect_enable = bk_feature_bssid_connect_enable_wrapper,
	._bk_feature_fast_connect_enable = bk_feature_fast_connect_enable_wrapper,
	._bk_feature_send_deauth_before_connect = bk_feature_send_deauth_before_connect_wrapper,
	._bk_feature_get_scan_speed_level = bk_feature_get_scan_speed_level_wrapper,
	._bk_feature_fast_dhcp_enable = bk_feature_fast_dhcp_enable_wrapper,
	._bk_feature_not_check_ssid_enable = bk_feature_not_check_ssid_enable_wrapper,
	._bk_feature_config_cache_enable = bk_feature_config_cache_enable_wrapper,
	._bk_feature_config_ckmn_enable = bk_feature_ckmn_enable_wrapper,
	._bk_feature_sta_vsie_enable = bk_feature_sta_vsie_enable_wrapper,
	._bk_feature_ap_statype_limit_enable = bk_feature_ap_statype_limit_enable_wrapper,
	._bk_feature_close_coexist_csa = bk_feature_close_coexist_csa_wrapper,
	._flush_all_dcache = flush_all_dcache_wrapper,
	._bk_ms_to_ticks = bk_ms_to_ticks_wrapper,
	._dma_memcpy = dma_memcpy_wrapper,
	._bk_wifi_interrupt_init = bk_wifi_interrupt_init_wrapper,
	._bk_wifi_stop_rf = bk_wifi_stop_rf_wrapper,
	._rtos_get_ms_per_tick = rtos_get_ms_per_tick_wrapper,
	._rtos_disable_int = rtos_disable_int_wrapper,
	._rtos_enable_int =rtos_enable_int_wrapper,
	._rtos_enter_critical = rtos_enter_critical_wrapper,
	._rtos_exit_critical = rtos_exit_critical_wrapper,
	._log = bk_printf_ext,
	._log_raw = bk_printf_raw,
	._rtos_get_time = rtos_get_time_wrapper,
	._os_malloc = os_malloc_wrapper,
	._os_free = os_free_wrapper,
	._rtos_assert = rtos_assert_wrapper,
	._os_memcmp = os_memcmp_wrapper,
	._os_memset = os_memset_wrapper,
	._os_zalloc = os_zalloc_wrapper,
	._rtos_realloc = rtos_realloc_wrapper,
#ifdef CONFIG_SHELL_ASYNCLOG
	._shell_assert_out = shell_assert_out,
#else
	._shell_assert_out = sync_assert_out,
#endif
	._bk_printf = bk_printf,
	._bk_null_printf = bk_null_printf,
	._ate_is_enabled = wifi_ate_is_enabled_wrapper,
	._rtos_push_to_queue = rtos_push_to_queue_wrapper,
	._rtos_is_queue_full = rtos_is_queue_full_wrapper,
	._rtos_is_queue_empty = rtos_is_queue_empty_wrapper,
	._rtos_pop_from_queue = rtos_pop_from_queue_wrapper,
	._rtos_push_to_queue_front = rtos_push_to_queue_front_wrapper,
	._bk_aon_rtc_get_current_tick = bk_aon_rtc_get_current_tick_wrapper,
	._bk_int_isr_register = bk_int_isr_register_wrapper,
	._rtc_get_ms_tick_cnt = bk_rtc_get_ms_tick_count_wrapper,
	._rtos_init_semaphore = rtos_init_semaphore_wrapper,
	._rtos_get_semaphore = rtos_get_semaphore_wrapper,
	._rtos_deinit_semaphore = rtos_deinit_semaphore_wrapper,
	._rtos_set_semaphore = rtos_set_semaphore_wrapper,
	._rtos_create_thread = rtos_create_thread_wrapper,
	._rtos_delete_thread = rtos_delete_thread_wrapper,
	._rtos_get_free_heap_size = rtos_get_free_heap_size_wrapper,
	._rtos_deinit_queue = rtos_deinit_queue_wrapper,
	._rtos_init_queue = rtos_init_queue_wrapper,
	._rtos_deinit_mutex = rtos_deinit_mutex_wrapper,
	._rtos_init_mutex = rtos_init_mutex_wrapper,
	._rtos_lock_mutex = rtos_lock_mutex_wrapper,
	._rtos_unlock_mutex = rtos_unlock_mutex_wrapper,
	._rtos_reload_timer = rtos_reload_timer_wrapper,
	._rtos_is_timer_running = rtos_is_timer_running_wrapper,
	._rtos_stop_timer = rtos_stop_timer_wrapper,
	._rtos_init_timer = rtos_init_timer_wrapper,
	._rtos_is_current_thread = rtos_is_current_thread_wrapper,
	._register_wifi_dump_hook = register_wifi_dump_hook_wrapper,
	/// os str API function pointer
	._os_strdup = os_strdup_wrapper,
	._os_strcmp = os_strcmp_wrapper,
	._os_strchr = os_strchr_wrapper,
	._os_strlen = os_strlen_wrapper,
	._os_memcpy = os_memcpy_wrapper,
	._os_memmove = os_memmove_wrapper,
	._os_strlcpy = os_strlcpy_wrapper,
	._os_strncmp = os_strncmp_wrapper,
	._os_strcpy = os_strcpy_wrapper,
	._rtos_delay_milliseconds = rtos_delay_milliseconds_wrapper,
	._os_snprintf = os_snprintf,
	//._os_vsnprintf = os_vsnprintf_wrapper,
	._os_strncpy = os_strncpy_wrapper,
	._os_strtoul = os_strtoul_wrapper,
	._os_strcasecmp = os_strcasecmp_wrapper,
	._os_strncasecmp = os_strncasecmp_wrapper,
	._os_strrchr = os_strrchr_wrapper,
	._os_strstr = os_strstr_wrapper,
	/////
	._rf_pll_ctrl = rf_pll_ctrl_wrapper,
	._send_udp_bc_pkt = bk_airkiss_start_udp_boardcast_wrapper,
	._tx_verify_test_call_back = tx_verify_test_call_back_wrapper,
	._sys_hal_enter_low_analog = sys_hal_enter_low_analog_wrapper,
	._sys_hal_exit_low_analog = sys_hal_exit_low_analog_wrapper,
};

__attribute__((section(".dtcm_sec_data "))) wifi_os_variable_t g_wifi_os_variable = {
	._sys_drv_clk_on = SYS_DRV_CLK_ON,
	._sys_sys_debug_config1_addr = SYS_SYS_DEBUG_CONFIG1_ADDR,
	._ps_forbid_in_doze = PS_FORBID_IN_DOZE,
	._ps_forbid_txing = PS_FORBID_TXING,
	._ps_forbid_hw_timer = PS_FORBID_HW_TIMER,
	._ps_forbid_vif_prevent = PS_FORBID_VIF_PREVENT,
	._ps_forbid_keevt_on = PS_FORBID_KEEVT_ON,
	._ps_bmsg_iotcl_rf_timer_init = PS_BMSG_IOCTL_RF_PS_TIMER_INIT,
	._ps_bmsg_ioctl_rf_ps_timer_deint = PS_BMSG_IOCTL_RF_PS_TIMER_DEINIT,
	._ps_bmsg_ioctl_rf_enable = PS_BMSG_IOCTL_RF_ENABLE,
	._ps_bmsg_ioctl_mcu_enable = PS_BMSG_IOCTL_MCU_ENABLE,
	._ps_bmsg_ioctl_mcu_disable = PS_BMSG_IOCTL_MCU_DISANABLE,
	._ps_bmsg_ioctl_ps_enable = PS_BMSG_IOCTL_PS_ENABLE,
	._ps_bmsg_ioctl_ps_disable = PS_BMSG_IOCTL_PS_DISANABLE,
	._ps_bmsg_ioctl_exc32k_start = PS_BMSG_IOCTL_EXC32K_START,
	._ps_bmsg_ioctl_exc32k_stop = PS_BMSG_IOCTL_EXC32K_STOP,
	._pm_lpo_src_divd = PM_LPO_SRC_DIVD,
	._pm_lpo_src_x32k = PM_LPO_SRC_X32K,
	._pm_lpo_src_rosc = PM_LPO_SRC_ROSC,
	._pm_power_module_name_btsp = PM_POWER_MODULE_NAME_BTSP,
	._pm_power_module_name_wifip_mac = PM_POWER_MODULE_NAME_WIFIP_MAC,
	._pm_power_module_name_phy = PM_POWER_MODULE_NAME_PHY,
	._pm_power_module_state_off = PM_POWER_MODULE_STATE_OFF,
	._pm_power_module_state_on = PM_POWER_MODULE_STATE_ON,
	._low_power_xtal_dpll_stability_delay_time = LOW_POWER_XTAL_DPLL_STABILITY_DELAY_TIME,
	._low_power_delay_time_hardware = LOW_POWER_DELAY_TIME_HARDWARE,
	._low_power_26m_stability_delay_time_hardware = LOW_POWER_26M_STABILITY_DELAY_TIME_HARDWARE,
	._pm_power_sub_module_name_phy_wifi = PM_POWER_SUB_MODULE_NAME_PHY_WIFI,
	._pm_sleep_module_name_wifip_mac = PM_SLEEP_MODULE_NAME_WIFIP_MAC,
	._pm_dev_id_mac = PM_DEV_ID_MAC,
	._pm_cpu_frq_60m = PM_CPU_FRQ_60M,
	._pm_cpu_frq_80m = PM_CPU_FRQ_80M,
	._pm_cpu_frq_120m = PM_CPU_FRQ_120M,
	#if (CONFIG_SOC_BK7236XX)
	._pm_cpu_frq_high = PM_CPU_FRQ_480M,
	#else
	._pm_cpu_frq_high = PM_CPU_FRQ_320M,
	#endif
	._pm_cpu_frq_default = PM_CPU_FRQ_60M,
	._pm_32k_step_begin = PM_32K_STEP_BEGIN,
	._pm_32k_step_finish = PM_32K_STEP_FINISH,
	._cmd_rf_wifipll_hold_bit_set = CMD_RF_WIFIPLL_HOLD_BIT_SET,
	._cmd_rf_wifipll_hold_bit_clr = CMD_RF_WIFIPLL_HOLD_BIT_CLR,
	._rf_wifipll_hold_by_wifi_bit = RF_WIFIPLL_HOLD_BY_WIFI_BIT,
	._wifi_modem_en = WIFI_MODEM_EN,
	._wifi_modem_rc_en = WIFI_MODEM_RC_EN,
	._wifi_mac_tx_rx_timer_int_bit = WIFI_MAC_TX_RX_TIMER_INT_BIT,
	._wifi_mac_tx_rx_misc_int_bit = WIFI_MAC_TX_RX_MISC_INT_BIT,
	._wifi_mac_rx_trigger_int_bit = WIFI_MAC_RX_TRIGGER_INT_BIT,
	._wifi_mac_tx_trigger_int_bit = WIFI_MAC_TX_TRIGGER_INT_BIT,
	._wifi_mac_port_trigger_int_bit = WIFI_MAC_PORT_TRIGGER_INT_BIT,
	._wifi_mac_gen_int_bit = WIFI_MAC_GEN_INT_BIT,
	._wifi_hsu_interrupt_ctrl_bit = WIFI_HSU_INTERRUPT_CTRL_BIT,
	._wifi_mac_wakeup_int_bit = WIFI_MAC_WAKEUP_INT_BIT,
	._int_src_mac_general = INT_SRC_MAC_GENERAL,
	._int_src_mac_rx_trigger = INT_SRC_MAC_RX_TRIGGER,
	._int_src_mac_txrx_timer = INT_SRC_MAC_TXRX_TIMER,
	._int_src_mac_prot_trigger = INT_SRC_MAC_PROT_TRIGGER,
	._int_src_mac_tx_trigger = INT_SRC_MAC_TX_TRIGGER,
	._int_src_modem = INT_SRC_MODEM,
	._int_src_modem_rc = INT_SRC_MODEM_RC,
	#if CONFIG_IMPROVE_HE_TB_ENABLE
	._improve_he_tb_enable = true,
	#else
	._improve_he_tb_enable = false,
	#endif
};
