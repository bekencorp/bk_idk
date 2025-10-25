// Copyright 2022-2025 Beken
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
#include "trx_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRX_LL_REG_BASE   SOC_TRX_REG_BASE

//reg :
static inline void trx_ll_set__value(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get__value(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline void trx_ll_set__nc_0_31(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x0 << 2));
	r->nc_0_31 = v;
}

static inline uint32_t trx_ll_get__nc_0_31(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x0 << 2));
	return r->nc_0_31;
}

//reg controlling the voltage of calibration:

static inline void trx_ll_set_controlling the voltage of calibration_value(uint32_t v) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_value(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_bypass_opt(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->bypass_opt;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_manual_band(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->manual_band;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_icpoffset_tx(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->icpoffset_tx;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_itune_mixer(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->itune_mixer;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_icpoffset_rx(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->icpoffset_rx;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_ctune_lpf(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->ctune_lpf;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_caldone_spi(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->caldone_spi;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_bypass_caldone(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->bypass_caldone;
}

static inline uint32_t trx_ll_get_controlling the voltage of calibration_vctrl_cal(void) {
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t*)(SOC_TRX_REG_BASE + (0x1 << 2));
	return r->vctrl_cal;
}

//reg enable vco in the top by spi:

static inline void trx_ll_set_enable vco in the top by spi_value(uint32_t v) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_value(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_ck2xen(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->ck2xen;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_clksel_cal(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->clksel_cal;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_bandcal_spi(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->bandcal_spi;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_band_spi_trgger(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->band_spi_trgger;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_icp_bias_com(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->icp_bias_com;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_hvref(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->hvref;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_lvref(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->lvref;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_done_delay(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->done_delay;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_amptrigger(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->amptrigger;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_ampautocal(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->ampautocal;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_ampcal_en(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->ampcal_en;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_ampctrl_m(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->ampctrl_m;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_errdet_en(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->errdet_en;
}

static inline uint32_t trx_ll_get_enable vco in the top by spi_errdet_spien(void) {
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t*)(SOC_TRX_REG_BASE + (0x2 << 2));
	return r->errdet_spien;
}

//reg controlling the bias current of vco:

static inline void trx_ll_set_controlling the bias current of vco_value(uint32_t v) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_value(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline void trx_ll_set_controlling the bias current of vco_ctune_mixer_ch(uint32_t v) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	r->ctune_mixer_ch = v;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_ctune_mixer(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->ctune_mixer;
}

static inline void trx_ll_set_controlling the bias current of vco_ampselu(uint32_t v) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	r->ampselu = v;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_ampselu(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->ampselu;
}

static inline void trx_ll_set_controlling the bias current of vco_icpbias_rx(uint32_t v) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	r->icpbias_rx = v;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_icpbias_rx(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->icpbias_rx;
}

static inline void trx_ll_set_controlling the bias current of vco_icpbias_tx(uint32_t v) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	r->icpbias_tx = v;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_icpbias_tx(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->icpbias_tx;
}

static inline uint32_t trx_ll_get_controlling the bias current of vco_itune_vco_spi(void) {
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t*)(SOC_TRX_REG_BASE + (0x3 << 2));
	return r->itune_vco_spi;
}

//reg loop int mode enable:

static inline void trx_ll_set_loop int mode enable_value(uint32_t v) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_loop int mode enable_value(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_loop int mode enable_icpbias_rx(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->icpbias_rx;
}

static inline uint32_t trx_ll_get_loop int mode enable_ampsel(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->ampsel;
}

static inline uint32_t trx_ll_get_loop int mode enable_bypass_aac(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->bypass_aac;
}

static inline uint32_t trx_ll_get_loop int mode enable_reset_spi(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->reset_spi;
}

static inline uint32_t trx_ll_get_loop int mode enable_nrsten(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->nrsten;
}

static inline uint32_t trx_ll_get_loop int mode enable_tristate_spi(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->tristate_spi;
}

static inline void trx_ll_set_loop int mode enable_selvcopol(uint32_t v) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	r->selvcopol = v;
}

static inline uint32_t trx_ll_get_loop int mode enable_selvcopol(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->selvcopol;
}

static inline uint32_t trx_ll_get_loop int mode enable_nint(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->nint;
}

static inline uint32_t trx_ll_get_loop int mode enable_ib1v_lomixer(void) {
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t*)(SOC_TRX_REG_BASE + (0x4 << 2));
	return r->ib1v_lomixer;
}

//reg set the frequency channel by spi:

static inline void trx_ll_set_set the frequency channel by spi_value(uint32_t v) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_value(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_cksel_sdm(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->cksel_sdm;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_vctrl_cal(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->vctrl_cal;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_pnen(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->pnen;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_sdm_rstn(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->sdm_rstn;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_vctrl_cal(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->vctrl_cal;
}

static inline uint32_t trx_ll_get_set the frequency channel by spi_chspi(void) {
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t*)(SOC_TRX_REG_BASE + (0x5 << 2));
	return r->chspi;
}

//reg dcoc i path input:

static inline void trx_ll_set_dcoc i path input_value(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_value(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void trx_ll_set_dcoc i path input_nc_1_1(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->nc_1_1 = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_nc_1_1(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->nc_1_1;
}

static inline void trx_ll_set_dcoc i path input_rx3s(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->rx3s = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_rx3s(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->rx3s;
}

static inline void trx_ll_set_dcoc i path input_nc_3_3(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->nc_3_3 = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_nc_3_3(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->nc_3_3;
}

static inline void trx_ll_set_dcoc i path input_nc_4_9(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->nc_4_9 = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_nc_4_9(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->nc_4_9;
}

static inline void trx_ll_set_dcoc i path input_nc_10_15(uint32_t v) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	r->nc_10_15 = v;
}

static inline uint32_t trx_ll_get_dcoc i path input_nc_10_15(void) {
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t*)(SOC_TRX_REG_BASE + (0x6 << 2));
	return r->nc_10_15;
}

//reg lpf i path calibiration input (high 2bits):

static inline void trx_ll_set_lpf i path calibiration input (high 2bits)_value(uint32_t v) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_value(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_spilpfrxg(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->spilpfrxg;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_autorxifgen(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->autorxifgen;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_dcoc_ctl(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->dcoc_ctl;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_lpftxtest_g(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->lpftxtest_g;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_adc_ckinv(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->adc_ckinv;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_txif_2rd_g(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->txif_2rd_g;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_abws_en(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->abws_en;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_lpftxtest_b(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->lpftxtest_b;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_txif_2rd_b(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->txif_2rd_b;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_txif_2rd_ble(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->txif_2rd_ble;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_dac_isel_buf(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->dac_isel_buf;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_dac_isel_opa(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->dac_isel_opa;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_lpfcapcalq_h2b(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->lpfcapcalq_h2b;
}

static inline uint32_t trx_ll_get_lpf i path calibiration input (high 2bits)_lpfcapcali_h2b(void) {
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t*)(SOC_TRX_REG_BASE + (0x7 << 2));
	return r->lpfcapcali_h2b;
}

//reg rssi high threshold for wifi:

static inline void trx_ll_set_rssi high threshold for wifi_value(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_value(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void trx_ll_set_rssi high threshold for wifi_lotrigspi(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->lotrigspi = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_lotrigspi(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->lotrigspi;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rfpll_notch_res(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rfpll_notch_res;
}

static inline void trx_ll_set_rssi high threshold for wifi_en_wifipll(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->en_wifipll = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_grssi_b(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->grssi_b;
}

static inline void trx_ll_set_rssi high threshold for wifi_grssi_w(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->grssi_w = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_grssi_w(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->grssi_w;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssi_ibs(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssi_ibs;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssi_tsel(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssi_tsel;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssi_ten(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssi_ten;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssi_ble(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssi_ble;
}

static inline void trx_ll_set_rssi high threshold for wifi_rssitl(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->rssitl = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssitl(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssitl;
}

static inline void trx_ll_set_rssi high threshold for wifi_rssitm(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->rssitm = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssitm(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssitm;
}

static inline void trx_ll_set_rssi high threshold for wifi_rssith(uint32_t v) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	r->rssith = v;
}

static inline uint32_t trx_ll_get_rssi high threshold for wifi_rssith(void) {
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t*)(SOC_TRX_REG_BASE + (0x8 << 2));
	return r->rssith;
}

//reg lna load  capacitor tunning:

static inline void trx_ll_set_lna load  capacitor tunning_value(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_value(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_agcrxfeen(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->agcrxfeen;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_grxlnaspi(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->grxlnaspi;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_grxi2vspi(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->grxi2vspi;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_vbsrxlo(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->vbsrxlo;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_ibsrxi2v(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->ibsrxi2v;
}

static inline void trx_ll_set_lna load  capacitor tunning_vlofastsel(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->vlofastsel = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_vlofastsel(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->vlofastsel;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_enfsr(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->enfsr;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_glnar(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->glnar;
}

static inline void trx_ll_set_lna load  capacitor tunning_att_spi(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->att_spi = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_att_spi(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->att_spi;
}

static inline void trx_ll_set_lna load  capacitor tunning_inr(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->inr = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_inr(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->inr;
}

static inline void trx_ll_set_lna load  capacitor tunning_lglc(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->lglc = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_lglc(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->lglc;
}

static inline void trx_ll_set_lna load  capacitor tunning_r_tune(uint32_t v) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	r->r_tune = v;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_r_tune(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->r_tune;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_isrxref(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->isrxref;
}

static inline uint32_t trx_ll_get_lna load  capacitor tunning_cloadlna(void) {
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t*)(SOC_TRX_REG_BASE + (0x9 << 2));
	return r->cloadlna;
}

//reg msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17:

static inline void trx_ll_set_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_value(uint32_t v) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_value(void) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_dygs_vbnc(void) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	return r->dygs_vbnc;
}

static inline uint32_t trx_ll_get_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_pa__vbpas(void) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	return r->pa__vbpas;
}

static inline uint32_t trx_ll_get_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_pa_endyc(void) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	return r->pa_endyc;
}

static inline uint32_t trx_ll_get_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_pa_dibiasbg(void) {
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t*)(SOC_TRX_REG_BASE + (0xa << 2));
	return r->pa_dibiasbg;
}

//reg modulator output tuning cap programming: +18ff/lsb (low q nodes):

static inline void trx_ll_set_modulator output tuning cap programming: +18ff/lsb (low q nodes)_value(uint32_t v) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_value(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_padcd(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->padcd;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_padcc(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->padcc;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_padcb(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->padcb;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_padca(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->padca;
}

static inline void trx_ll_set_modulator output tuning cap programming: +18ff/lsb (low q nodes)_hvdhs(uint32_t v) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	r->hvdhs = v;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_hvdhs(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->hvdhs;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_dia(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->dia;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_dmodldosel(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->dmodldosel;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_pad_dics(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->pad_dics;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_pad_di(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->pad_di;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_pad_encapdy(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->pad_encapdy;
}

static inline void trx_ll_set_modulator output tuning cap programming: +18ff/lsb (low q nodes)_modca(uint32_t v) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	r->modca = v;
}

static inline uint32_t trx_ll_get_modulator output tuning cap programming: +18ff/lsb (low q nodes)_modca(void) {
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t*)(SOC_TRX_REG_BASE + (0xb << 2));
	return r->modca;
}

//reg (0: spi;  1: digital) power control enable:

static inline void trx_ll_set_(0: spi;  1: digital) power control enable_value(uint32_t v) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_value(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_gmgain(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->gmgain;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_padctrl(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->padctrl;
}

static inline void trx_ll_set_(0: spi;  1: digital) power control enable_pactrl(uint32_t v) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	r->pactrl = v;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_pactrl(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->pactrl;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_rgm(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->rgm;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_dgmdc(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->dgmdc;
}

static inline void trx_ll_set_(0: spi;  1: digital) power control enable_mixvswb(uint32_t v) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	r->mixvswb = v;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_mixvswb(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->mixvswb;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_mixvbh(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->mixvbh;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_pa_dibiaspt(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->pa_dibiaspt;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_pa_cap(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->pa_cap;
}

static inline void trx_ll_set_(0: spi;  1: digital) power control enable_pamapen(uint32_t v) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	r->pamapen = v;
}

static inline uint32_t trx_ll_get_(0: spi;  1: digital) power control enable_pamapen(void) {
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t*)(SOC_TRX_REG_BASE + (0xc << 2));
	return r->pamapen;
}

//reg msb of pa current bias temperature  setting , another bits at 0x00[0]:

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_value(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_value(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_ipatcs(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->ipatcs;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_tssiqrg(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->tssiqrg;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_pa_vref_vbgs(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->pa_vref_vbgs = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_pa_vref_vbgs(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->pa_vref_vbgs;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_pa_vb_vddgs(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->pa_vb_vddgs = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_pa_vb_vddgs(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->pa_vb_vddgs;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_tssiranges(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->tssiranges;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_en_pcal(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->en_pcal = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_en_pcal(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->en_pcal;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_en_iqcal(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->en_iqcal = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_en_iqcal(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->en_iqcal;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_itas(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->itas;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_rccalen(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->rccalen = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_rccalen(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->rccalen;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_rxlo ldo(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->rxlo ldo;
}

static inline void trx_ll_set_msb of pa current bias temperature  setting , another bits at 0x00[0]_hvdls(uint32_t v) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	r->hvdls = v;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_hvdls(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->hvdls;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_enpadcapbias(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->enpadcapbias;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_pad_capbiass(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->pad_capbiass;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_txlo ldo(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->txlo ldo;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_ldoda(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->ldoda;
}

static inline uint32_t trx_ll_get_msb of pa current bias temperature  setting , another bits at 0x00[0]_ipatcs(void) {
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t*)(SOC_TRX_REG_BASE + (0xd << 2));
	return r->ipatcs;
}

//reg nc:

static inline void trx_ll_set_nc_value(uint32_t v) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_nc_value(void) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_nc_dygs_vbnc_2(void) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	return r->dygs_vbnc_2;
}

static inline uint32_t trx_ll_get_nc_pa__vbpas_2(void) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	return r->pa__vbpas_2;
}

static inline uint32_t trx_ll_get_nc_pa_endyc_2(void) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	return r->pa_endyc_2;
}

static inline void trx_ll_set_nc_pa_encsdyh_2(uint32_t v) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	r->pa_encsdyh_2 = v;
}

static inline uint32_t trx_ll_get_nc_pa_encsdyh_2(void) {
	trx_nc_t *r = (trx_nc_t*)(SOC_TRX_REG_BASE + (0xe << 2));
	return r->pa_encsdyh_2;
}

//reg 0 = rest; 1=normal work:

static inline void trx_ll_set_0 = rest; 1=normal work_value(uint32_t v) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_value(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_iops(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->iops;
}

static inline void trx_ll_set_0 = rest; 1=normal work_fictl(uint32_t v) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	r->fictl = v;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_fictl(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->fictl;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_cgmbs(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->cgmbs;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_enclip_flt(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->enclip_flt;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_enfsr_flt(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->enfsr_flt;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_adc_isel(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->adc_isel;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_dlypc_i(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->dlypc_i;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_tdtune_i(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->tdtune_i;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_tdtuneb(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->tdtuneb;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_calen(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->calen;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_calnsel(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->calnsel;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_ckmode(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->ckmode;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_vrefs(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->vrefs;
}

static inline void trx_ll_set_0 = rest; 1=normal work_adc_rstn(uint32_t v) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	r->adc_rstn = v;
}

static inline uint32_t trx_ll_get_0 = rest; 1=normal work_adc_rstn(void) {
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t*)(SOC_TRX_REG_BASE + (0x12 << 2));
	return r->adc_rstn;
}

//reg cell delay control spi value:

static inline void trx_ll_set_cell delay control spi value_value(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_value(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_atsel(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->atsel;
}

static inline uint32_t trx_ll_get_cell delay control spi value_entst(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->entst;
}

static inline void trx_ll_set_cell delay control spi value_nc_5_5(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->nc_5_5 = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_nc_5_5(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->nc_5_5;
}

static inline void trx_ll_set_cell delay control spi value_lovctrl_testen(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->lovctrl_testen = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_lovctrl_testen(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->lovctrl_testen;
}

static inline void trx_ll_set_cell delay control spi value_lovctrlsel(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->lovctrlsel = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_lovctrlsel(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->lovctrlsel;
}

static inline void trx_ll_set_cell delay control spi value_trsw_bt_mode(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->trsw_bt_mode = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_trsw_bt_mode(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->trsw_bt_mode;
}

static inline void trx_ll_set_cell delay control spi value_btpll_en_spi(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->btpll_en_spi = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_btpll_en_spi(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->btpll_en_spi;
}

static inline void trx_ll_set_cell delay control spi value_nc_10_10(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->nc_10_10 = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_nc_10_10(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->nc_10_10;
}

static inline void trx_ll_set_cell delay control spi value_t0r1_spi(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->t0r1_spi = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_t0r1_spi(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->t0r1_spi;
}

static inline void trx_ll_set_cell delay control spi value_lotrigctr(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->lotrigctr = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_lotrigctr(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->lotrigctr;
}

static inline void trx_ll_set_cell delay control spi value_losel_rx(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->losel_rx = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_losel_rx(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->losel_rx;
}

static inline void trx_ll_set_cell delay control spi value_loselvctr(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->loselvctr = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_loselvctr(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->loselvctr;
}

static inline void trx_ll_set_cell delay control spi value_losel0dig(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->losel0dig = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_losel0dig(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->losel0dig;
}

static inline void trx_ll_set_cell delay control spi value_ckmode_pwd(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->ckmode_pwd = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_ckmode_pwd(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->ckmode_pwd;
}

static inline void trx_ll_set_cell delay control spi value_vbias_adc(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->vbias_adc = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_vbias_adc(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->vbias_adc;
}

static inline uint32_t trx_ll_get_cell delay control spi value_dlypc_i(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->dlypc_i;
}

static inline void trx_ll_set_cell delay control spi value_nc_25_25(uint32_t v) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	r->nc_25_25 = v;
}

static inline uint32_t trx_ll_get_cell delay control spi value_nc_25_25(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->nc_25_25;
}

static inline uint32_t trx_ll_get_cell delay control spi value_tdtune_q(void) {
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t*)(SOC_TRX_REG_BASE + (0x13 << 2));
	return r->tdtune_q;
}

//reg clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge:

static inline void trx_ll_set_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_value(uint32_t v) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_value(void) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void trx_ll_set_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_nc_1_1(uint32_t v) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	r->nc_1_1 = v;
}

static inline uint32_t trx_ll_get_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_nc_1_1(void) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	return r->nc_1_1;
}

static inline void trx_ll_set_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_wifi_loen(uint32_t v) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	r->wifi_loen = v;
}

static inline uint32_t trx_ll_get_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_wifi_loen(void) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	return r->wifi_loen;
}

static inline uint32_t trx_ll_get_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_rxlo(void) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	return r->rxlo;
}

static inline uint32_t trx_ll_get_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_ck2sdmpol_sel(void) {
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t*)(SOC_TRX_REG_BASE + (0x14 << 2));
	return r->ck2sdmpol_sel;
}

//reg :

static inline void trx_ll_set__value(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get__value(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get__reset_nload(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->reset_nload;
}

static inline uint32_t trx_ll_get__tristate(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->tristate;
}

static inline uint32_t trx_ll_get__selpol(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->selpol;
}

static inline void trx_ll_set__nc_3_7(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->nc_3_7 = v;
}

static inline uint32_t trx_ll_get__nc_3_7(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->nc_3_7;
}

static inline void trx_ll_set__cp1_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->cp1_rx = v;
}

static inline uint32_t trx_ll_get__cp1_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->cp1_rx;
}

static inline void trx_ll_set__rp3_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->rp3_rx = v;
}

static inline uint32_t trx_ll_get__rp3_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->rp3_rx;
}

static inline void trx_ll_set__rp2_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->rp2_rx = v;
}

static inline uint32_t trx_ll_get__rp2_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->rp2_rx;
}

static inline void trx_ll_set__rp1_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->rp1_rx = v;
}

static inline uint32_t trx_ll_get__rp1_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->rp1_rx;
}

static inline void trx_ll_set__ioffset_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->ioffset_rx = v;
}

static inline uint32_t trx_ll_get__ioffset_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->ioffset_rx;
}

static inline void trx_ll_set__icp_rx(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->icp_rx = v;
}

static inline uint32_t trx_ll_get__icp_rx(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->icp_rx;
}

static inline void trx_ll_set__ck_nload_edgesel(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	r->ck_nload_edgesel = v;
}

static inline uint32_t trx_ll_get__ck_nload_edgesel(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x15 << 2));
	return r->ck_nload_edgesel;
}

//reg band calibration optimize enable.:

static inline void trx_ll_set_band calibration optimize enable._value(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._value(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void trx_ll_set_band calibration optimize enable._vreflctrl(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->vreflctrl = v;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._vreflctrl(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->vreflctrl;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._vrefhctrl(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->vrefhctrl;
}

static inline void trx_ll_set_band calibration optimize enable._reset_ncounter(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->reset_ncounter = v;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._reset_ncounter(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->reset_ncounter;
}

static inline void trx_ll_set_band calibration optimize enable._nc_5_28(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->nc_5_28 = v;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._nc_5_28(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->nc_5_28;
}

static inline void trx_ll_set_band calibration optimize enable._pwd5g(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->pwd5g = v;
}

static inline void trx_ll_set_band calibration optimize enable._pwd2g(uint32_t v) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	r->pwd2g = v;
}

static inline uint32_t trx_ll_get_band calibration optimize enable._bp_opt(void) {
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t*)(SOC_TRX_REG_BASE + (0x16 << 2));
	return r->bp_opt;
}

//reg n value spi control:

static inline void trx_ll_set_n value spi control_value(uint32_t v) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_n value spi control_value(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get_n value spi control_rctrl(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->rctrl;
}

static inline void trx_ll_set_n value spi control_pwd_vcoampcal(uint32_t v) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	r->pwd_vcoampcal = v;
}

static inline uint32_t trx_ll_get_n value spi control_pwd_vcoampcal(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->pwd_vcoampcal;
}

static inline uint32_t trx_ll_get_n value spi control_rxloctrl_vcocal(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->rxloctrl_vcocal;
}

static inline uint32_t trx_ll_get_n value spi control_done_delay(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->done_delay;
}

static inline void trx_ll_set_n value spi control_spi_trigger(uint32_t v) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	r->spi_trigger = v;
}

static inline uint32_t trx_ll_get_n value spi control_spi_trigger(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->spi_trigger;
}

static inline uint32_t trx_ll_get_n value spi control_errdet_spien(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->errdet_spien;
}

static inline uint32_t trx_ll_get_n value spi control_ckref_loop_sel(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->ckref_loop_sel;
}

static inline uint32_t trx_ll_get_n value spi control_ckref_nl_sel(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->ckref_nl_sel;
}

static inline uint32_t trx_ll_get_n value spi control_nrsten(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->nrsten;
}

static inline uint32_t trx_ll_get_n value spi control_int_mod(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->int_mod;
}

static inline uint32_t trx_ll_get_n value spi control_band_spi(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->band_spi;
}

static inline uint32_t trx_ll_get_n value spi control_nmanual(void) {
	trx_n value spi control_t *r = (trx_n value spi control_t*)(SOC_TRX_REG_BASE + (0x17 << 2));
	return r->nmanual;
}

//reg vco amplitude auto calibration threshold control..:

static inline void trx_ll_set_vco amplitude auto calibration threshold control.._value(uint32_t v) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get_vco amplitude auto calibration threshold control.._value(void) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline void trx_ll_set_vco amplitude auto calibration threshold control.._nc_0_19(uint32_t v) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	r->nc_0_19 = v;
}

static inline uint32_t trx_ll_get_vco amplitude auto calibration threshold control.._nc_0_19(void) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	return r->nc_0_19;
}

static inline uint32_t trx_ll_get_vco amplitude auto calibration threshold control.._vcobias_men(void) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	return r->vcobias_men;
}

static inline uint32_t trx_ll_get_vco amplitude auto calibration threshold control.._vcobias_spi(void) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	return r->vcobias_spi;
}

static inline uint32_t trx_ll_get_vco amplitude auto calibration threshold control.._vcoampdet(void) {
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t*)(SOC_TRX_REG_BASE + (0x18 << 2));
	return r->vcoampdet;
}

//reg :

static inline void trx_ll_set__value(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get__value(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline uint32_t trx_ll_get__bwctrl(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->bwctrl;
}

static inline void trx_ll_set__vcohpen(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	r->vcohpen = v;
}

static inline uint32_t trx_ll_get__vcohpen(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->vcohpen;
}

static inline void trx_ll_set__nc_5_12(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	r->nc_5_12 = v;
}

static inline uint32_t trx_ll_get__nc_5_12(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->nc_5_12;
}

static inline uint32_t trx_ll_get__vcal_sel(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->vcal_sel;
}

static inline uint32_t trx_ll_get__vsel1v_vco(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->vsel1v_vco;
}

static inline uint32_t trx_ll_get__vsel1v_pll(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->vsel1v_pll;
}

static inline uint32_t trx_ll_get__vsel1v_cp(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->vsel1v_cp;
}

static inline void trx_ll_set__ck26m_40m_sel(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	r->ck26m_40m_sel = v;
}

static inline uint32_t trx_ll_get__ck26m_40m_sel(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x19 << 2));
	return r->ck26m_40m_sel;
}

//reg :

static inline void trx_ll_set__value(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t trx_ll_get__value(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline void trx_ll_set__nc_0_31(uint32_t v) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x1a << 2));
	r->nc_0_31 = v;
}

static inline uint32_t trx_ll_get__nc_0_31(void) {
	trx__t *r = (trx__t*)(SOC_TRX_REG_BASE + (0x1a << 2));
	return r->nc_0_31;
}
#ifdef __cplusplus
}
#endif
