/*
 * Copyright 2020-2025 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sdkconfig.h>

#if CONFIG_AT && CONFIG_OTA_HTTP

#include "http_atsvr_http.h"
#include "http_at_command.h"
#include "atsvr_unite.h"
#include "at_server.h"
#include "atsvr_port.h"
#include <common/bk_err.h>
#include <components/log.h>
#include <driver/uart.h>
#include "utils_httpc.h"
#include <os/os.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_ATSVR_TAG "http_atsvr"

/* AT+HTTPSCERT command */
#define HTTPSCERT_DATA_MAX 2048

void http_at_post_result_hook(int http_code, int cli_ret, const char *resp_buf, uint32_t resp_len)
{
	char msg[288];

	if (resp_buf != NULL && resp_len > 0) {
		unsigned int pl = resp_len > 120U ? 120U : resp_len;

		snprintf(msg, sizeof(msg), "\r\nCMDRSP:HTTPCPOST:%d,%d,%.*s\r\n", http_code, cli_ret, (int)pl,
			 resp_buf);
	} else {
		snprintf(msg, sizeof(msg), "\r\nCMDRSP:HTTPCPOST:%d,%d\r\n", http_code, cli_ret);
	}

	atsvr_output_msg(msg);
}

static int http_at_atsvr_getsize_handle(int sync, int argc, char **argv)
{
	char msg[96];
	uint32_t sz = 0;
	int n;

	(void)sync;

	if (argc != 1 || argv[0] == NULL) {
		BK_LOGE(HTTP_ATSVR_TAG, "Usage: AT+HTTPGETSIZE=<url>\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	if (httpclient_get_content_length_by_url(argv[0], 30000, &sz) != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	n = snprintf(msg, sizeof(msg), "\r\nCMDRSP:HTTPGETSIZE:%lu\r\n", (unsigned long)sz);
	if (n <= 0 || n >= (int)sizeof(msg)) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	atsvr_output_msg(msg);
	atsvr_cmd_rsp_ok();
	return 0;
}

static int http_at_atsvr_httpclient_handle(int sync, int argc, char **argv)
{
	httpclient_t client;
	httpclient_data_t data;
	char resp[HTTP_RESP_CONTENT_LEN];
	char rsp_msg[288];
	const char *url = NULL;
	const char *req_body = NULL;
	const char *post_content_type = "application/x-www-form-urlencoded";
	const char *accept_hdr = "Accept: text/xml,text/html,\r\n";
	int method = HTTPCLIENT_HEAD;
	int content_type = 0;
	int ret;
	unsigned int rl = 0;
	int n;

	(void)sync;

	if (argc < 3 || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
		BK_LOGE(HTTP_ATSVR_TAG,
			"Usage: AT+HTTPCLIENT=<opt>,<content-type>,<url>[,<data>]\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	content_type = atoi(argv[1]);
	url = argv[2];
	if (argc >= 4 && argv[3] && argv[3][0] != '\0')
		req_body = argv[3];

	switch (atoi(argv[0])) {
	case 1: method = HTTPCLIENT_HEAD; break;
	case 2: method = HTTPCLIENT_GET; break;
	case 3: method = HTTPCLIENT_POST; break;
	case 4: method = HTTPCLIENT_PUT; break;
	case 5: method = HTTPCLIENT_DELETE; break;
	default: method = HTTPCLIENT_HEAD; break;
	}

	switch (content_type) {
	case 0:
		accept_hdr = "Accept: application/x-www-form-urlencoded\r\n";
		post_content_type = "application/x-www-form-urlencoded";
		break;
	case 1:
		accept_hdr = "Accept: application/json\r\n";
		post_content_type = "application/json";
		break;
	case 2:
		/* Keep SDK3.0-compatible literal */
		accept_hdr = "Accept: multipart/for-data\r\n";
		post_content_type = "multipart/for-data";
		break;
	case 3:
	default:
		accept_hdr = "Accept: text/xml\r\n";
		post_content_type = "text/xml";
		break;
	}

	if (url == NULL || url[0] == '\0') {
		BK_LOGE(HTTP_ATSVR_TAG, "HTTPCLIENT url invalid\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}
	if (strncmp(url, "https://", strlen("https://")) == 0) {
		BK_LOGE(HTTP_ATSVR_TAG,
			"HTTPCLIENT supports HTTP only, https is not supported in this command\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	BK_LOGI(HTTP_ATSVR_TAG, "HTTPCLIENT method=%d ctype=%d url=%s\r\n",
		method, content_type, url);

	memset(&client, 0, sizeof(client));
	memset(&data, 0, sizeof(data));
	memset(resp, 0, sizeof(resp));

	client.header = (char *)accept_hdr;
	data.post_buf = (char *)req_body;
	data.post_buf_len = req_body ? (uint32_t)strlen(req_body) : 0;
	data.post_content_type = (char *)post_content_type;
	data.response_buf = resp;
	data.response_buf_len = sizeof(resp);
	data.response_content_len = sizeof(resp);

	ret = httpclient_common(&client, url, 80, NULL, method, 30000, &data);
	if (ret != 0) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	/* response_content_len is binary-safe; do not use strnlen() for OTA/bin payload */
	if (data.response_content_len > 0)
		rl = (unsigned int)data.response_content_len;
	else if (data.response_buf && data.response_buf_len > 1)
		rl = (unsigned int)strnlen(data.response_buf, data.response_buf_len - 1);

	n = snprintf(rsp_msg, sizeof(rsp_msg), "\r\n+HTTPCLIENT:<%u>\r\n", rl);
	if (n > 0)
		atsvr_output_msg(rsp_msg);

	atsvr_cmd_rsp_ok();
	return 0;
}

static int http_atsvr_uart_read_exact(uart_id_t id, uint8_t *buf, uint32_t len)
{
	uint32_t got = 0;

	while (got < len) {
		int r = bk_uart_read_bytes(id, buf + got, len - got, BEKEN_WAIT_FOREVER);

		if (r < 0)
			return -1;
		got += (uint32_t)r;
	}
	return 0;
}

static void http_atsvr_post_uart_thread(beken_thread_arg_t arg)
{
	void *job = (void *)arg;
	uint8_t *buf;
	unsigned long blen;

	if (job == NULL) {
		rtos_delete_thread(NULL);
		return;
	}

	buf = http_at_post_job_body_ptr(job);
	blen = http_at_post_job_body_len(job);
	if (buf == NULL || blen == 0UL) {
		http_at_post_job_abort(job);
		rtos_delete_thread(NULL);
		return;
	}

	atsvr_set_uart_rx_passthrough_lock(1);
	atsvr_cmd_rsp_passthrough();

	if (http_atsvr_uart_read_exact((uart_id_t)AT_UART_PORT_CFG, buf, (uint32_t)blen) != 0) {
		BK_LOGE(HTTP_ATSVR_TAG, "HTTPCPOST uart read failed\r\n");
		http_at_post_job_abort(job);
		atsvr_set_uart_rx_passthrough_lock(0);
		rtos_delete_thread(NULL);
		return;
	}

	http_at_post_job_execute(job);
	atsvr_set_uart_rx_passthrough_lock(0);
	rtos_delete_thread(NULL);
}

typedef struct {
	int recvtype;
	unsigned long recvdatalen;
} httpscert_job_t;

static void http_atsvr_httpscert_uart_thread(beken_thread_arg_t arg)
{
	httpscert_job_t *job = (httpscert_job_t *)arg;
	uint8_t *cert_buf = NULL;
	uint32_t need_len;

	if (job == NULL) {
		rtos_delete_thread(NULL);
		return;
	}

	need_len = (job->recvtype == 1) ? (uint32_t)job->recvdatalen : HTTPSCERT_DATA_MAX;
	if (need_len == 0U) {
		free(job);
		rtos_delete_thread(NULL);
		return;
	}

	atsvr_set_uart_rx_passthrough_lock(1);
	cert_buf = (uint8_t *)malloc(need_len + 1U);
	if (cert_buf == NULL) {
		atsvr_set_uart_rx_passthrough_lock(0);
		free(job);
		rtos_delete_thread(NULL);
		return;
	}

	atsvr_cmd_rsp_passthrough();

	if (http_atsvr_uart_read_exact((uart_id_t)AT_UART_PORT_CFG, cert_buf, need_len) != 0) {
		BK_LOGE(HTTP_ATSVR_TAG, "HTTPSCERT uart read failed\r\n");
		free(cert_buf);
		free(job);
		atsvr_set_uart_rx_passthrough_lock(0);
		rtos_delete_thread(NULL);
		return;
	}

	cert_buf[need_len] = '\0';
#if CONFIG_HTTP
	(void)http_at_set_https_ca_crt(cert_buf, (unsigned long)need_len);
#else
	(void)cert_buf;
#endif

	free(cert_buf);
	free(job);
	atsvr_set_uart_rx_passthrough_lock(0);
	rtos_delete_thread(NULL);
}

/*
 * Match SDK: AT+HTTPCPOST=<url>,<length>[,<http_req_head_cnt>][,<http_req_header>][,<port>][,<timeout_ms>][,<content_type>]
 * After OK, prints passthrough prompt then reads exactly <length> raw bytes from AT UART as POST body.
 */
static int http_at_atsvr_post_handle(int sync, int argc, char **argv)
{
	unsigned long body_len;
	int port = 0;
	unsigned long timeout_ms = 30000;
	const char *ctype = NULL;
	void *job;
	UINT32 th_ret;

	(void)sync;

	if (argc < 2 || argv[0] == NULL || argv[1] == NULL) {
		BK_LOGE(HTTP_ATSVR_TAG,
			"Usage: AT+HTTPCPOST=<url>,<len>[,<head_cnt>][,<header>][,<port>][,<timeout_ms>][,<content_type>]\r\n"
			"  (or legacy: AT+HTTPCPOST=<url>,<len>,<port>,<timeout_ms>)\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	body_len = strtoul(argv[1], NULL, 10);

	if (argc == 3) {
		if (argv[2] != NULL && argv[2][0] != '\0')
			port = atoi(argv[2]);
	} else if (argc == 4) {
		if (argv[2] != NULL && argv[2][0] != '\0')
			port = atoi(argv[2]);
		if (argv[3] != NULL && argv[3][0] != '\0')
			timeout_ms = strtoul(argv[3], NULL, 10);
	} else if (argc >= 5) {
		int pi = 2;

		pi++;
		pi++;
		if (argc > pi && argv[pi] != NULL && argv[pi][0] != '\0')
			port = atoi(argv[pi]);
		pi++;
		if (argc > pi && argv[pi] != NULL && argv[pi][0] != '\0')
			timeout_ms = strtoul(argv[pi], NULL, 10);
		pi++;
		if (argc > pi && argv[pi] != NULL && argv[pi][0] != '\0')
			ctype = argv[pi];
	}

	job = http_at_post_job_create(argv[0], body_len, port, timeout_ms, ctype);
	if (job == NULL) {
		atsvr_cmd_rsp_error();
		return -1;
	}

	/* Lock UART RX parser before starting passthrough worker thread,
	 * so we don't race in the small window after user starts sending data. */
	atsvr_set_uart_rx_passthrough_lock(1);
	th_ret = rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "http_post_uart",
				    (beken_thread_function_t)http_atsvr_post_uart_thread, 5120,
				    (beken_thread_arg_t)job);
	if (th_ret != kNoErr) {
		BK_LOGE(HTTP_ATSVR_TAG, "HTTPCPOST thread fail %u\r\n", th_ret);
		http_at_post_job_abort(job);
		atsvr_cmd_rsp_error();
		atsvr_set_uart_rx_passthrough_lock(0);
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

static int http_at_atsvr_httpscert_handle(int sync, int argc, char **argv);

static const struct _atsvr_command http_atsvr_cmds_table[] = {
	ATSVR_CMD_HADLER("AT+HTTPCLIENT",
			 "http request:AT+HTTPCLIENT=<opt>,<content-type>,<url>[,<data>]",
			 NULL, http_at_atsvr_httpclient_handle, false, 0, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+HTTPGETSIZE", "query http resource size:AT+HTTPGETSIZE=<url>",
			 NULL, http_at_atsvr_getsize_handle, false, 0, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+HTTPCPOST",
			 "uart body then HTTP POST:AT+HTTPCPOST=<url>,<len>[,<head_cnt>][,<header>][,<port>][,<t_ms>][,<ctype>]",
			 NULL, http_at_atsvr_post_handle, false, 0, 0, NULL, false),
	ATSVR_CMD_HADLER("AT+HTTPSCERT",
			 "write HTTPS CA cert:AT+HTTPSCERT=<type>,<lenth>",
			 NULL, http_at_atsvr_httpscert_handle, false, 0, 0, NULL, false),
};

static int http_at_atsvr_httpscert_handle(int sync, int argc, char **argv)
{
	int recvtype = 0;
	unsigned long recvdatalen = 0UL;
	httpscert_job_t *job;
	UINT32 th_ret;

	(void)sync;

	if (argc < 2 || argv[0] == NULL || argv[1] == NULL) {
		BK_LOGE(HTTP_ATSVR_TAG, "Usage: AT+HTTPSCERT=<type>,<lenth>\r\n");
		atsvr_cmd_rsp_error();
		return -1;
	}

	recvtype = atoi(argv[0]);
	recvdatalen = strtoul(argv[1], NULL, 10);

	job = (httpscert_job_t *)malloc(sizeof(httpscert_job_t));
	if (job == NULL) {
		atsvr_cmd_rsp_error();
		return -1;
	}
	job->recvtype = recvtype;
	job->recvdatalen = recvdatalen;

	atsvr_set_uart_rx_passthrough_lock(1);
	th_ret = rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "httpscert_uart",
				    (beken_thread_function_t)http_atsvr_httpscert_uart_thread, 5120,
				    (beken_thread_arg_t)job);
	if (th_ret != kNoErr) {
		free(job);
		atsvr_cmd_rsp_error();
		atsvr_set_uart_rx_passthrough_lock(0);
		return -1;
	}

	atsvr_cmd_rsp_ok();
	return 0;
}

void http_at_atsvr_init(void)
{
	atsvr_register_commands(http_atsvr_cmds_table,
				sizeof(http_atsvr_cmds_table) / sizeof(http_atsvr_cmds_table[0]),
				"http_at", NULL);
}

#endif
