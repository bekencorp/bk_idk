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

/*
 * External PSRAM driver (QSPI indirect mode).
 *
 * bk_qspi_psram_write/read() issue one continuous SPI burst: CS stays low,
 * command+address sent once, then payload streamed in 256-byte FIFO chunks.
 * quad_write/quad_read() select wire mode and opcode for subsequent I/O.
 */

#include <driver/qspi.h>
#include <driver/qspi_psram.h>
#include "qspi_driver.h"
#include "qspi_psram.h"

/* Standard PSRAM SPI opcodes */
#define READ_ID_CMD            0x9F
#define RESET_ENABLE_CMD       0x66
#define RESET_CMD              0x99
#define ENTER_QUAD_MODE_CMD    0x35
#define EXIT_QUAD_MODE_CMD     0xF5
#define PSRAM_READ_ID_SIZE     8
#define QUAD_WRITE_CMD         0x38
#define WRITE_CMD              0x02
#define FAST_READ_QUAD         0xEB
#define FAST_READ              0x0B
#define READ_CMD               0x03
#define QSPI_FIFO_LEN_MAX      256  /* HW TX/RX FIFO depth */

/* Indirect read/write parameters; updated by quad/single_* helpers */
static qspi_wire_mode_t s_psram_wr_wire = QSPI_1WIRE;
static uint32_t s_psram_wr_cmd = WRITE_CMD;
static qspi_wire_mode_t s_psram_rd_wire = QSPI_1WIRE;
static uint32_t s_psram_rd_cmd = READ_CMD;
static uint32_t s_psram_rd_dummy = 0;

/* Send a command-only SPI frame (no address, no payload). */
static bk_err_t bk_qspi_psram_send_cmd(qspi_id_t id, uint32_t cmd)
{
	qspi_cmd_t spi_cmd = {0};

	spi_cmd.wire_mode = QSPI_1WIRE;
	spi_cmd.work_mode = INDIRECT_MODE;
	spi_cmd.op = QSPI_WRITE;
	spi_cmd.cmd = cmd;

	return bk_qspi_command(id, &spi_cmd);
}

/* Tear down a continuous burst: restore cmd/clk path and release CS. */
static void bk_qspi_psram_continuous_stop(qspi_id_t id)
{
	bk_qspi_disable_cmd_sck_mode(id, false);
	bk_qspi_force_cs_low(id, false);
}

/*
 * Indirect write as one SPI burst (CS held low for the whole transfer).
 *
 * Per-chunk flow:
 *   1. Fill TX FIFO (write_keep resume=true skips FIFO reset after 1st chunk)
 *   2. First chunk only: command_no_disable() sends cmd+addr, keeps CS low
 *   3. disable_cmd_sck: later chunks clock out data only, no repeated cmd
 *   4. Repeat until all bytes sent, then continuous_stop()
 */
static bk_err_t bk_qspi_psram_indirect_write(qspi_id_t id, uint32_t addr,
					     qspi_wire_mode_t wire_mode, uint32_t cmd,
					     const void *data, uint32_t size)
{
	qspi_cmd_t write_cmd = {0};
	uint32_t chunk_len;
	const uint8_t *payload = data;
	bool first = true;
	bk_err_t ret;

	if (size == 0) {
		return BK_OK;
	}

	write_cmd.wire_mode = wire_mode;
	write_cmd.work_mode = INDIRECT_MODE;
	write_cmd.op = QSPI_WRITE;
	write_cmd.cmd = cmd;

	/* Hold CS for entire transfer instead of per-FIFO frame */
	ret = bk_qspi_force_cs_low(id, true);
	if (ret != BK_OK) {
		bk_qspi_psram_continuous_stop(id);
		return ret;
	}

	while (size > 0) {
		chunk_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;

		/* resume=true on 2nd+ chunk: do not reset FIFO between chunks */
		ret = bk_qspi_write_keep(id, payload, chunk_len, !first);
		if (ret != BK_OK) {
			bk_qspi_psram_continuous_stop(id);
			return ret;
		}

		if (first) {
			write_cmd.addr = addr;
			write_cmd.data_len = chunk_len;
			/* Start transfer; do not deassert CS when this chunk finishes */
			ret = bk_qspi_command_no_disable(id, &write_cmd);
			if (ret != BK_OK) {
				bk_qspi_psram_continuous_stop(id);
				return ret;
			}
			/* Subsequent chunks: data phase only, no cmd/addr on bus */
			ret = bk_qspi_disable_cmd_sck_mode(id, true);
			if (ret != BK_OK) {
				bk_qspi_psram_continuous_stop(id);
				return ret;
			}
			first = false;
		}

		addr += chunk_len;
		payload += chunk_len;
		size -= chunk_len;
	}

	bk_qspi_psram_continuous_stop(id);
	return BK_OK;
}

