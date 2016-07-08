#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{

	(void)argc;
	(void)argv;

	int a = 0x41414141;
	int *b = &a;

	printf("a = %u *b = %u\n", a, *b);
}
