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

#if (CONFIG_AT_CMD || CONFIG_AT) && CONFIG_OTA_HTTP

#include "http_at_command.h"

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CONFIG_AT_CMD
#include "at_common.h"
#endif

#if CONFIG_AT && CONFIG_HTTP
#include "utils_httpc.h"
#endif
#if CONFIG_WIFI_ENABLE
#include <modules/wifi.h>
#include <modules/wifi_types.h>
#endif

#define HTTP_AT_TAG "http_at"

#ifndef HTTP_POST_BODY_MAX
#define HTTP_POST_BODY_MAX (8192)
#endif

#if CONFIG_AT

static volatile unsigned int s_http_post_busy;

static char *s_https_ca_crt_pem;

typedef struct {
	char *url;
	uint8_t *body;
	uint32_t body_len;
	int port;
	uint32_t timeout_ms;
	char content_type[64];
} http_post_ctx_t;

/*
 * utils_httpc httpclient_parse_url() requires a '/' after host[:port]. Append '/' when missing
 * (e.g. http://192.168.1.1 -> http://192.168.1.1/).
 */
static char *http_at_strdup_url_for_httpc(const char *url)
{
	size_t len;
	char *copy;
	const char *scheme;

	if (url == NULL || url[0] == '\0')
		return NULL;

	scheme = strstr(url, "://");
	if (scheme != NULL) {
		const char *hostpart = scheme + 3;

		if (strchr(hostpart, '/') == NULL) {
			const char *qmark = strchr(hostpart, '?');

			len = strlen(url);
			if (qmark != NULL) {
				size_t prefix = (size_t)(qmark - url);

				copy = (char *)os_malloc(len + 2);
				if (copy == NULL)
					return NULL;
				memcpy(copy, url, prefix);
				copy[prefix] = '/';
				memcpy(copy + prefix + 1, qmark, strlen(qmark) + 1);
				return copy;
			}

			copy = (char *)os_malloc(len + 2);
			if (copy == NULL)
				return NULL;
			memcpy(copy, url, len + 1);
			copy[len] = '/';
			copy[len + 1] = '\0';
			return copy;
		}
	}

	len = strlen(url);
	copy = (char *)os_malloc(len + 1);
	if (copy == NULL)
		return NULL;
	memcpy(copy, url, len + 1);
	return copy;
}

#if CONFIG_HTTP

int http_at_set_https_ca_crt(const uint8_t *pem, unsigned long pem_len)
{
	char *new_buf;

	if (pem == NULL || pem_len == 0UL) {
		return -1;
	}

	new_buf = (char *)os_malloc((size_t)pem_len + 1U);
	if (new_buf == NULL) {
		return -1;
	}

	memcpy(new_buf, pem, (size_t)pem_len);
	new_buf[pem_len] = '\0';

	if (s_https_ca_crt_pem) {
		os_free(s_https_ca_crt_pem);
		s_https_ca_crt_pem = NULL;
	}
	s_https_ca_crt_pem = new_buf;
	return 0;
}

const char *http_at_get_https_ca_crt(void)
{
	return s_https_ca_crt_pem;
}

__attribute__((weak)) void http_at_post_result_hook(int http_code, int cli_ret, const char *resp_buf,
						      uint32_t resp_len)
{
	(void)http_code;
	(void)cli_ret;
	(void)resp_buf;
	(void)resp_len;
}

static void http_post_output_result(httpclient_t *client, int cli_ret, httpclient_data_t *data)
{
	int code = client->response_code;
	const char *rb = data->response_buf;
	uint32_t rl = 0;

	if (rb != NULL && data->response_buf_len > 1)
		rl = (uint32_t)strnlen(rb, (size_t)data->response_buf_len - 1);

	http_at_post_result_hook(code, cli_ret, rb, rl);
}

static void http_post_job_finish(http_post_ctx_t *ctx)
{
	if (ctx == NULL)
		return;
	if (ctx->url)
		os_free(ctx->url);
	if (ctx->body)
		os_free(ctx->body);
	os_free(ctx);
	s_http_post_busy = 0;
}

void http_at_post_job_execute(void *job)
{
	http_post_ctx_t *ctx = (http_post_ctx_t *)job;
	httpclient_t client;
	httpclient_data_t data;
	char resp[HTTP_RESP_CONTENT_LEN];
	int ret = -1;

	if (ctx == NULL) {
		s_http_post_busy = 0;
		return;
	}

	os_memset(&client, 0, sizeof(client));
	os_memset(&data, 0, sizeof(data));
	os_memset(resp, 0, sizeof(resp));

	client.header = "Accept: application/x-www-form-urlencoded\r\n";
	data.post_buf = (char *)ctx->body;
	data.post_buf_len = ctx->body_len;
	data.post_content_type = ctx->content_type[0] ? ctx->content_type : "application/x-www-form-urlencoded";
	data.response_buf = resp;
	data.response_buf_len = sizeof(resp);
	data.response_content_len = sizeof(resp);

	{
		int default_port = 80;
		int use_port;
		const char *ca_crt = s_https_ca_crt_pem;

		if (ctx->url && strncmp(ctx->url, "https://", strlen("https://")) == 0) {
			default_port = 443;
		}

		use_port = ctx->port > 0 ? ctx->port : default_port;

		ret = httpclient_common(&client, ctx->url, use_port, ca_crt, HTTPCLIENT_POST,
				ctx->timeout_ms > 0 ? ctx->timeout_ms : 30000, &data);
	}

	http_post_output_result(&client, ret, &data);
	http_post_job_finish(ctx);
}

