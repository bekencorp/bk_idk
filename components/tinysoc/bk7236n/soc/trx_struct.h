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

#ifdef __cplusplus
extern "C" {
#endif


typedef volatile union {
	struct {
		uint32_t nc_0_31          : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} trx__t;


typedef volatile union {
	struct {
		uint32_t bypass_opt       :  1; /**<bit[0 : 0] */
		uint32_t manual_band      :  1; /**<bit[1 : 1] */
		uint32_t bp_ibias         :  1; /**<bit[2 : 2] */
		uint32_t vsel1v_vcoldo    :  3; /**<bit[3 : 5] */
		uint32_t edgesel_nck      :  1; /**<bit[6 : 6] */
		uint32_t icpoffset_tx     :  6; /**<bit[7 : 12] */
		uint32_t itune_mixer      :  3; /**<bit[13 : 15] */
		uint32_t icpoffset_rx     :  6; /**<bit[16 : 21] */
		uint32_t ctune_lpf        :  6; /**<bit[22 : 27] */
		uint32_t nload_dlyen      :  1; /**<bit[28 : 28] */
		uint32_t caldone_spi      :  1; /**<bit[29 : 29] */
		uint32_t bypass_caldone   :  1; /**<bit[30 : 30] */
		uint32_t vctrl_cal        :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_controlling the voltage of calibration_t;


typedef volatile union {
	struct {
		uint32_t ck2xen           :  1; /**<bit[0 : 0] */
		uint32_t clksel_cal       :  1; /**<bit[1 : 1] */
		uint32_t bandcal_spi      : 10; /**<bit[2 : 11] */
		uint32_t band_spi_trgger  :  1; /**<bit[12 : 12] */
		uint32_t icp_bias_com     :  2; /**<bit[13 : 14] */
		uint32_t hvref            :  2; /**<bit[15 : 16] */
		uint32_t lvref            :  2; /**<bit[17 : 18] */
		uint32_t done_delay       :  3; /**<bit[19 : 21] */
		uint32_t amptrigger       :  1; /**<bit[22 : 22] */
		uint32_t ampautocal       :  1; /**<bit[23 : 23] */
		uint32_t ampcal_en        :  1; /**<bit[24 : 24] */
		uint32_t ampctrl_m        :  1; /**<bit[25 : 25] */
		uint32_t errdet_en        :  1; /**<bit[26 : 26] */
		uint32_t errdet_spien     :  1; /**<bit[27 : 27] */
		uint32_t vsel1v_pllldo    :  2; /**<bit[28 : 29] */
		uint32_t pllen            :  1; /**<bit[30 : 30] */
		uint32_t vcoen            :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_enable vco in the top by spi_t;


typedef volatile union {
	struct {
		uint32_t ctune_mixer_ch   :  4; /**<bit[0 : 3] */
		uint32_t ctune_mixer      :  6; /**<bit[4 : 9] */
		uint32_t ampselu          :  3; /**<bit[10 : 12] */
		uint32_t icpbias_rx       :  1; /**<bit[13 : 13] */
		uint32_t icpbias_tx       :  1; /**<bit[14 : 14] */
		uint32_t itune_vco_spi    : 17; /**<bit[15 : 31] */
	};
	uint32_t v;
} trx_controlling the bias current of vco_t;


typedef volatile union {
	struct {
		uint32_t icpbias_rx       :  4; /**<bit[0 : 3] */
		uint32_t icpbias_tx       :  4; /**<bit[4 : 7] */
		uint32_t ampsel           :  5; /**<bit[8 : 12] */
		uint32_t bypass_aac       :  1; /**<bit[13 : 13] */
		uint32_t reset_spi        :  1; /**<bit[14 : 14] */
		uint32_t nrsten           :  1; /**<bit[15 : 15] */
		uint32_t tristate_spi     :  1; /**<bit[16 : 16] */
		uint32_t selvcopol        :  1; /**<bit[17 : 17] */
		uint32_t nclken_spi       :  1; /**<bit[18 : 18] */
		uint32_t nint             : 10; /**<bit[19 : 28] */
		uint32_t ib1v_lomixer     :  2; /**<bit[29 : 30] */
		uint32_t int_mode         :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_loop int mode enable_t;


typedef volatile union {
	struct {
		uint32_t nsdmlsb          :  8; /**<bit[0 : 7] */
		uint32_t cksel_sdm        :  2; /**<bit[8 : 9] */
		uint32_t vctrl_cal        :  1; /**<bit[10 : 10] */
		uint32_t vsel1v_lo        :  3; /**<bit[11 : 13] */
		uint32_t pnen             :  1; /**<bit[14 : 14] */
		uint32_t sdm_rstn         :  1; /**<bit[15 : 15] */
		uint32_t vsel1v_pllldo    :  3; /**<bit[16 : 18] */
		uint32_t vctrl_cal        :  1; /**<bit[19 : 19] */
		uint32_t vsel1v_cpldo     :  5; /**<bit[20 : 24] */
		uint32_t chspi            :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} trx_set the frequency channel by spi_t;


typedef volatile union {
	struct {
		uint32_t dpd_en           :  1; /**<bit[0 : 0] */
		uint32_t nc_1_1           :  1; /**<bit[1 : 1] */
		uint32_t rx3s             :  1; /**<bit[2 : 2] */
		uint32_t nc_3_3           :  1; /**<bit[3 : 3] */
		uint32_t nc_4_9           :  6; /**<bit[4 : 9] */
		uint32_t nc_10_15         :  6; /**<bit[10 : 15] */
		uint32_t dcocq            :  8; /**<bit[16 : 23] */
		uint32_t dcoci            :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} trx_dcoc i path input_t;


typedef volatile union {
	struct {
		uint32_t dig_dcoen        :  1; /**<bit[0 : 0] */
		uint32_t spilpfrxg        :  4; /**<bit[1 : 4] */
		uint32_t autorxifgen      :  1; /**<bit[5 : 5] */
		uint32_t dcoc_ctl         :  2; /**<bit[6 : 7] */
		uint32_t txgs             :  1; /**<bit[8 : 8] */
		uint32_t lpftxtest_g      :  1; /**<bit[9 : 9] */
		uint32_t adc_ckinv        :  1; /**<bit[10 : 10] */
		uint32_t txif_2rd_g       :  1; /**<bit[11 : 11] */
		uint32_t abws_en          :  1; /**<bit[12 : 12] */
		uint32_t lpftxtest_b      :  1; /**<bit[13 : 13] */
		uint32_t txif_2rd_b       :  1; /**<bit[14 : 14] */
		uint32_t txif_2rd_ble     :  1; /**<bit[15 : 15] */
		uint32_t dac_isel_buf     :  3; /**<bit[16 : 18] */
		uint32_t dac_isel_opa     :  3; /**<bit[19 : 21] */
		uint32_t dac_delay        :  5; /**<bit[22 : 26] */
		uint32_t dac_ck_edge      :  1; /**<bit[27 : 27] */
		uint32_t lpfcapcalq_h2b   :  2; /**<bit[28 : 29] */
		uint32_t lpfcapcali_h2b   :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} trx_lpf i path calibiration input (high 2bits)_t;


typedef volatile union {
	struct {
		uint32_t lotrigspi        :  1; /**<bit[0 : 0] */
		uint32_t rfpll_notch_res  :  7; /**<bit[1 : 7] */
		uint32_t en_wifipll       :  1; /**<bit[8 : 8] */
		uint32_t grssi_b          :  1; /**<bit[9 : 9] */
		uint32_t grssi_w          :  1; /**<bit[10 : 10] */
		uint32_t rssi_ibs         :  2; /**<bit[11 : 12] */
		uint32_t rssi_tsel        :  2; /**<bit[13 : 14] */
		uint32_t rssi_ten         :  1; /**<bit[15 : 15] */
		uint32_t rssi_ble         :  4; /**<bit[16 : 19] */
		uint32_t rssitl           :  4; /**<bit[20 : 23] */
		uint32_t rssitm           :  4; /**<bit[24 : 27] */
		uint32_t rssith           :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} trx_rssi high threshold for wifi_t;


typedef volatile union {
	struct {
		uint32_t agcrxfeen        :  1; /**<bit[0 : 0] */
		uint32_t grxlnaspi        :  2; /**<bit[1 : 2] */
		uint32_t grxi2vspi        :  2; /**<bit[3 : 4] */
		uint32_t vbsrxlo          :  3; /**<bit[5 : 7] */
		uint32_t ibsrxi2v         :  2; /**<bit[8 : 9] */
		uint32_t vlofastsel       :  1; /**<bit[10 : 10] */
		uint32_t enclip           :  1; /**<bit[11 : 11] */
		uint32_t enfsr            :  1; /**<bit[12 : 12] */
		uint32_t vslnaldo         :  3; /**<bit[13 : 15] */
		uint32_t glnar            :  2; /**<bit[16 : 17] */
		uint32_t v2is             :  2; /**<bit[18 : 19] */
		uint32_t lnais            :  2; /**<bit[20 : 21] */
		uint32_t att_spi          :  1; /**<bit[22 : 22] */
		uint32_t att_manu_en      :  1; /**<bit[23 : 23] */
		uint32_t inr              :  1; /**<bit[24 : 24] */
		uint32_t lglc             :  1; /**<bit[25 : 25] */
		uint32_t r_tune           :  1; /**<bit[26 : 26] */
		uint32_t isrxref          :  3; /**<bit[27 : 29] */
		uint32_t cloadlna         :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} trx_lna load  capacitor tunning_t;


typedef volatile union {
	struct {
		uint32_t dygs_vbnc        :  2; /**<bit[0 : 1] */
		uint32_t dygs_vbnl        :  2; /**<bit[2 : 3] */
		uint32_t dygs_vbnh        :  2; /**<bit[4 : 5] */
		uint32_t pabiascgm        :  1; /**<bit[6 : 6] */
		uint32_t pa_vdls          :  3; /**<bit[7 : 9] */
		uint32_t pa_vdhs          :  3; /**<bit[10 : 12] */
		uint32_t pa__vbpas        :  4; /**<bit[13 : 16] */
		uint32_t pa_vbnls         :  4; /**<bit[17 : 20] */
		uint32_t pa_vbnhs         :  4; /**<bit[21 : 24] */
		uint32_t pa_endyc         :  1; /**<bit[25 : 25] */
		uint32_t pa_encsdyl       :  1; /**<bit[26 : 26] */
		uint32_t pa_encsdyh       :  1; /**<bit[27 : 27] */
		uint32_t pa_dibiasbg      :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t;


typedef volatile union {
	struct {
		uint32_t padcd            :  4; /**<bit[0 : 3] */
		uint32_t padcc            :  4; /**<bit[4 : 7] */
		uint32_t padcb            :  4; /**<bit[8 : 11] */
		uint32_t padca            :  4; /**<bit[12 : 15] */
		uint32_t hvdhs            :  1; /**<bit[16 : 16] */
		uint32_t dia              :  1; /**<bit[17 : 17] */
		uint32_t dmodldosel       :  3; /**<bit[18 : 20] */
		uint32_t pad_dics         :  2; /**<bit[21 : 22] */
		uint32_t pad_di           :  4; /**<bit[23 : 26] */
		uint32_t pad_encapdy      :  1; /**<bit[27 : 27] */
		uint32_t modca            :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t;


typedef volatile union {
	struct {
		uint32_t gmgain           :  4; /**<bit[0 : 3] */
		uint32_t padctrl          :  4; /**<bit[4 : 7] */
		uint32_t pactrl           :  4; /**<bit[8 : 11] */
		uint32_t rgm              :  4; /**<bit[12 : 15] */
		uint32_t dgmdc            :  4; /**<bit[16 : 19] */
		uint32_t mixvswb          :  2; /**<bit[20 : 21] */
		uint32_t mixvbh           :  2; /**<bit[22 : 23] */
		uint32_t pa_dibiaspt      :  4; /**<bit[24 : 27] */
		uint32_t pa_cap           :  3; /**<bit[28 : 30] */
		uint32_t pamapen          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_(0: spi;  1: digital) power control enable_t;


typedef volatile union {
	struct {
		uint32_t ipatcs           :  1; /**<bit[0 : 0] */
		uint32_t tssiqrg          :  3; /**<bit[1 : 3] */
		uint32_t pa_vref_vbgs     :  1; /**<bit[4 : 4] */
		uint32_t pa_vb_vddgs      :  2; /**<bit[5 : 6] */
		uint32_t tssiranges       :  2; /**<bit[7 : 8] */
		uint32_t en_pcal          :  1; /**<bit[9 : 9] */
		uint32_t en_iqcal         :  1; /**<bit[10 : 10] */
		uint32_t itas             :  1; /**<bit[11 : 11] */
		uint32_t rccalen          :  1; /**<bit[12 : 12] */
		uint32_t pa_dyopbias      :  3; /**<bit[13 : 15] */
		uint32_t rxlo ldo         :  3; /**<bit[16 : 18] */
		uint32_t hvdls            :  1; /**<bit[19 : 19] */
		uint32_t enpadcapbias     :  1; /**<bit[20 : 20] */
		uint32_t pad_capbiass     :  3; /**<bit[21 : 23] */
		uint32_t txlo ldo         :  3; /**<bit[24 : 26] */
		uint32_t ldoda            :  3; /**<bit[27 : 29] */
		uint32_t ipatcs           :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t;


typedef volatile union {
	struct {
		uint32_t dygs_vbnc_2      :  2; /**<bit[0 : 1] */
		uint32_t dygs_vbnl_2      :  2; /**<bit[2 : 3] */
		uint32_t dygs_vbnh_2      :  2; /**<bit[4 : 5] */
		uint32_t pabiascgm_2      :  1; /**<bit[6 : 6] */
		uint32_t pa_vdls_2        :  3; /**<bit[7 : 9] */
		uint32_t pa_vdhs_2        :  3; /**<bit[10 : 12] */
		uint32_t pa__vbpas_2      :  4; /**<bit[13 : 16] */
		uint32_t pa_vbnls_2       :  4; /**<bit[17 : 20] */
		uint32_t pa_vbnhs_2       :  4; /**<bit[21 : 24] */
		uint32_t pa_endyc_2       :  1; /**<bit[25 : 25] */
		uint32_t pa_encsdyl_2     :  1; /**<bit[26 : 26] */
		uint32_t pa_encsdyh_2     :  1; /**<bit[27 : 27] */
		uint32_t nc_28_31         :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} trx_nc_t;


typedef volatile union {
	struct {
		uint32_t iops             :  1; /**<bit[0 : 0] */
		uint32_t fictl            :  3; /**<bit[1 : 3] */
		uint32_t cgmbs            :  1; /**<bit[4 : 4] */
		uint32_t ldoifsel         :  3; /**<bit[5 : 7] */
		uint32_t enclip_flt       :  1; /**<bit[8 : 8] */
		uint32_t enfsr_flt        :  1; /**<bit[9 : 9] */
		uint32_t flagsel          :  1; /**<bit[10 : 10] */
		uint32_t ldoad            :  3; /**<bit[11 : 13] */
		uint32_t adc_isel         :  3; /**<bit[14 : 16] */
		uint32_t dlypc_i          :  2; /**<bit[17 : 18] */
		uint32_t tdtune_i         :  6; /**<bit[19 : 24] */
		uint32_t tdtuneb          :  1; /**<bit[25 : 25] */
		uint32_t calen            :  1; /**<bit[26 : 26] */
		uint32_t calnsel          :  2; /**<bit[27 : 28] */
		uint32_t ckmode           :  1; /**<bit[29 : 29] */
		uint32_t vrefs            :  1; /**<bit[30 : 30] */
		uint32_t adc_rstn         :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_0 = rest; 1=normal work_t;


typedef volatile union {
	struct {
		uint32_t lpfouttsten      :  1; /**<bit[0 : 0] */
		uint32_t lpfintsten       :  1; /**<bit[1 : 1] */
		uint32_t atsel            :  2; /**<bit[2 : 3] */
		uint32_t entst            :  1; /**<bit[4 : 4] */
		uint32_t nc_5_5           :  1; /**<bit[5 : 5] */
		uint32_t lovctrl_testen   :  1; /**<bit[6 : 6] */
		uint32_t lovctrlsel       :  1; /**<bit[7 : 7] */
		uint32_t trsw_bt_mode     :  1; /**<bit[8 : 8] */
		uint32_t btpll_en_spi     :  1; /**<bit[9 : 9] */
		uint32_t nc_10_10         :  1; /**<bit[10 : 10] */
		uint32_t t0r1_spi         :  1; /**<bit[11 : 11] */
		uint32_t lotrigctr        :  1; /**<bit[12 : 12] */
		uint32_t losel_rx         :  2; /**<bit[13 : 14] */
		uint32_t loselvctr        :  1; /**<bit[15 : 15] */
		uint32_t losel0dig        :  1; /**<bit[16 : 16] */
		uint32_t ckmode_pwd       :  1; /**<bit[17 : 17] */
		uint32_t vbias_adc        :  5; /**<bit[18 : 22] */
		uint32_t dlypc_i          :  2; /**<bit[23 : 24] */
		uint32_t nc_25_25         :  1; /**<bit[25 : 25] */
		uint32_t tdtune_q         :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} trx_cell delay control spi value_t;


typedef volatile union {
	struct {
		uint32_t nc_0_0           :  1; /**<bit[0 : 0] */
		uint32_t nc_1_1           :  1; /**<bit[1 : 1] */
		uint32_t wifi_loen        :  1; /**<bit[2 : 2] */
		uint32_t diffn            : 24; /**<bit[3 : 26] */
		uint32_t pnen             :  1; /**<bit[27 : 27] */
		uint32_t if2m             :  1; /**<bit[28 : 28] */
		uint32_t rxlo             :  1; /**<bit[29 : 29] */
		uint32_t cksel_sdm        :  1; /**<bit[30 : 30] */
		uint32_t ck2sdmpol_sel    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t;


typedef volatile union {
	struct {
		uint32_t reset_nload      :  1; /**<bit[0 : 0] */
		uint32_t tristate         :  1; /**<bit[1 : 1] */
		uint32_t selpol           :  1; /**<bit[2 : 2] */
		uint32_t nc_3_7           :  5; /**<bit[3 : 7] */
		uint32_t cp1_rx           :  4; /**<bit[8 : 11] */
		uint32_t rp3_rx           :  4; /**<bit[12 : 15] */
		uint32_t rp2_rx           :  4; /**<bit[16 : 19] */
		uint32_t rp1_rx           :  5; /**<bit[20 : 24] */
		uint32_t ioffset_rx       :  2; /**<bit[25 : 26] */
		uint32_t icp_rx           :  4; /**<bit[27 : 30] */
		uint32_t ck_nload_edgesel :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx__t;


typedef volatile union {
	struct {
		uint32_t vreflctrl        :  2; /**<bit[0 : 1] */
		uint32_t vrefhctrl        :  2; /**<bit[2 : 3] */
		uint32_t reset_ncounter   :  1; /**<bit[4 : 4] */
		uint32_t nc_5_28          : 24; /**<bit[5 : 28] */
		uint32_t pwd5g            :  1; /**<bit[29 : 29] */
		uint32_t pwd2g            :  1; /**<bit[30 : 30] */
		uint32_t bp_opt           :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx_band calibration optimize enable._t;


typedef volatile union {
	struct {
		uint32_t rctrl            :  4; /**<bit[0 : 3] */
		uint32_t pwd_vcoampcal    :  1; /**<bit[4 : 4] */
		uint32_t rxloctrl_vcocal  :  1; /**<bit[5 : 5] */
		uint32_t done_delay       :  3; /**<bit[6 : 8] */
		uint32_t spi_trigger      :  1; /**<bit[9 : 9] */
		uint32_t errdet_spien     :  1; /**<bit[10 : 10] */
		uint32_t ckref_loop_sel   :  1; /**<bit[11 : 11] */
		uint32_t ckref_nl_sel     :  1; /**<bit[12 : 12] */
		uint32_t nrsten           :  1; /**<bit[13 : 13] */
		uint32_t int_mod          :  1; /**<bit[14 : 14] */
		uint32_t band_men         :  1; /**<bit[15 : 15] */
		uint32_t band_spi         :  8; /**<bit[16 : 23] */
		uint32_t nmanual          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} trx_n value spi control_t;


typedef volatile union {
	struct {
		uint32_t nc_0_19          : 20; /**<bit[0 : 19] */
		uint32_t vcobias_men      :  1; /**<bit[20 : 20] */
		uint32_t vcobias_spi      :  6; /**<bit[21 : 26] */
		uint32_t vcoampdet        :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} trx_vco amplitude auto calibration threshold control.._t;


typedef volatile union {
	struct {
		uint32_t nc_0_1           :  2; /**<bit[0 : 1] */
		uint32_t bwctrl           :  1; /**<bit[2 : 2] */
		uint32_t nc_3_3           :  1; /**<bit[3 : 3] */
		uint32_t vcohpen          :  1; /**<bit[4 : 4] */
		uint32_t nc_5_12          :  8; /**<bit[5 : 12] */
		uint32_t vcal_sel         :  2; /**<bit[13 : 14] */
		uint32_t nc_15_20         :  6; /**<bit[15 : 20] */
		uint32_t vsel1v_vco       :  3; /**<bit[21 : 23] */
		uint32_t vsel1v_pll       :  3; /**<bit[24 : 26] */
		uint32_t vsel1v_cp        :  3; /**<bit[27 : 29] */
		uint32_t nc_30_30         :  1; /**<bit[30 : 30] */
		uint32_t ck26m_40m_sel    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} trx__t;


typedef volatile union {
	struct {
		uint32_t nc_0_31          : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} trx__t;

typedef volatile struct {
	volatile trx__t ;
	volatile trx_controlling the voltage of calibration_t controlling the voltage of calibration;
	volatile trx_enable vco in the top by spi_t enable vco in the top by spi;
	volatile trx_controlling the bias current of vco_t controlling the bias current of vco;
	volatile trx_loop int mode enable_t loop int mode enable;
	volatile trx_set the frequency channel by spi_t set the frequency channel by spi;
	volatile trx_dcoc i path input_t dcoc i path input;
	volatile trx_lpf i path calibiration input (high 2bits)_t lpf i path calibiration input (high 2bits);
	volatile trx_rssi high threshold for wifi_t rssi high threshold for wifi;
	volatile trx_lna load  capacitor tunning_t lna load  capacitor tunning;
	volatile trx_msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17_t msb of programming of pa current  100ua + 25ua*#lsb, lsb at reg11 bit17;
	volatile trx_modulator output tuning cap programming: +18ff/lsb (low q nodes)_t modulator output tuning cap programming: +18ff/lsb (low q nodes);
	volatile trx_(0: spi;  1: digital) power control enable_t (0: spi;  1: digital) power control enable;
	volatile trx_msb of pa current bias temperature  setting , another bits at 0x00[0]_t msb of pa current bias temperature  setting , another bits at 0x00[0];
	volatile trx_nc_t nc;
	volatile uint32_t rsv_f_11[3];
	volatile trx_0 = rest; 1=normal work_t 0 = rest; 1=normal work;
	volatile trx_cell delay control spi value_t cell delay control spi value;
	volatile trx_clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge_t clock(to sdm) polarity selection.
          0:  negedge 
          1:  posedge;
	volatile trx__t ;
	volatile trx_band calibration optimize enable._t band calibration optimize enable.;
	volatile trx_n value spi control_t n value spi control;
	volatile trx_vco amplitude auto calibration threshold control.._t vco amplitude auto calibration threshold control..;
	volatile trx__t ;
	volatile trx__t ;
} trx_hw_t;

#ifdef __cplusplus
}
#endif
