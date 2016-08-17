#include <screen.h>
#include <stdio.h>

int recurse(int x){
	printf("%d\n", x);
	x = x--;
	if (x>0)
		recurse(x);
	else
		return x;

}

SCREEN(pow)
float pow_int(float base, int exponent)
{
    float result = 1.0f;

    for (int i = 0; i < 4; i++) {
        result *= base;
    }

    return recurse(2);
}
int main(int argc, char *argv[]){

	return pow_int(2, argc);
}
