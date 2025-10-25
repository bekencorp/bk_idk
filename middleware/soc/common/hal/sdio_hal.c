// Copyright 2020-2025 Beken
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

#include "sdio_hal.h"
#include "sdio_ll.h"

void sdio_hal_set_sd_soft_resetn(uint32_t value)
{
	sdio_ll_set_sd_soft_resetn(value);
}

__attribute__((section(".itcm_sec_code"))) void sdio_hal_slave_cmd_start(uint32_t value)
{
	sdio_ll_set_sd_cmd_start(value);
}

void sdio_hal_set_sd_data_en(uint32_t value)
{
	sdio_ll_set_sd_data_en(value);
}

void sdio_hal_set_sd_data_stop_en(uint32_t value)
{
	sdio_ll_set_sd_data_stop_en(value);
}

void sdio_hal_rx_set_sd_byte_sel(bool big_endian)
{
	sdio_ll_set_sd_byte_sel(big_endian);
}

void sdio_hal_set_sd_data_bus(uint32_t value)
{
	sdio_ll_set_sd_data_bus(value);
}

/* REG_0x09:reg0x9->CMD_S_RES_END_INT:0x9[24],Slave only; Slave has finish reponsed the CMD to host side.,0x0,R/W1C*/
__attribute__((section(".iram_sec_code"))) bool sdio_hal_slave_get_cmd_response_end_int(void)
{
    return (bool)sdio_ll_get_cmd_s_res_end_int();
}

void sdio_hal_slave_clear_cmd_response_end_int(void)
{
	//write 1 to clear INT status, 0 do nothing.
    sdio_ll_set_cmd_s_res_end_int(1);
}

/* REG_0x0a:reg0xa->TX_FIFO_NEED_WRITE_MASK_CG:0xa[13],1:sd host fifo memory need write mask for clk gate writing use only,RW*/
void sdio_hal_host_set_tx_fifo_need_write_mask_cg(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_tx_fifo_need_write_mask_cg(value);
}

uint32_t sdio_hal_get_read_ready(void)
{
	return sdio_ll_get_rxfifo_rd_ready();
}

uint32_t sdio_hal_get_write_ready(void)
{
	return sdio_ll_get_txfifo_wr_ready();
}

//BK7256 only:BK7256 SDIO host&slave uses only one IP controller and controlled by REG.
void sdio_hal_set_host_slave_mode(sdio_host_slave_mode_t mode)
{
	sdio_ll_set_sd_slave(mode);
}

void sdio_hal_set_sd_cmd_crc_check(uint32_t value)
{
	sdio_ll_set_sd_cmd_crc_check(value);
}

void sdio_hal_set_cmd_keep_det(uint32_t value)
{
	sdio_ll_set_cmd_keep_det(value);
}

uint32_t sdio_hal_slave_read_data(void)
{
	return sdio_ll_get_rx_fifo_dout();
}

void sdio_hal_slave_write_data(uint32_t value)
{
	sdio_ll_set_reg0xf_value(value);
}

uint32_t sdio_hal_get_int_status(void)
{
	return sdio_ll_get_int_status();
}

uint32_t sdio_hal_slave_get_read_int_status(void)
{
	return sdio_ll_get_dat_s_rd_bus_int();
}

void sdio_hal_slave_clear_read_int_status(void)
{
	sdio_ll_set_dat_s_rd_bus_int(1);
}

uint32_t sdio_hal_slave_get_rx_count(void)
{
	return sdio_ll_get_sd_slave_status_cmd_s_rec_bb_cnt();
}

void sdio_hal_slave_notify_host_next_block(void)
{
	sdio_ll_set_dat_s_rd_mul_blk(0);

	//confirm the reg status is stable.delay time should more then one cycle of SDIO
	for(volatile int i = 0; i < 200; i++);

	sdio_ll_set_dat_s_rd_mul_blk(1);
}

void sdio_hal_set_dat_s_rd_mul_blk(uint32_t value)
{
	sdio_ll_set_dat_s_rd_mul_blk(value);
}

uint32 sdio_hal_slave_get_func_reg_value(void)
{
	return sdio_ll_get_reg0x13_value();
}

//SW:private protocal,host read this reg value as the slave send packet length(UNIT is block count, each block size is SDIO_BLK_SIZE==512 bytes)
__attribute__((section(".itcm_sec_code"))) void sdio_hal_slave_set_tx_length(uint32_t len)
{
	sdio_ll_set_reg0x13_value(len);
}

void sdio_hal_slave_clear_stop(void)
{
	sdio_ll_set_cmd_52_stop_clr(1);
}

void sdio_hal_slave_tx_transaction_en(void)
{
	sdio_ll_set_sd_start_wr_en(1);
}

