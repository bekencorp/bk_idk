#include <stdio.h>
#include "foo.h"

int c1(void)
{
	bk_foo();
	printf("hello component c1\n");
	return 0;
}
