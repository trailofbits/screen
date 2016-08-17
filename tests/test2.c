#include <stdio.h>

/*int func_1(int arg);
int func_2(int arg);

int func_1(int arg)
{   if (arg < 1) return arg;
    else { printf("func_1\n"); return func_2(arg >> 1); } }
int func_2(int arg)
{   if (arg < 1) return arg;
    else { printf("func_2\n"); return func_1(arg >> 1); } }
*/
int
main(int argc, char *argv[])
{   
	
int a = 10;
int b = 4;
if (argc > 1)
	a = 8;

b = a >> 1;
printf("%d\n", b);
//func_1(10);
    return 0 ; }