/*
 * Indirect read as one SPI burst (CS held low for the whole transfer).
 *
 * Differs from write:
 *   - command_no_disable() runs before the first read_keep()
 *   - keep_clk=true on non-final chunks so CLK does not gap between FIFO reads
 */
static bk_err_t bk_qspi_psram_indirect_read(qspi_id_t id, uint32_t addr,
					    qspi_wire_mode_t wire_mode, uint32_t cmd,
					    uint32_t dummy_cycle, void *data, uint32_t size)
{
	qspi_cmd_t read_cmd = {0};
	uint32_t chunk_len;
	uint8_t *payload = data;
	bool first = true;
	bool first_data = true;
	bk_err_t ret;

	if (size == 0) {
		return BK_OK;
	}

	read_cmd.wire_mode = wire_mode;
	read_cmd.work_mode = INDIRECT_MODE;
	read_cmd.op = QSPI_READ;
	read_cmd.cmd = cmd;
	read_cmd.dummy_cycle = dummy_cycle;

	ret = bk_qspi_force_cs_low(id, true);
	if (ret != BK_OK) {
		bk_qspi_psram_continuous_stop(id);
		return ret;
	}

	while (size > 0) {
		chunk_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		/* More data follows: keep clock running after this FIFO drain */
		bool keep_clk = (size > chunk_len);

		if (first) {
			read_cmd.addr = addr;
			/* cmd + addr + dummy once; CS stays low */
			ret = bk_qspi_command_no_disable(id, &read_cmd);
			if (ret != BK_OK) {
				bk_qspi_psram_continuous_stop(id);
				return ret;
			}
			ret = bk_qspi_disable_cmd_sck_mode(id, true);
			if (ret != BK_OK) {
				bk_qspi_psram_continuous_stop(id);
				return ret;
			}
			first = false;
		}

		ret = bk_qspi_read_keep(id, payload, chunk_len, !first_data, keep_clk);
		if (ret != BK_OK) {
			bk_qspi_psram_continuous_stop(id);
			return ret;
		}
		first_data = false;

		addr += chunk_len;
		payload += chunk_len;
		size -= chunk_len;
	}

	bk_qspi_psram_continuous_stop(id);
	return BK_OK;
}

/* PSRAM software reset: 0x66 (enable) then 0x99 (reset). */
bk_err_t bk_qspi_psram_reset(qspi_id_t id)
{
	BK_LOG_ON_ERR(bk_qspi_psram_send_cmd(id, RESET_ENABLE_CMD));
	BK_LOG_ON_ERR(bk_qspi_psram_send_cmd(id, RESET_CMD));
	return BK_OK;
}

bk_err_t bk_qspi_psram_init(qspi_id_t id)
{
	qspi_config_t config = {0};
	config.src_clk = QSPI_SCLK_480M;
	config.src_clk_div = 0x4;
	config.clk_div = 0x10;
	BK_LOG_ON_ERR(bk_qspi_init(id, &config));
	BK_LOG_ON_ERR(bk_qspi_psram_reset(id));
	/* Default to 1-wire until enter_quad_mode + quad_write/read */
	s_psram_wr_wire = QSPI_1WIRE;
	s_psram_wr_cmd = WRITE_CMD;
	s_psram_rd_wire = QSPI_1WIRE;
	s_psram_rd_cmd = READ_CMD;
	s_psram_rd_dummy = 0;
	return BK_OK;
}

bk_err_t bk_qspi_psram_deinit(qspi_id_t id)
{
	BK_LOG_ON_ERR(bk_qspi_deinit(id));
	return BK_OK;
}

bk_err_t bk_qspi_psram_enter_quad_mode(qspi_id_t id)
{
	qspi_cmd_t enter_quad_cmd = {0};

	enter_quad_cmd.wire_mode = QSPI_1WIRE;
	enter_quad_cmd.work_mode = INDIRECT_MODE;
	enter_quad_cmd.op = QSPI_WRITE;
	enter_quad_cmd.cmd = ENTER_QUAD_MODE_CMD;

	BK_LOG_ON_ERR(bk_qspi_command(id, &enter_quad_cmd));
	return BK_OK;
}

bk_err_t bk_qspi_psram_exit_quad_mode(qspi_id_t id)
{
	qspi_cmd_t exit_quad_cmd = {0};

	exit_quad_cmd.wire_mode = QSPI_4WIRE;
	exit_quad_cmd.work_mode = INDIRECT_MODE;
	exit_quad_cmd.op = QSPI_WRITE;
	exit_quad_cmd.cmd = EXIT_QUAD_MODE_CMD;

	BK_LOG_ON_ERR(bk_qspi_command(id, &exit_quad_cmd));
	return BK_OK;
}

