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

#include "hal_config.h"
#include "trx_hw.h"
#include "trx_hal.h"

typedef void (*trx_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	trx_dump_fn_t fn;
} trx_reg_fn_map_t;

static void trx_dump_(void)
{
	SOC_LOGI(": %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x0 << 2)));
}

static void trx_dump_controlling the voltage of calibration(void)
{
	trx_controlling the voltage of calibration_t *r = (trx_controlling the voltage of calibration_t *)(SOC_TRX_REG_BASE + (0x1 << 2));

	SOC_LOGI("controlling the voltage of calibration: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x1 << 2)));
	SOC_LOGI("	bypass_opt: %8x\r\n", r->bypass_opt);
	SOC_LOGI("	manual_band: %8x\r\n", r->manual_band);
	SOC_LOGI("	bp_ibias: %8x\r\n", r->bp_ibias);
	SOC_LOGI("	vsel1v_vcoldo: %8x\r\n", r->vsel1v_vcoldo);
	SOC_LOGI("	edgesel_nck: %8x\r\n", r->edgesel_nck);
	SOC_LOGI("	icpoffset_tx: %8x\r\n", r->icpoffset_tx);
	SOC_LOGI("	itune_mixer: %8x\r\n", r->itune_mixer);
	SOC_LOGI("	icpoffset_rx: %8x\r\n", r->icpoffset_rx);
	SOC_LOGI("	ctune_lpf: %8x\r\n", r->ctune_lpf);
	SOC_LOGI("	nload_dlyen: %8x\r\n", r->nload_dlyen);
	SOC_LOGI("	caldone_spi: %8x\r\n", r->caldone_spi);
	SOC_LOGI("	bypass_caldone: %8x\r\n", r->bypass_caldone);
	SOC_LOGI("	vctrl_cal: %8x\r\n", r->vctrl_cal);
}

static void trx_dump_enable vco in the top by spi(void)
{
	trx_enable vco in the top by spi_t *r = (trx_enable vco in the top by spi_t *)(SOC_TRX_REG_BASE + (0x2 << 2));

	SOC_LOGI("enable vco in the top by spi: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	ck2xen: %8x\r\n", r->ck2xen);
	SOC_LOGI("	clksel_cal: %8x\r\n", r->clksel_cal);
	SOC_LOGI("	bandcal_spi: %8x\r\n", r->bandcal_spi);
	SOC_LOGI("	band_spi_trgger: %8x\r\n", r->band_spi_trgger);
	SOC_LOGI("	icp_bias_com: %8x\r\n", r->icp_bias_com);
	SOC_LOGI("	hvref: %8x\r\n", r->hvref);
	SOC_LOGI("	lvref: %8x\r\n", r->lvref);
	SOC_LOGI("	done_delay: %8x\r\n", r->done_delay);
	SOC_LOGI("	amptrigger: %8x\r\n", r->amptrigger);
	SOC_LOGI("	ampautocal: %8x\r\n", r->ampautocal);
	SOC_LOGI("	ampcal_en: %8x\r\n", r->ampcal_en);
	SOC_LOGI("	ampctrl_m: %8x\r\n", r->ampctrl_m);
	SOC_LOGI("	errdet_en: %8x\r\n", r->errdet_en);
	SOC_LOGI("	errdet_spien: %8x\r\n", r->errdet_spien);
	SOC_LOGI("	vsel1v_pllldo: %8x\r\n", r->vsel1v_pllldo);
	SOC_LOGI("	pllen: %8x\r\n", r->pllen);
	SOC_LOGI("	vcoen: %8x\r\n", r->vcoen);
}

static void trx_dump_controlling the bias current of vco(void)
{
	trx_controlling the bias current of vco_t *r = (trx_controlling the bias current of vco_t *)(SOC_TRX_REG_BASE + (0x3 << 2));

	SOC_LOGI("controlling the bias current of vco: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x3 << 2)));
	SOC_LOGI("	ctune_mixer_ch: %8x\r\n", r->ctune_mixer_ch);
	SOC_LOGI("	ctune_mixer: %8x\r\n", r->ctune_mixer);
	SOC_LOGI("	ampselu: %8x\r\n", r->ampselu);
	SOC_LOGI("	icpbias_rx: %8x\r\n", r->icpbias_rx);
	SOC_LOGI("	icpbias_tx: %8x\r\n", r->icpbias_tx);
	SOC_LOGI("	itune_vco_spi: %8x\r\n", r->itune_vco_spi);
}

static void trx_dump_loop int mode enable(void)
{
	trx_loop int mode enable_t *r = (trx_loop int mode enable_t *)(SOC_TRX_REG_BASE + (0x4 << 2));

	SOC_LOGI("loop int mode enable: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	icpbias_rx: %8x\r\n", r->icpbias_rx);
	SOC_LOGI("	icpbias_tx: %8x\r\n", r->icpbias_tx);
	SOC_LOGI("	ampsel: %8x\r\n", r->ampsel);
	SOC_LOGI("	bypass_aac: %8x\r\n", r->bypass_aac);
	SOC_LOGI("	reset_spi: %8x\r\n", r->reset_spi);
	SOC_LOGI("	nrsten: %8x\r\n", r->nrsten);
	SOC_LOGI("	tristate_spi: %8x\r\n", r->tristate_spi);
	SOC_LOGI("	selvcopol: %8x\r\n", r->selvcopol);
	SOC_LOGI("	nclken_spi: %8x\r\n", r->nclken_spi);
	SOC_LOGI("	nint: %8x\r\n", r->nint);
	SOC_LOGI("	ib1v_lomixer: %8x\r\n", r->ib1v_lomixer);
	SOC_LOGI("	int_mode: %8x\r\n", r->int_mode);
}

static void trx_dump_set the frequency channel by spi(void)
{
	trx_set the frequency channel by spi_t *r = (trx_set the frequency channel by spi_t *)(SOC_TRX_REG_BASE + (0x5 << 2));

	SOC_LOGI("set the frequency channel by spi: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	nsdmlsb: %8x\r\n", r->nsdmlsb);
	SOC_LOGI("	cksel_sdm: %8x\r\n", r->cksel_sdm);
	SOC_LOGI("	vctrl_cal: %8x\r\n", r->vctrl_cal);
	SOC_LOGI("	vsel1v_lo: %8x\r\n", r->vsel1v_lo);
	SOC_LOGI("	pnen: %8x\r\n", r->pnen);
	SOC_LOGI("	sdm_rstn: %8x\r\n", r->sdm_rstn);
	SOC_LOGI("	vsel1v_pllldo: %8x\r\n", r->vsel1v_pllldo);
	SOC_LOGI("	vctrl_cal: %8x\r\n", r->vctrl_cal);
	SOC_LOGI("	vsel1v_cpldo: %8x\r\n", r->vsel1v_cpldo);
	SOC_LOGI("	chspi: %8x\r\n", r->chspi);
}

static void trx_dump_dcoc i path input(void)
{
	trx_dcoc i path input_t *r = (trx_dcoc i path input_t *)(SOC_TRX_REG_BASE + (0x6 << 2));

	SOC_LOGI("dcoc i path input: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x6 << 2)));
	SOC_LOGI("	dpd_en: %8x\r\n", r->dpd_en);
	SOC_LOGI("	nc_1_1: %8x\r\n", r->nc_1_1);
	SOC_LOGI("	rx3s: %8x\r\n", r->rx3s);
	SOC_LOGI("	nc_3_3: %8x\r\n", r->nc_3_3);
	SOC_LOGI("	nc_4_9: %8x\r\n", r->nc_4_9);
	SOC_LOGI("	nc_10_15: %8x\r\n", r->nc_10_15);
	SOC_LOGI("	dcocq: %8x\r\n", r->dcocq);
	SOC_LOGI("	dcoci: %8x\r\n", r->dcoci);
}

static void trx_dump_lpf i path calibiration input (high 2bits)(void)
{
	trx_lpf i path calibiration input (high 2bits)_t *r = (trx_lpf i path calibiration input (high 2bits)_t *)(SOC_TRX_REG_BASE + (0x7 << 2));

	SOC_LOGI("lpf i path calibiration input (high 2bits): %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x7 << 2)));
	SOC_LOGI("	dig_dcoen: %8x\r\n", r->dig_dcoen);
	SOC_LOGI("	spilpfrxg: %8x\r\n", r->spilpfrxg);
	SOC_LOGI("	autorxifgen: %8x\r\n", r->autorxifgen);
	SOC_LOGI("	dcoc_ctl: %8x\r\n", r->dcoc_ctl);
	SOC_LOGI("	txgs: %8x\r\n", r->txgs);
	SOC_LOGI("	lpftxtest_g: %8x\r\n", r->lpftxtest_g);
	SOC_LOGI("	adc_ckinv: %8x\r\n", r->adc_ckinv);
	SOC_LOGI("	txif_2rd_g: %8x\r\n", r->txif_2rd_g);
	SOC_LOGI("	abws_en: %8x\r\n", r->abws_en);
	SOC_LOGI("	lpftxtest_b: %8x\r\n", r->lpftxtest_b);
	SOC_LOGI("	txif_2rd_b: %8x\r\n", r->txif_2rd_b);
	SOC_LOGI("	txif_2rd_ble: %8x\r\n", r->txif_2rd_ble);
	SOC_LOGI("	dac_isel_buf: %8x\r\n", r->dac_isel_buf);
	SOC_LOGI("	dac_isel_opa: %8x\r\n", r->dac_isel_opa);
	SOC_LOGI("	dac_delay: %8x\r\n", r->dac_delay);
	SOC_LOGI("	dac_ck_edge: %8x\r\n", r->dac_ck_edge);
	SOC_LOGI("	lpfcapcalq_h2b: %8x\r\n", r->lpfcapcalq_h2b);
	SOC_LOGI("	lpfcapcali_h2b: %8x\r\n", r->lpfcapcali_h2b);
}

static void trx_dump_rssi high threshold for wifi(void)
{
	trx_rssi high threshold for wifi_t *r = (trx_rssi high threshold for wifi_t *)(SOC_TRX_REG_BASE + (0x8 << 2));

	SOC_LOGI("rssi high threshold for wifi: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	lotrigspi: %8x\r\n", r->lotrigspi);
	SOC_LOGI("	rfpll_notch_res: %8x\r\n", r->rfpll_notch_res);
	SOC_LOGI("	en_wifipll: %8x\r\n", r->en_wifipll);
	SOC_LOGI("	grssi_b: %8x\r\n", r->grssi_b);
	SOC_LOGI("	grssi_w: %8x\r\n", r->grssi_w);
	SOC_LOGI("	rssi_ibs: %8x\r\n", r->rssi_ibs);
	SOC_LOGI("	rssi_tsel: %8x\r\n", r->rssi_tsel);
	SOC_LOGI("	rssi_ten: %8x\r\n", r->rssi_ten);
	SOC_LOGI("	rssi_ble: %8x\r\n", r->rssi_ble);
	SOC_LOGI("	rssitl: %8x\r\n", r->rssitl);
	SOC_LOGI("	rssitm: %8x\r\n", r->rssitm);
	SOC_LOGI("	rssith: %8x\r\n", r->rssith);
}

static void trx_dump_lna load  capacitor tunning(void)
{
	trx_lna load  capacitor tunning_t *r = (trx_lna load  capacitor tunning_t *)(SOC_TRX_REG_BASE + (0x9 << 2));

	SOC_LOGI("lna load  capacitor tunning: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x9 << 2)));
	SOC_LOGI("	agcrxfeen: %8x\r\n", r->agcrxfeen);
	SOC_LOGI("	grxlnaspi: %8x\r\n", r->grxlnaspi);
	SOC_LOGI("	grxi2vspi: %8x\r\n", r->grxi2vspi);
	SOC_LOGI("	vbsrxlo: %8x\r\n", r->vbsrxlo);
	SOC_LOGI("	ibsrxi2v: %8x\r\n", r->ibsrxi2v);
	SOC_LOGI("	vlofastsel: %8x\r\n", r->vlofastsel);
	SOC_LOGI("	enclip: %8x\r\n", r->enclip);
	SOC_LOGI("	enfsr: %8x\r\n", r->enfsr);
	SOC_LOGI("	vslnaldo: %8x\r\n", r->vslnaldo);
	SOC_LOGI("	glnar: %8x\r\n", r->glnar);
	SOC_LOGI("	v2is: %8x\r\n", r->v2is);
	SOC_LOGI("	lnais: %8x\r\n", r->lnais);
	SOC_LOGI("	att_spi: %8x\r\n", r->att_spi);
	SOC_LOGI("	att_manu_en: %8x\r\n", r->att_manu_en);
	SOC_LOGI("	inr: %8x\r\n", r->inr);
	SOC_LOGI("	lglc: %8x\r\n", r->lglc);
	SOC_LOGI("	r_tune: %8x\r\n", r->r_tune);
	SOC_LOGI("	isrxref: %8x\r\n", r->isrxref);
	SOC_LOGI("	cloadlna: %8x\r\n", r->cloadlna);
}

static void trx_dump_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17(void)
{
	trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *r = (trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t *)(SOC_TRX_REG_BASE + (0xa << 2));

	SOC_LOGI("msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0xa << 2)));
	SOC_LOGI("	dygs_vbnc: %8x\r\n", r->dygs_vbnc);
	SOC_LOGI("	dygs_vbnl: %8x\r\n", r->dygs_vbnl);
	SOC_LOGI("	dygs_vbnh: %8x\r\n", r->dygs_vbnh);
	SOC_LOGI("	pabiascgm: %8x\r\n", r->pabiascgm);
	SOC_LOGI("	pa_vdls: %8x\r\n", r->pa_vdls);
	SOC_LOGI("	pa_vdhs: %8x\r\n", r->pa_vdhs);
	SOC_LOGI("	pa__vbpas: %8x\r\n", r->pa__vbpas);
	SOC_LOGI("	pa_vbnls: %8x\r\n", r->pa_vbnls);
	SOC_LOGI("	pa_vbnhs: %8x\r\n", r->pa_vbnhs);
	SOC_LOGI("	pa_endyc: %8x\r\n", r->pa_endyc);
	SOC_LOGI("	pa_encsdyl: %8x\r\n", r->pa_encsdyl);
	SOC_LOGI("	pa_encsdyh: %8x\r\n", r->pa_encsdyh);
	SOC_LOGI("	pa_dibiasbg: %8x\r\n", r->pa_dibiasbg);
}

static void trx_dump_modulator output tuning cap programming: +18ff/lsb (low q nodes)(void)
{
	trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *r = (trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t *)(SOC_TRX_REG_BASE + (0xb << 2));

	SOC_LOGI("modulator output tuning cap programming: +18ff/lsb (low q nodes): %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0xb << 2)));
	SOC_LOGI("	padcd: %8x\r\n", r->padcd);
	SOC_LOGI("	padcc: %8x\r\n", r->padcc);
	SOC_LOGI("	padcb: %8x\r\n", r->padcb);
	SOC_LOGI("	padca: %8x\r\n", r->padca);
	SOC_LOGI("	hvdhs: %8x\r\n", r->hvdhs);
	SOC_LOGI("	dia: %8x\r\n", r->dia);
	SOC_LOGI("	dmodldosel: %8x\r\n", r->dmodldosel);
	SOC_LOGI("	pad_dics: %8x\r\n", r->pad_dics);
	SOC_LOGI("	pad_di: %8x\r\n", r->pad_di);
	SOC_LOGI("	pad_encapdy: %8x\r\n", r->pad_encapdy);
	SOC_LOGI("	modca: %8x\r\n", r->modca);
}

static void trx_dump_(0: spi;  1: digital) power control enable(void)
{
	trx_(0: spi;  1: digital) power control enable_t *r = (trx_(0: spi;  1: digital) power control enable_t *)(SOC_TRX_REG_BASE + (0xc << 2));

	SOC_LOGI("(0: spi;  1: digital) power control enable: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0xc << 2)));
	SOC_LOGI("	gmgain: %8x\r\n", r->gmgain);
	SOC_LOGI("	padctrl: %8x\r\n", r->padctrl);
	SOC_LOGI("	pactrl: %8x\r\n", r->pactrl);
	SOC_LOGI("	rgm: %8x\r\n", r->rgm);
	SOC_LOGI("	dgmdc: %8x\r\n", r->dgmdc);
	SOC_LOGI("	mixvswb: %8x\r\n", r->mixvswb);
	SOC_LOGI("	mixvbh: %8x\r\n", r->mixvbh);
	SOC_LOGI("	pa_dibiaspt: %8x\r\n", r->pa_dibiaspt);
	SOC_LOGI("	pa_cap: %8x\r\n", r->pa_cap);
	SOC_LOGI("	pamapen: %8x\r\n", r->pamapen);
}

static void trx_dump_msb of pa current bias temperature  setting , another bits at 0x00[0](void)
{
	trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *r = (trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t *)(SOC_TRX_REG_BASE + (0xd << 2));

	SOC_LOGI("msb of pa current bias temperature  setting , another bits at 0x00[0]: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0xd << 2)));
	SOC_LOGI("	ipatcs: %8x\r\n", r->ipatcs);
	SOC_LOGI("	tssiqrg: %8x\r\n", r->tssiqrg);
	SOC_LOGI("	pa_vref_vbgs: %8x\r\n", r->pa_vref_vbgs);
	SOC_LOGI("	pa_vb_vddgs: %8x\r\n", r->pa_vb_vddgs);
	SOC_LOGI("	tssiranges: %8x\r\n", r->tssiranges);
	SOC_LOGI("	en_pcal: %8x\r\n", r->en_pcal);
	SOC_LOGI("	en_iqcal: %8x\r\n", r->en_iqcal);
	SOC_LOGI("	itas: %8x\r\n", r->itas);
	SOC_LOGI("	rccalen: %8x\r\n", r->rccalen);
	SOC_LOGI("	pa_dyopbias: %8x\r\n", r->pa_dyopbias);
	SOC_LOGI("	rxlo ldo: %8x\r\n", r->rxlo ldo);
	SOC_LOGI("	hvdls: %8x\r\n", r->hvdls);
	SOC_LOGI("	enpadcapbias: %8x\r\n", r->enpadcapbias);
	SOC_LOGI("	pad_capbiass: %8x\r\n", r->pad_capbiass);
	SOC_LOGI("	txlo ldo: %8x\r\n", r->txlo ldo);
	SOC_LOGI("	ldoda: %8x\r\n", r->ldoda);
	SOC_LOGI("	ipatcs: %8x\r\n", r->ipatcs);
}

static void trx_dump_nc(void)
{
	trx_nc_t *r = (trx_nc_t *)(SOC_TRX_REG_BASE + (0xe << 2));

	SOC_LOGI("nc: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0xe << 2)));
	SOC_LOGI("	dygs_vbnc_2: %8x\r\n", r->dygs_vbnc_2);
	SOC_LOGI("	dygs_vbnl_2: %8x\r\n", r->dygs_vbnl_2);
	SOC_LOGI("	dygs_vbnh_2: %8x\r\n", r->dygs_vbnh_2);
	SOC_LOGI("	pabiascgm_2: %8x\r\n", r->pabiascgm_2);
	SOC_LOGI("	pa_vdls_2: %8x\r\n", r->pa_vdls_2);
	SOC_LOGI("	pa_vdhs_2: %8x\r\n", r->pa_vdhs_2);
	SOC_LOGI("	pa__vbpas_2: %8x\r\n", r->pa__vbpas_2);
	SOC_LOGI("	pa_vbnls_2: %8x\r\n", r->pa_vbnls_2);
	SOC_LOGI("	pa_vbnhs_2: %8x\r\n", r->pa_vbnhs_2);
	SOC_LOGI("	pa_endyc_2: %8x\r\n", r->pa_endyc_2);
	SOC_LOGI("	pa_encsdyl_2: %8x\r\n", r->pa_encsdyl_2);
	SOC_LOGI("	pa_encsdyh_2: %8x\r\n", r->pa_encsdyh_2);
	SOC_LOGI("	nc_28_31: %8x\r\n", r->nc_28_31);
}

static void trx_dump_rsv_f_11(void)
{
	for (uint32_t idx = 0; idx < 3; idx++) {
		SOC_LOGI("rsv_f_11: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + ((0xf + idx) << 2)));
	}
}

static void trx_dump_0 = rest; 1=normal work(void)
{
	trx_0 = rest; 1=normal work_t *r = (trx_0 = rest; 1=normal work_t *)(SOC_TRX_REG_BASE + (0x12 << 2));

	SOC_LOGI("0 = rest; 1=normal work: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x12 << 2)));
	SOC_LOGI("	iops: %8x\r\n", r->iops);
	SOC_LOGI("	fictl: %8x\r\n", r->fictl);
	SOC_LOGI("	cgmbs: %8x\r\n", r->cgmbs);
	SOC_LOGI("	ldoifsel: %8x\r\n", r->ldoifsel);
	SOC_LOGI("	enclip_flt: %8x\r\n", r->enclip_flt);
	SOC_LOGI("	enfsr_flt: %8x\r\n", r->enfsr_flt);
	SOC_LOGI("	flagsel: %8x\r\n", r->flagsel);
	SOC_LOGI("	ldoad: %8x\r\n", r->ldoad);
	SOC_LOGI("	adc_isel: %8x\r\n", r->adc_isel);
	SOC_LOGI("	dlypc_i: %8x\r\n", r->dlypc_i);
	SOC_LOGI("	tdtune_i: %8x\r\n", r->tdtune_i);
	SOC_LOGI("	tdtuneb: %8x\r\n", r->tdtuneb);
	SOC_LOGI("	calen: %8x\r\n", r->calen);
	SOC_LOGI("	calnsel: %8x\r\n", r->calnsel);
	SOC_LOGI("	ckmode: %8x\r\n", r->ckmode);
	SOC_LOGI("	vrefs: %8x\r\n", r->vrefs);
	SOC_LOGI("	adc_rstn: %8x\r\n", r->adc_rstn);
}

static void trx_dump_cell delay control spi value(void)
{
	trx_cell delay control spi value_t *r = (trx_cell delay control spi value_t *)(SOC_TRX_REG_BASE + (0x13 << 2));

	SOC_LOGI("cell delay control spi value: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x13 << 2)));
	SOC_LOGI("	lpfouttsten: %8x\r\n", r->lpfouttsten);
	SOC_LOGI("	lpfintsten: %8x\r\n", r->lpfintsten);
	SOC_LOGI("	atsel: %8x\r\n", r->atsel);
	SOC_LOGI("	entst: %8x\r\n", r->entst);
	SOC_LOGI("	nc_5_5: %8x\r\n", r->nc_5_5);
	SOC_LOGI("	lovctrl_testen: %8x\r\n", r->lovctrl_testen);
	SOC_LOGI("	lovctrlsel: %8x\r\n", r->lovctrlsel);
	SOC_LOGI("	trsw_bt_mode: %8x\r\n", r->trsw_bt_mode);
	SOC_LOGI("	btpll_en_spi: %8x\r\n", r->btpll_en_spi);
	SOC_LOGI("	nc_10_10: %8x\r\n", r->nc_10_10);
	SOC_LOGI("	t0r1_spi: %8x\r\n", r->t0r1_spi);
	SOC_LOGI("	lotrigctr: %8x\r\n", r->lotrigctr);
	SOC_LOGI("	losel_rx: %8x\r\n", r->losel_rx);
	SOC_LOGI("	loselvctr: %8x\r\n", r->loselvctr);
	SOC_LOGI("	losel0dig: %8x\r\n", r->losel0dig);
	SOC_LOGI("	ckmode_pwd: %8x\r\n", r->ckmode_pwd);
	SOC_LOGI("	vbias_adc: %8x\r\n", r->vbias_adc);
	SOC_LOGI("	dlypc_i: %8x\r\n", r->dlypc_i);
	SOC_LOGI("	nc_25_25: %8x\r\n", r->nc_25_25);
	SOC_LOGI("	tdtune_q: %8x\r\n", r->tdtune_q);
}

static void trx_dump_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge(void)
{
	trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *r = (trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t *)(SOC_TRX_REG_BASE + (0x14 << 2));

	SOC_LOGI("clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x14 << 2)));
	SOC_LOGI("	nc_0_0: %8x\r\n", r->nc_0_0);
	SOC_LOGI("	nc_1_1: %8x\r\n", r->nc_1_1);
	SOC_LOGI("	wifi_loen: %8x\r\n", r->wifi_loen);
	SOC_LOGI("	diffn: %8x\r\n", r->diffn);
	SOC_LOGI("	pnen: %8x\r\n", r->pnen);
	SOC_LOGI("	if2m: %8x\r\n", r->if2m);
	SOC_LOGI("	rxlo: %8x\r\n", r->rxlo);
	SOC_LOGI("	cksel_sdm: %8x\r\n", r->cksel_sdm);
	SOC_LOGI("	ck2sdmpol_sel: %8x\r\n", r->ck2sdmpol_sel);
}

static void trx_dump_(void)
{
	trx__t *r = (trx__t *)(SOC_TRX_REG_BASE + (0x15 << 2));

	SOC_LOGI(": %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x15 << 2)));
	SOC_LOGI("	reset_nload: %8x\r\n", r->reset_nload);
	SOC_LOGI("	tristate: %8x\r\n", r->tristate);
	SOC_LOGI("	selpol: %8x\r\n", r->selpol);
	SOC_LOGI("	nc_3_7: %8x\r\n", r->nc_3_7);
	SOC_LOGI("	cp1_rx: %8x\r\n", r->cp1_rx);
	SOC_LOGI("	rp3_rx: %8x\r\n", r->rp3_rx);
	SOC_LOGI("	rp2_rx: %8x\r\n", r->rp2_rx);
	SOC_LOGI("	rp1_rx: %8x\r\n", r->rp1_rx);
	SOC_LOGI("	ioffset_rx: %8x\r\n", r->ioffset_rx);
	SOC_LOGI("	icp_rx: %8x\r\n", r->icp_rx);
	SOC_LOGI("	ck_nload_edgesel: %8x\r\n", r->ck_nload_edgesel);
}

static void trx_dump_band calibration optimize enable.(void)
{
	trx_band calibration optimize enable._t *r = (trx_band calibration optimize enable._t *)(SOC_TRX_REG_BASE + (0x16 << 2));

	SOC_LOGI("band calibration optimize enable.: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x16 << 2)));
	SOC_LOGI("	vreflctrl: %8x\r\n", r->vreflctrl);
	SOC_LOGI("	vrefhctrl: %8x\r\n", r->vrefhctrl);
	SOC_LOGI("	reset_ncounter: %8x\r\n", r->reset_ncounter);
	SOC_LOGI("	nc_5_28: %8x\r\n", r->nc_5_28);
	SOC_LOGI("	pwd5g: %8x\r\n", r->pwd5g);
	SOC_LOGI("	pwd2g: %8x\r\n", r->pwd2g);
	SOC_LOGI("	bp_opt: %8x\r\n", r->bp_opt);
}

static void trx_dump_n value spi control(void)
{
	trx_n value spi control_t *r = (trx_n value spi control_t *)(SOC_TRX_REG_BASE + (0x17 << 2));

	SOC_LOGI("n value spi control: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x17 << 2)));
	SOC_LOGI("	rctrl: %8x\r\n", r->rctrl);
	SOC_LOGI("	pwd_vcoampcal: %8x\r\n", r->pwd_vcoampcal);
	SOC_LOGI("	rxloctrl_vcocal: %8x\r\n", r->rxloctrl_vcocal);
	SOC_LOGI("	done_delay: %8x\r\n", r->done_delay);
	SOC_LOGI("	spi_trigger: %8x\r\n", r->spi_trigger);
	SOC_LOGI("	errdet_spien: %8x\r\n", r->errdet_spien);
	SOC_LOGI("	ckref_loop_sel: %8x\r\n", r->ckref_loop_sel);
	SOC_LOGI("	ckref_nl_sel: %8x\r\n", r->ckref_nl_sel);
	SOC_LOGI("	nrsten: %8x\r\n", r->nrsten);
	SOC_LOGI("	int_mod: %8x\r\n", r->int_mod);
	SOC_LOGI("	band_men: %8x\r\n", r->band_men);
	SOC_LOGI("	band_spi: %8x\r\n", r->band_spi);
	SOC_LOGI("	nmanual: %8x\r\n", r->nmanual);
}

static void trx_dump_vco amplitude auto calibration threshold control..(void)
{
	trx_vco amplitude auto calibration threshold control.._t *r = (trx_vco amplitude auto calibration threshold control.._t *)(SOC_TRX_REG_BASE + (0x18 << 2));

	SOC_LOGI("vco amplitude auto calibration threshold control..: %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x18 << 2)));
	SOC_LOGI("	nc_0_19: %8x\r\n", r->nc_0_19);
	SOC_LOGI("	vcobias_men: %8x\r\n", r->vcobias_men);
	SOC_LOGI("	vcobias_spi: %8x\r\n", r->vcobias_spi);
	SOC_LOGI("	vcoampdet: %8x\r\n", r->vcoampdet);
}

static void trx_dump_(void)
{
	trx__t *r = (trx__t *)(SOC_TRX_REG_BASE + (0x19 << 2));

	SOC_LOGI(": %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x19 << 2)));
	SOC_LOGI("	nc_0_1: %8x\r\n", r->nc_0_1);
	SOC_LOGI("	bwctrl: %8x\r\n", r->bwctrl);
	SOC_LOGI("	nc_3_3: %8x\r\n", r->nc_3_3);
	SOC_LOGI("	vcohpen: %8x\r\n", r->vcohpen);
	SOC_LOGI("	nc_5_12: %8x\r\n", r->nc_5_12);
	SOC_LOGI("	vcal_sel: %8x\r\n", r->vcal_sel);
	SOC_LOGI("	nc_15_20: %8x\r\n", r->nc_15_20);
	SOC_LOGI("	vsel1v_vco: %8x\r\n", r->vsel1v_vco);
	SOC_LOGI("	vsel1v_pll: %8x\r\n", r->vsel1v_pll);
	SOC_LOGI("	vsel1v_cp: %8x\r\n", r->vsel1v_cp);
	SOC_LOGI("	nc_30_30: %8x\r\n", r->nc_30_30);
	SOC_LOGI("	ck26m_40m_sel: %8x\r\n", r->ck26m_40m_sel);
}

static void trx_dump_(void)
{
	SOC_LOGI(": %8x\r\n", REG_READ(SOC_TRX_REG_BASE + (0x1a << 2)));
}

static trx_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, trx_dump_},
	{0x1, 0x1, trx_dump_controlling the voltage of calibration},
	{0x2, 0x2, trx_dump_enable vco in the top by spi},
	{0x3, 0x3, trx_dump_controlling the bias current of vco},
	{0x4, 0x4, trx_dump_loop int mode enable},
	{0x5, 0x5, trx_dump_set the frequency channel by spi},
	{0x6, 0x6, trx_dump_dcoc i path input},
	{0x7, 0x7, trx_dump_lpf i path calibiration input (high 2bits)},
	{0x8, 0x8, trx_dump_rssi high threshold for wifi},
	{0x9, 0x9, trx_dump_lna load  capacitor tunning},
	{0xa, 0xa, trx_dump_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17},
	{0xb, 0xb, trx_dump_modulator output tuning cap programming: +18ff/lsb (low q nodes)},
	{0xc, 0xc, trx_dump_(0: spi;  1: digital) power control enable},
	{0xd, 0xd, trx_dump_msb of pa current bias temperature  setting , another bits at 0x00[0]},
	{0xe, 0xe, trx_dump_nc},
	{0xf, 0x12, trx_dump_rsv_f_11},
	{0x12, 0x12, trx_dump_0 = rest; 1=normal work},
	{0x13, 0x13, trx_dump_cell delay control spi value},
	{0x14, 0x14, trx_dump_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge},
	{0x15, 0x15, trx_dump_},
	{0x16, 0x16, trx_dump_band calibration optimize enable.},
	{0x17, 0x17, trx_dump_n value spi control},
	{0x18, 0x18, trx_dump_vco amplitude auto calibration threshold control..},
	{0x19, 0x19, trx_dump_},
	{0x1a, 0x1a, trx_dump_},
	{-1, -1, 0}
};

void trx_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
