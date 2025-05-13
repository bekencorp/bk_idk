#include <components/log.h>
#include "bk_private/bk_init.h"
#define TAG "example"

int main(void)
{
	bk_init();
	BK_LOGI(TAG, "Hello world!\n");
	return 0;
}
