#include <pagai_assert.h>

int f(int x, int k) {
	assume(k < 100000);
	assume(k > -100000);
	assume(x < 100000);
	assume(x > -100000);
	k += x;
	return k;
}