void http_at_post_job_abort(void *job)
{
	http_post_job_finish((http_post_ctx_t *)job);
}

uint8_t *http_at_post_job_body_ptr(void *job)
{
	http_post_ctx_t *ctx = (http_post_ctx_t *)job;

	return ctx ? ctx->body : NULL;
}

unsigned long http_at_post_job_body_len(void *job)
{
	http_post_ctx_t *ctx = (http_post_ctx_t *)job;

	return ctx ? (unsigned long)ctx->body_len : 0UL;
}

void *http_at_post_job_create(const char *url, unsigned long body_len, int port, unsigned long timeout_ms,
			      const char *content_type)
{
	http_post_ctx_t *ctx;
	char *url_copy;
	uint8_t *body;

	if (url == NULL || url[0] == '\0') {
		BK_LOGE(HTTP_AT_TAG, "HTTPCPOST url invalid\r\n");
		return NULL;
	}

	if (body_len == 0 || body_len > HTTP_POST_BODY_MAX) {
		BK_LOGE(HTTP_AT_TAG, "HTTPCPOST len invalid %lu (max %d)\r\n", body_len, HTTP_POST_BODY_MAX);
		return NULL;
	}

#if CONFIG_WIFI_ENABLE
	{
		wifi_link_status_t link = {0};

		if (bk_wifi_sta_get_link_status(&link) != BK_OK) {
			BK_LOGE(HTTP_AT_TAG, "HTTPCPOST: STA link status unavailable\r\n");
			return NULL;
		}
		if (link.state != WIFI_LINKSTATE_STA_GOT_IP) {
			BK_LOGE(HTTP_AT_TAG, "HTTPCPOST: STA has no IP (state=%d)\r\n", (int)link.state);
			return NULL;
		}
	}
#endif

	if (s_http_post_busy) {
		BK_LOGE(HTTP_AT_TAG, "HTTPCPOST busy\r\n");
		return NULL;
	}

	body = (uint8_t *)os_malloc((size_t)body_len);
	if (body == NULL) {
		BK_LOGE(HTTP_AT_TAG, "HTTPCPOST malloc body fail\r\n");
		return NULL;
	}

	ctx = (http_post_ctx_t *)os_malloc(sizeof(http_post_ctx_t));
	if (ctx == NULL) {
		os_free(body);
		return NULL;
	}

	url_copy = http_at_strdup_url_for_httpc(url);
	if (url_copy == NULL) {
		os_free(body);
		os_free(ctx);
		return NULL;
	}

	if (strcmp(url, url_copy) != 0)
		BK_LOGI(HTTP_AT_TAG, "HTTPCPOST URL normalized: %s\r\n", url_copy);

	os_memset(ctx, 0, sizeof(*ctx));
	ctx->url = url_copy;
	ctx->body = body;
	ctx->body_len = (uint32_t)body_len;
	ctx->port = port;
	ctx->timeout_ms = (uint32_t)(timeout_ms > 0 ? timeout_ms : 30000);
	if (content_type && content_type[0]) {
		strncpy(ctx->content_type, content_type, sizeof(ctx->content_type) - 1);
		ctx->content_type[sizeof(ctx->content_type) - 1] = '\0';
	}

	s_http_post_busy = 1;
	return ctx;
}

#else /* !CONFIG_HTTP */

void *http_at_post_job_create(const char *url, unsigned long body_len, int port, unsigned long timeout_ms,
			      const char *content_type)
{
	(void)url;
	(void)body_len;
	(void)port;
	(void)timeout_ms;
	(void)content_type;
	BK_LOGE(HTTP_AT_TAG, "HTTPCPOST needs CONFIG_HTTP\r\n");
	return NULL;
}

void http_at_post_job_execute(void *job)
{
	(void)job;
}

void http_at_post_job_abort(void *job)
{
	(void)job;
}

uint8_t *http_at_post_job_body_ptr(void *job)
{
	(void)job;
	return NULL;
}

unsigned long http_at_post_job_body_len(void *job)
{
	(void)job;
	return 0UL;
}

#endif /* CONFIG_HTTP */

#endif /* CONFIG_AT */

#if CONFIG_AT_CMD
void http_at_cli_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = AT_CMD_RSP_ERROR;
	const char *url = NULL;
	uint32_t sz = 0;

	if (argc == 2 && argv[1] != NULL) {
		url = argv[1];
	} else if (argc == 1 && argv[0] != NULL) {
		const char *prefix = "AT+HTTPGETSIZE=";

		if (strncmp(argv[0], prefix, strlen(prefix)) == 0 && argv[0][strlen(prefix)] != '\0') {
			url = argv[0] + strlen(prefix);
		}
	}

	if (url == NULL) {
		BK_LOGE(HTTP_AT_TAG, "Usage: AT+HTTPGETSIZE <url> or AT+HTTPGETSIZE=<url>\r\n");
		goto out;
	}

	if (httpclient_get_content_length_by_url(url, 30000, &sz) != 0) {
		goto out;
	}

	snprintf(pcWriteBuffer, (size_t)xWriteBufferLen, "%s:%lu\r\n%s", AT_CMDRSP_HEAD, (unsigned long)sz,
		 AT_CMD_RSP_SUCCEED);
	return;

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

#endif
