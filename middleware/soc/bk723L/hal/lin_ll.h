// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "lin_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIN_LL_REG_BASE   SOC_LIN_REG_BASE
#define LIN_GPIO_MAP \
{ \
	{{GPIO_0, GPIO_DEV_LIN_TXD}, {GPIO_1, GPIO_DEV_LIN_RXD}, {GPIO_2, GPIO_DEV_LIN_SLEEP}}, \
	{{GPIO_30, GPIO_DEV_LIN_RXD}, {GPIO_31, GPIO_DEV_LIN_TXD}, {GPIO_42, GPIO_DEV_LIN_SLEEP}}, \
	{{GPIO_40, GPIO_DEV_LIN_RXD}, {GPIO_41, GPIO_DEV_LIN_TXD}, {GPIO_42, GPIO_DEV_LIN_SLEEP}}, \
}

//reg deviceid:

static inline void lin_ll_set_deviceid_value(uint32_t v) {
	lin_deviceid_t *r = (lin_deviceid_t*)(SOC_LIN_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_deviceid_value(void) {
	lin_deviceid_t *r = (lin_deviceid_t*)(SOC_LIN_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t lin_ll_get_deviceid_deviceid(void) {
	lin_deviceid_t *r = (lin_deviceid_t*)(SOC_LIN_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg versionid:

static inline void lin_ll_set_versionid_value(uint32_t v) {
	lin_versionid_t *r = (lin_versionid_t*)(SOC_LIN_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_versionid_value(void) {
	lin_versionid_t *r = (lin_versionid_t*)(SOC_LIN_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t lin_ll_get_versionid_versionid(void) {
	lin_versionid_t *r = (lin_versionid_t*)(SOC_LIN_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg global_ctrl:

static inline void lin_ll_set_global_ctrl_value(uint32_t v) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_global_ctrl_value(void) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void lin_ll_set_global_ctrl_soft_rst(uint32_t v) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	r->soft_rst = v;
}

static inline uint32_t lin_ll_get_global_ctrl_soft_rst(void) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	return r->soft_rst;
}

static inline void lin_ll_set_global_ctrl_bypass_cfg(uint32_t v) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	r->bypass_cfg = v;
}

static inline uint32_t lin_ll_get_global_ctrl_bypass_cfg(void) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	return r->bypass_cfg;
}

static inline void lin_ll_set_global_ctrl_master(uint32_t v) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	r->lin_master = v;
}

static inline uint32_t lin_ll_get_global_ctrl_master(void) {
	lin_global_ctrl_t *r = (lin_global_ctrl_t*)(SOC_LIN_REG_BASE + (0x2 << 2));
	return r->lin_master;
}

//reg global_status:

static inline void lin_ll_set_global_status_value(uint32_t v) {
	lin_global_status_t *r = (lin_global_status_t*)(SOC_LIN_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_global_status_value(void) {
	lin_global_status_t *r = (lin_global_status_t*)(SOC_LIN_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t lin_ll_get_global_status_global_status(void) {
	lin_global_status_t *r = (lin_global_status_t*)(SOC_LIN_REG_BASE + (0x3 << 2));
	return r->global_status;
}

//reg data0:

static inline void lin_ll_set_data_value(uint32_t index, uint32_t v) {
	lin_data_t *r = (lin_data_t*)(SOC_LIN_REG_BASE + ((0x20 + index) << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_data_value(uint32_t index) {
	lin_data_t *r = (lin_data_t*)(SOC_LIN_REG_BASE + ((0x20 + index) << 2));
	return r->v;
}

static inline void lin_ll_set_data_byte(uint32_t index, uint8_t v) {
	lin_data_t *r = (lin_data_t*)(SOC_LIN_REG_BASE + ((0x20 + index) << 2));
	r->byte = v;
}

static inline uint8_t lin_ll_get_data_byte(uint32_t index) {
	lin_data_t *r = (lin_data_t*)(SOC_LIN_REG_BASE + ((0x20 + index) << 2));
	return r->byte;
}

//reg ctrl:

static inline void lin_ll_set_ctrl_value(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_ctrl_value(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline void lin_ll_set_ctrl_start_req(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->start_req = v;
}

static inline uint32_t lin_ll_get_ctrl_start_req(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->start_req;
}

static inline void lin_ll_set_ctrl_wakeup(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->wakeup = v;
}

static inline uint32_t lin_ll_get_ctrl_wakeup(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->wakeup;
}

static inline void lin_ll_set_ctrl_reset_error(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->reset_error = v;
}

static inline uint32_t lin_ll_get_ctrl_reset_error(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->reset_error;
}

static inline void lin_ll_set_ctrl_reset_int(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->reset_int = v;
}

static inline uint32_t lin_ll_get_ctrl_reset_int(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->reset_int;
}

static inline void lin_ll_set_ctrl_data_ack(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->data_ack = v;
}

static inline uint32_t lin_ll_get_ctrl_data_ack(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->data_ack;
}

static inline void lin_ll_set_ctrl_transmit(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->transmit = v;
}

static inline uint32_t lin_ll_get_ctrl_transmit(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->transmit;
}

static inline void lin_ll_set_ctrl_sleep(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->sleep = v;
}

static inline uint32_t lin_ll_get_ctrl_sleep(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->sleep;
}

static inline void lin_ll_set_ctrl_stop(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->stop = v;
}

static inline uint32_t lin_ll_get_ctrl_stop(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->stop;
}

static inline void lin_ll_set_ctrl_reserved_8_31(uint32_t v) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	r->reserved_8_31 = v;
}

static inline uint32_t lin_ll_get_ctrl_reserved_8_31(void) {
	lin_ctrl_t *r = (lin_ctrl_t*)(SOC_LIN_REG_BASE + (0x28 << 2));
	return r->reserved_8_31;
}

//reg status:

static inline void lin_ll_set_status_value(uint32_t v) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_status_value(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->v;
}

static inline uint32_t lin_ll_get_status_complete(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->complete;
}

static inline uint32_t lin_ll_get_status_wake_up(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->wake_up;
}

static inline uint32_t lin_ll_get_status_error(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->error;
}

static inline uint32_t lin_ll_get_status_int(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->intr;
}

static inline uint32_t lin_ll_get_status_data_req(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->data_req;
}

static inline uint32_t lin_ll_get_status_aborted(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->aborted;
}

static inline uint32_t lin_ll_get_status_bus_idle_timeout(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->bus_idle_timeout;
}

static inline uint32_t lin_ll_get_status_lin_active(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->lin_active;
}

static inline void lin_ll_set_status_reserved_8_31(uint32_t v) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	r->reserved_8_31 = v;
}

static inline uint32_t lin_ll_get_status_reserved_8_31(void) {
	lin_status_t *r = (lin_status_t*)(SOC_LIN_REG_BASE + (0x29 << 2));
	return r->reserved_8_31;
}

//reg err:

static inline void lin_ll_set_err_value(uint32_t v) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_err_value(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->v;
}

static inline uint32_t lin_ll_get_err_bit(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->bit;
}

static inline uint32_t lin_ll_get_err_chk(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->chk;
}

static inline uint32_t lin_ll_get_err_timeout(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->timeout;
}

static inline uint32_t lin_ll_get_err_parity(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->parity;
}

static inline void lin_ll_set_err_reserved_4_31(uint32_t v) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	r->reserved_4_31 = v;
}

static inline uint32_t lin_ll_get_err_reserved_4_31(void) {
	lin_err_t *r = (lin_err_t*)(SOC_LIN_REG_BASE + (0x2a << 2));
	return r->reserved_4_31;
}

//reg type:

static inline void lin_ll_set_type_value(uint32_t v) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_type_value(void) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline void lin_ll_set_type_data_length(uint32_t v) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	r->data_length = v;
}

static inline uint32_t lin_ll_get_type_data_length(void) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	return r->data_length;
}

static inline void lin_ll_set_type_reserved_4_6(uint32_t v) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	r->reserved_4_6 = v;
}

static inline uint32_t lin_ll_get_type_reserved_4_6(void) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	return r->reserved_4_6;
}

static inline void lin_ll_set_type_enh_check(uint32_t v) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	r->enh_check = v;
}

static inline uint32_t lin_ll_get_type_enh_check(void) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	return r->enh_check;
}

static inline void lin_ll_set_type_reserved_8_31(uint32_t v) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	r->reserved_8_31 = v;
}

static inline uint32_t lin_ll_get_type_reserved_8_31(void) {
	lin_type_t *r = (lin_type_t*)(SOC_LIN_REG_BASE + (0x2b << 2));
	return r->reserved_8_31;
}

//reg btcfg0:

static inline void lin_ll_set_btcfg0_value(uint32_t v) {
	lin_btcfg0_t *r = (lin_btcfg0_t*)(SOC_LIN_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_btcfg0_value(void) {
	lin_btcfg0_t *r = (lin_btcfg0_t*)(SOC_LIN_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline void lin_ll_set_btcfg0_bt_div1(uint32_t v) {
	lin_btcfg0_t *r = (lin_btcfg0_t*)(SOC_LIN_REG_BASE + (0x2c << 2));
	r->bt_div1 = v;
}

static inline uint32_t lin_ll_get_btcfg0_bt_div1(void) {
	lin_btcfg0_t *r = (lin_btcfg0_t*)(SOC_LIN_REG_BASE + (0x2c << 2));
	return r->bt_div1;
}

//reg btcfg1:

static inline void lin_ll_set_btcfg1_value(uint32_t v) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_btcfg1_value(void) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	return r->v;
}

static inline void lin_ll_set_btcfg1_bt_div2(uint32_t v) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	r->bt_div2 = v;
}

static inline uint32_t lin_ll_get_btcfg1_bt_div2(void) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	return r->bt_div2;
}

static inline void lin_ll_set_btcfg1_bt_mul(uint32_t v) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	r->bt_mul = v;
}

static inline uint32_t lin_ll_get_btcfg1_bt_mul(void) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	return r->bt_mul;
}

static inline void lin_ll_set_btcfg1_prescl1(uint32_t v) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	r->prescl1 = v;
}

static inline uint32_t lin_ll_get_btcfg1_prescl1(void) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	return r->prescl1;
}

static inline void lin_ll_set_btcfg1_reserved_8_31(uint32_t v) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	r->reserved_8_31 = v;
}

static inline uint32_t lin_ll_get_btcfg1_reserved_8_31(void) {
	lin_btcfg1_t *r = (lin_btcfg1_t*)(SOC_LIN_REG_BASE + (0x2d << 2));
	return r->reserved_8_31;
}

//reg ident:

static inline void lin_ll_set_ident_value(uint32_t v) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_ident_value(void) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline void lin_ll_set_ident_id(uint32_t v) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	r->id = v;
}

static inline uint32_t lin_ll_get_ident_id(void) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	return r->id;
}

