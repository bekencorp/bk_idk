#include "bk_private/bk_init.h"
#include "c1.h"

extern int c1(void);
extern int c2(void);

int main(void)
{
	bk_init();
	c1();
	//c2();
	return 0;
}