//host write, slave read, if slave fifo is full, slave will stop host to write data
void sdio_hal_slave_rx_clear_host_wait_write_data(void)
{
	sdio_ll_set_dat_s_wr_wai_int(1);
}

#if CONFIG_SDIO_SLAVE

uint32_t sdio_hal_slave_get_samp_sel(void)
{
    return sdio_ll_get_samp_sel();
}

void sdio_hal_slave_set_samp_sel(uint32_t value)
{
	sdio_ll_set_samp_sel(value);
}

__attribute__((section(".iram_sec_code"))) uint32_t sdio_hal_slave_get_cmd_arg0(void)
{
	return sdio_ll_get_sd_rsp_agument_0();
}

uint32_t sdio_hal_get_tx_fifo_empty_int_status(void)
{
    return sdio_ll_get_tx_fifo_empt();
}

void sdio_hal_clear_tx_fifo_empty_int_status(void)
{
    sdio_ll_clear_tx_fifo_empt();
}

//NOTES:this is for slave read block end
uint32_t sdio_hal_slave_get_data_rec_end_int(void)
{
	return sdio_ll_get_sd_data_rec_end_int();
}

void sdio_hal_slave_clear_data_rec_end_int(void)
{
	sdio_ll_set_sd_data_rec_end_int(1);
}

//NOTES:be care this is for slave write end, and sdio_ll_get_sd_data_wr_end_int is for host write end
uint32_t sdio_hal_slave_get_wr_end_int(void)
{
    return sdio_ll_get_dat_s_wr_wai_int();
}

void sdio_hal_slave_clear_wr_end_int(void)
{
	//write 1 to clear INT status, 0 do nothing.
    sdio_ll_set_dat_s_wr_wai_int(1);
}

void sdio_hal_set_tx_fifo_empty_int(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_tx_fifo_empt_mask(value);
}

void sdio_hal_slave_set_cmd_res_end_int(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_cmd_s_res_end_int_mask(value);
}

void sdio_hal_slave_set_data_rec_end_int(uint32_t value)
{
    sdio_ll_set_sd_data_rec_end_mask(value);
}

void sdio_hal_slave_set_write_end_int(uint32_t value)
{
    sdio_ll_set_dat_s_wr_wai_int_mask(value);
}

void sdio_hal_slave_set_read_end_int(uint32_t value)
{
    sdio_ll_set_dat_s_rd_bus_int_mask(value);
}

void sdio_hal_slave_set_blk_size(uint32_t value)
{
	sdio_ll_set_sd_data_blk_size(value);
}

uint32_t sdio_ll_get_sd_slave_wr_finish(void)
{
	return sdio_ll_get_sd_slave_status_sd_start_wr_en_r3();
}

//WARNING:it will reset fifo status.
void sdio_hal_fifo_reset(void)
{
	sdio_ll_set_tx_fifo_rst();
	sdio_ll_set_rx_fifo_rst();
	//no need reset status, if reseting sdio when host send CMD,the CMD can't be decoded by sdio hardware.
#if CONFIG_SDIO_V2P1 || CONFIG_SDIO_V2P2
	sdio_ll_set_sd_sta_rst();
#endif
}

#if CONFIG_SDIO_V2P1 || CONFIG_SDIO_V2P2
void sdio_hal_slave_clear_wr_blk_int_status(void)
{
	sdio_ll_set_dat_s_wr_blk_int(1);
}

uint32_t sdio_hal_slave_get_wr_blk_int_status(void)
{
	return sdio_ll_get_dat_s_wr_blk_int();
}

void sdio_hal_slave_set_write_block_int_mask(uint32_t en)
{
	sdio_ll_slave_set_write_block_int_mask(en);
}

void sdio_hal_slave_set_write_mul_sel(uint32_t en)
{
	sdio_ll_set_write_mul_sel(en);
}

void sdio_hal_slave_clear_rd_blk_int_status(void)
{
	sdio_ll_set_dat_wrsts_err_int(1);
}

uint32_t sdio_hal_slave_get_rd_blk_int_status(void)
{
	return sdio_ll_get_dat_wrsts_err_int();
}

void sdio_hal_slave_set_read_block_int_mask(uint32_t en)
{
	sdio_ll_slave_set_read_block_int_mask(en);
}

void sdio_hal_slave_set_read_mul_sel(uint32_t en)
{
	sdio_ll_set_read_mul_sel(en);
}

//register addr from host side is:0,1,2,3 bytes
void sdio_hal_slave_set_arg1_val(uint32_t v)
{
	sdio_ll_set_reg0x13_value(v);
}

uint32_t sdio_hal_slave_get_arg1_val(void)
{
	return sdio_ll_get_reg0x13_value();
}

//register addr from host side is:4,5,6,7 bytes
void sdio_hal_slave_set_arg2_val(uint32_t v)
{
	sdio_ll_set_reg0x14_value(v);
}