/*
 * Configure quad write (0x38) for indirect I/O.
 * Also programs MEMORY_MAPPED_MODE for legacy mmap users.
 */
bk_err_t bk_qspi_psram_quad_write(qspi_id_t id)
{
	qspi_cmd_t quad_wr_cmd = {0};

	s_psram_wr_wire = QSPI_4WIRE;
	s_psram_wr_cmd = QUAD_WRITE_CMD;
	quad_wr_cmd.wire_mode = QSPI_4WIRE;
	quad_wr_cmd.work_mode = MEMORY_MAPPED_MODE;
	quad_wr_cmd.op = QSPI_WRITE;
	quad_wr_cmd.cmd = QUAD_WRITE_CMD;
	quad_wr_cmd.addr = 0;
	quad_wr_cmd.dummy_cycle = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &quad_wr_cmd));
	return BK_OK;
}

bk_err_t bk_qspi_psram_single_write(qspi_id_t id)
{
	qspi_cmd_t quad_wr_cmd = {0};

	s_psram_wr_wire = QSPI_1WIRE;
	s_psram_wr_cmd = WRITE_CMD;
	quad_wr_cmd.wire_mode = QSPI_1WIRE;
	quad_wr_cmd.work_mode = MEMORY_MAPPED_MODE;
	quad_wr_cmd.op = QSPI_WRITE;
	quad_wr_cmd.cmd = WRITE_CMD;
	quad_wr_cmd.addr = 0;
	quad_wr_cmd.dummy_cycle = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &quad_wr_cmd));
	return BK_OK;
}

/*
 * Configure quad fast read (0xEB, 6 dummy) for indirect I/O.
 * Also programs MEMORY_MAPPED_MODE for legacy mmap users.
 */
bk_err_t bk_qspi_psram_quad_read(qspi_id_t id)
{
	qspi_cmd_t qspi_rd_cmd = {0};

	s_psram_rd_wire = QSPI_4WIRE;
	s_psram_rd_cmd = FAST_READ_QUAD;
	s_psram_rd_dummy = 6;
	qspi_rd_cmd.wire_mode = QSPI_4WIRE;
	qspi_rd_cmd.work_mode = MEMORY_MAPPED_MODE;
	qspi_rd_cmd.op = QSPI_READ;
	qspi_rd_cmd.cmd = FAST_READ_QUAD;
	qspi_rd_cmd.addr = 0;
	qspi_rd_cmd.dummy_cycle = 6;

	BK_LOG_ON_ERR(bk_qspi_command(id, &qspi_rd_cmd));
	return BK_OK;
}

bk_err_t bk_qspi_psram_single_read(qspi_id_t id)
{
	qspi_cmd_t qspi_rd_cmd = {0};

	s_psram_rd_wire = QSPI_1WIRE;
	s_psram_rd_cmd = READ_CMD;
	s_psram_rd_dummy = 0;
	qspi_rd_cmd.wire_mode = QSPI_1WIRE;
	qspi_rd_cmd.work_mode = MEMORY_MAPPED_MODE;
	qspi_rd_cmd.op = QSPI_READ;
	qspi_rd_cmd.cmd = READ_CMD;
	qspi_rd_cmd.addr = 0;
	qspi_rd_cmd.dummy_cycle = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &qspi_rd_cmd));
	return BK_OK;
}

/* Public write: continuous indirect burst using s_psram_wr_* settings. */
bk_err_t bk_qspi_psram_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	return bk_qspi_psram_indirect_write(id, base_addr, s_psram_wr_wire,
					    s_psram_wr_cmd, data, size);
}

/* Public read: continuous indirect burst using s_psram_rd_* settings. */
bk_err_t bk_qspi_psram_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	return bk_qspi_psram_indirect_read(id, base_addr, s_psram_rd_wire,
					   s_psram_rd_cmd, s_psram_rd_dummy, data, size);
}

/* Read 8-byte electronic ID (cmd 0x9F) for chip detection. */
bk_err_t bk_qspi_psram_read_eid(qspi_id_t id, uint8_t *eid, uint32_t size)
{
	qspi_cmd_t read_id_cmd = {0};

	if (eid == NULL || size == 0) {
		return BK_ERR_PARAM;
	}

	if (size > PSRAM_READ_ID_SIZE) {
		size = PSRAM_READ_ID_SIZE;
	}

	read_id_cmd.wire_mode = QSPI_1WIRE;
	read_id_cmd.work_mode = INDIRECT_MODE;
	read_id_cmd.op = QSPI_READ;
	read_id_cmd.device = QSPI_PSRAM;
	read_id_cmd.cmd = READ_ID_CMD;
	read_id_cmd.addr = 0;
	read_id_cmd.data_len = size;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));
	BK_LOG_ON_ERR(bk_qspi_read(id, eid, size));
	return BK_OK;
}

