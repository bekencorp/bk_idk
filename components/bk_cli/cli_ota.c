#include "cli.h"
#include "modules/ota.h"
#if CONFIG_SECURITY_OTA
#include "_ota.h"
#endif

#if CONFIG_OTA_TFTP
extern void tftp_start(void);
extern void string_to_ip(char *s);
static void tftp_ota_get_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	short len = 0, i;
	extern char     BootFile[] ;

	if (argc > 3) {
		os_printf("ota server_ip ota_file\r\n");
		return;
	}

	os_printf("%s\r\n", argv[1]);

	os_strcpy(BootFile, argv[2]);
	os_printf("%s\r\n", BootFile);
	string_to_ip(argv[1]);


	tftp_start();

	return;

}
#endif

#if CONFIG_HTTP_AB_PARTITION
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	exec_flag ret_partition = 0;
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	ret_partition = bk_ota_get_current_partition();
	if(ret_partition == 0x0)
	{
    	os_printf("partition A\r\n");
    }
	else
	{
    	os_printf("partition B\r\n");
    }
#else
	ret_partition = bk_ota_get_current_partition();
	if((ret_partition == 0xFF) ||(ret_partition == EXEX_A_PART))
	{
    	os_printf("partition A\r\n");
    }
	else
	{
    	os_printf("partition B\r\n");
    }
#endif
}
#endif

#if CONFIG_DIRECT_XIP
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern uint32_t flash_get_excute_enable();
	uint32_t id = flash_get_excute_enable();
	if(id == 0){
		os_printf("partition A\r\n");
	} else if (id == 1){
		os_printf("partition B\r\n");
	}
}
#endif

#if CONFIG_OTA_HTTP
extern int bk_http_ota_download(const char *uri);
void http_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;
	if (argc != 2)
		goto HTTP_CMD_ERR;

	/* Check for help option */
	if (os_strcmp(argv[1], "--help") == 0 || os_strcmp(argv[1], "-h") == 0 || os_strcmp(argv[1], "help") == 0) {
		os_printf("Usage: http_ota <url>\r\n");
		os_printf("  url - HTTP URL for OTA download\r\n");
		return;
	}

	ret = bk_http_ota_download(argv[1]);

	if (0 != ret)
		os_printf("http_ota download failed.");

	return;

HTTP_CMD_ERR:
	os_printf("Usage: http_ota <url>\r\n");
	os_printf("  url - HTTP URL for OTA download\r\n");
}
#endif

#if CONFIG_OTA_HTTPS && !CONFIG_OTA_TEST
int bk_https_ota_download(const char *url);
void bk_https_start_download(beken_thread_arg_t arg) {
	char *url = (char *)arg;
	int ret;

	if (!url) {
		os_printf("%s: url is NULL\r\n", __func__);
		rtos_delete_thread(NULL);
		return;
	}

	os_printf("%s: starting download from %s\r\n", __func__, url);
	os_printf("%s: About to call bk_https_ota_download, url=%p\r\n", __func__, url);
	ret = bk_https_ota_download(url);
	os_printf("%s: bk_https_ota_download returned, ret=%d\r\n", __func__, ret);
	if(ret != BK_OK) {
		os_printf("%s download fail, ret:%d\r\n", __func__, ret);
	}

	/* Free the URL string allocated in https_ota_Command */
	os_free(url);
	url = NULL;

	rtos_delete_thread(NULL);
}

void https_ota_start(const char *url)
{
	UINT32 ret;

	if (!url) {
		os_printf("https_ota_start: url is NULL\r\n");
		return;
	}

	os_printf("https_ota_start\r\n");
	ret = rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
							 "https_ota",
							 (beken_thread_function_t)bk_https_start_download,
							 5120,
							 (beken_thread_arg_t)url);

	if (kNoErr != ret) {
		os_printf("https_ota_start failed, ret:%d\r\n", ret);
		os_free((void *)url);
	}
}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *url_copy = NULL;

	if (argc != 2)
		goto HTTP_CMD_ERR;

	/* Check for help option */
	if (os_strcmp(argv[1], "--help") == 0 || os_strcmp(argv[1], "-h") == 0 || os_strcmp(argv[1], "help") == 0) {
		os_printf("Usage: https_ota <url>\r\n");
		os_printf("  url - HTTPS URL for OTA download\r\n");
		return;
	}

	/* Allocate memory and copy the URL string for thread safety */
	url_copy = (char *)os_malloc(os_strlen(argv[1]) + 1);
	if (!url_copy) {
		os_printf("https_ota: failed to allocate memory for URL\r\n");
		return;
	}
	os_strcpy(url_copy, argv[1]);

	https_ota_start(url_copy);

	return;

HTTP_CMD_ERR:
	os_printf("Usage: https_ota <url>\r\n");
	os_printf("  url - HTTPS URL for OTA download\r\n");
}
#endif

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
static const struct cli_command s_ota_commands[] = {

#if CONFIG_OTA_TFTP
	{"tftpota", "tftpota [ip] [file]", tftp_ota_get_Command},
#endif

#if CONFIG_OTA_HTTP
	{"http_ota", "http_ota url", http_ota_Command},
#endif

#if CONFIG_OTA_HTTPS && !CONFIG_OTA_TEST
	{"https_ota", "https_ota <url>", https_ota_Command},
	
#endif

#if CONFIG_HTTP_AB_PARTITION
	{"ab_version", NULL, get_http_ab_version},
#endif

#if CONFIG_DIRECT_XIP
	{"ab_version", NULL, get_http_ab_version},
#endif
};

#if (CONFIG_TFM_FWU)
extern int32_t ns_interface_lock_init(void);
#endif
int cli_ota_init(void)
{
#if (CONFIG_TFM_FWU)
	ns_interface_lock_init();
#endif
	return cli_register_commands(s_ota_commands, OTA_CMD_CNT);
}