uint32_t sdio_hal_slave_get_arg2_val(void)
{
	return sdio_ll_get_reg0x14_value();
}

#if !CONFIG_SOC_BK7236
void sdio_hal_cmd_sst_sel_en(uint32_t value)
{
	sdio_ll_cmd_sst_sel_en(value);
}

void sdio_hal_dat1_int_en(uint32_t value)
{
	sdio_ll_dat1_int_en(value);
}

void sdio_hal_cccr_intx_en(uint32_t value)
{
	sdio_ll_cccr_intx_en(value);
}

void sdio_hal_cccr_s4mi_en(uint32_t value)
{
	sdio_ll_cccr_s4mi_en(value);
}

/* REG_0x3f:reg0x3f->cmd_s_buf_cnt_clr:0x3f[0],cmd buffer 计数器清零,0x0,RW*/
void sdio_hal_set_reg0x3f_cmd_s_buf_cnt_clr(uint32_t value)
{
	sdio_ll_set_reg0x3f_cmd_s_buf_cnt_clr(value);
}

/* REG_0x3f:reg0x3f->cmd_s_buf_cnt_num:0x3f[3:1],设置cmd buffer 缓存深度，最大深度值为5; 缓存深度=cmd_s_buf_cnt_num+1,0x0,RW*/
void sdio_hal_set_reg0x3f_cmd_s_buf_cnt_num(uint32_t value)
{
	sdio_ll_set_reg0x3f_cmd_s_buf_cnt_num(value);
}

/* REG_0x3f:reg0x3f->cmd_s_buf_cnt:0x3f[6:4],cmd buffer 缓存个数; 0：1个缓存的CMD; 1：2个缓存的CMD; 2：3个缓存的CMD; 3：4个缓存的CMD; 4：5个缓存的CMD ,0x0,R*/
uint32_t sdio_hal_get_reg0x3f_cmd_s_buf_cnt(void)
{
	return sdio_ll_get_reg0x3f_cmd_s_buf_cnt();
}

/* REG_0x3f:reg0x3f->cmd_s_buf_cover_clr:0x3f[7],cmd buffer 溢出标志位清零,0x0,RW*/
void sdio_hal_set_reg0x3f_cmd_s_buf_cover_clr(uint32_t value)
{
	sdio_ll_set_reg0x3f_cmd_s_buf_cover_clr(value);
}

/* REG_0x3f:reg0x3f->cmd_s_buf_cover:0x3f[8],cmd buffer 溢出标志位,0x0,R*/
uint32_t sdio_hal_get_reg0x3f_cmd_s_buf_cover(void)
{
	return sdio_ll_get_reg0x3f_cmd_s_buf_cover();
}

/* REG_0x3f:reg0x3f->cmd_s_buf_en:0x3f[9],cmd buffer enable; 注：先把cmd_s_buf_cnt_num深度值设置完成，再使能cmd_s_buf_en,0x0,RW*/
void sdio_hal_set_reg0x3f_cmd_s_buf_en(uint32_t value)
{
	sdio_ll_set_reg0x3f_cmd_s_buf_en(value);
}

/* REG_0x3f:reg0x3f->cmd_s_cnt_clr:0x3f[10],cmd计数器清零,0x0,RW*/
void sdio_hal_set_reg0x3f_cmd_s_cnt_clr(uint32_t value)
{
	sdio_ll_set_reg0x3f_cmd_s_cnt_clr(value);
}

uint32_t sdio_hal_get_reg0x40_value(void)
{
	return sdio_ll_get_reg0x40_value();
}

uint32_t sdio_hal_get_reg0x41_cmd_s_rec_argument_buf0(void)
{
	return sdio_ll_get_reg0x41_cmd_s_rec_argument_buf0();
}

uint32_t sdio_hal_get_reg0x42_cmd_s_rec_argument_buf1(void)
{
	return sdio_ll_get_reg0x42_cmd_s_rec_argument_buf1();
}

uint32_t sdio_hal_get_reg0x43_cmd_s_rec_argument_buf2(void)
{
	return sdio_ll_get_reg0x43_cmd_s_rec_argument_buf2();
}

uint32_t sdio_hal_get_reg0x44_cmd_s_rec_argument_buf3(void)
{
	return sdio_ll_get_reg0x44_cmd_s_rec_argument_buf3();
}

uint32_t sdio_hal_get_reg0x45_cmd_s_rec_argument_buf4(void)
{
	return sdio_ll_get_reg0x45_cmd_s_rec_argument_buf4();
}

uint32_t sdio_hal_get_reg0x46_cmd_s_cnt(void)
{
	return sdio_ll_get_reg0x46_cmd_s_cnt();
}
#endif
#endif	//CONFIG_SDIO_V2P1

#endif
