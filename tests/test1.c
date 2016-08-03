// had it working to get a simple flattening list, but clearly not branch specific
//main() -> fun1() -> llvm.var.annotation() -> printf() -> llvm.var.annotation() -> printf() -> foo() -> printf() -> printf()

#include <stdio.h>
#include <stdlib.h>

__attribute__((annotate("screen_function_paths"))) 
void foo(int a) { 
	printf("%d", a);	
	return;
}

int fun1();
int fun2();
int fun3();
int fun4();
		
int fun1(){
	//__attribute__((annotate("screen_paths_start"))) char a = 'a';
	//printf("starting character: %c", a);
	//__attribute__((annotate("screen_paths_end"))) char b = 'b';
	//printf("Ending character: %c", b);
	int a = 2;
	int b = 3;
	char c = a + b;
        fun2();
	return (int) c;
}

int fun2(){
	printf("starting character:");
        fun3();
        return 0;
}

int fun3() {
  fun4();
  return 0;
}

int fun4() {
	__attribute__((annotate("screen_paths2_end"))) char b = 'b';
        (void) b;
        return 0;
}

int fun0(){

	fun2();
	return 0;
}

__attribute__((annotate("screen_function_paths"))) 
int main(int argc, char **argv)
{

	__attribute__((annotate("screen_paths2_start"))) char aa = 'a';
	(void) aa;
	(void)argc;
	(void)argv;

	int a = 0x41414141;
	int *b = &a;
	printf("BranchInst approaching...\n");
	if ( argc != 1)
		a = fun1();
	else
		fun0(a);

	printf("a = %u *b = %u\n", a, *b);
}