static inline void lin_ll_set_ident_tran_dir(uint32_t v) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	r->tran_dir = v;
}

static inline uint32_t lin_ll_get_ident_tran_dir(void) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	return r->tran_dir;
}


static inline void lin_ll_set_ident_reserved_6_31(uint32_t v) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	r->reserved_6_31 = v;
}

static inline uint32_t lin_ll_get_ident_reserved_6_31(void) {
	lin_ident_t *r = (lin_ident_t*)(SOC_LIN_REG_BASE + (0x2e << 2));
	return r->reserved_6_31;
}

//reg tcfg:

static inline void lin_ll_set_tcfg_value(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t lin_ll_get_tcfg_value(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline void lin_ll_set_tcfg_wup_repeat_time(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->wup_repeat_time = v;
}

static inline uint32_t lin_ll_get_tcfg_wup_repeat_time(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->wup_repeat_time;
}

static inline void lin_ll_set_tcfg_bus_inactivity_time(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->bus_inactivity_time = v;
}

static inline uint32_t lin_ll_get_tcfg_bus_inactivity_time(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->bus_inactivity_time;
}

static inline void lin_ll_set_tcfg_reserved_4_5(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->reserved_4_5 = v;
}

static inline uint32_t lin_ll_get_tcfg_reserved_4_5(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->reserved_4_5;
}

static inline void lin_ll_set_tcfg_prescl2(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->prescl2 = v;
}

static inline uint32_t lin_ll_get_tcfg_prescl2(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->prescl2;
}

static inline void lin_ll_set_tcfg_reserved_7_31(uint32_t v) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	r->reserved_7_31 = v;
}

static inline uint32_t lin_ll_get_tcfg_reserved_7_31(void) {
	lin_tcfg_t *r = (lin_tcfg_t*)(SOC_LIN_REG_BASE + (0x2f << 2));
	return r->reserved_7_31;
}

#if (CONFIG_LIN_PM_CB_SUPPORT)
#define LIN_PM_BACKUP_REG_NUM    2

static inline void lin_ll_backup(uint32_t *pm_backup)
{
	pm_backup[0] |= (REG_READ(SOC_LIN_REG_BASE + (0x2 << 2)) & 0xff);
	pm_backup[0] |= (REG_READ(SOC_LIN_REG_BASE + (0x28 << 2)) & 0xff) << 8;
	pm_backup[0] |= (REG_READ(SOC_LIN_REG_BASE + (0x2b << 2)) & 0xff) << 16;
	pm_backup[0] |= (REG_READ(SOC_LIN_REG_BASE + (0x2c << 2)) & 0xff) << 24;
	pm_backup[1] |= (REG_READ(SOC_LIN_REG_BASE + (0x2d << 2)) & 0xff);
	pm_backup[1] |= (REG_READ(SOC_LIN_REG_BASE + (0x2e << 2)) & 0xff) << 8;
	pm_backup[1] |= (REG_READ(SOC_LIN_REG_BASE + (0x2f << 2)) & 0xff) << 16;
}

static inline void lin_ll_restore(uint32_t *pm_backup)
{
	REG_WRITE(SOC_LIN_REG_BASE + (0x2 << 2), (pm_backup[0]) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x28 << 2), (pm_backup[0] >> 8) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x2b << 2), (pm_backup[0] >> 16) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x2c << 2), (pm_backup[0] >> 24) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x2d << 2), (pm_backup[1]) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x2e << 2), (pm_backup[1] >> 8) & 0xff);
	REG_WRITE(SOC_LIN_REG_BASE + (0x2f << 2), (pm_backup[1] >> 16) & 0xff);
}
#endif

#ifdef __cplusplus
}
#endif
