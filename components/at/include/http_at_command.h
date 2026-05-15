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
#pragma once

#include <sdkconfig.h>
#include <stdint.h>

#if (CONFIG_AT_CMD || CONFIG_AT) && CONFIG_OTA_HTTP

#if CONFIG_AT_CMD
void http_at_cli_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#if CONFIG_AT
/* Opaque job: payload buffer is filled by caller (e.g. AT UART) before execute. */
void *http_at_post_job_create(const char *url, unsigned long body_len, int port, unsigned long timeout_ms,
			      const char *content_type);
uint8_t *http_at_post_job_body_ptr(void *job);
unsigned long http_at_post_job_body_len(void *job);
void http_at_post_job_execute(void *job);
void http_at_post_job_abort(void *job);

/* Set HTTPS CA certificate (PEM string). Used by httpclient_common() when URL is https://. */
#if CONFIG_HTTP
int http_at_set_https_ca_crt(const uint8_t *pem, unsigned long pem_len);
const char *http_at_get_https_ca_crt(void);
#endif
#endif

#endif
