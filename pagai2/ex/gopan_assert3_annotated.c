#include <assert.h>


int main() {
	int x = 0;
	int y = 0;

	while (1) {
		/* invariant:
		Disjunct 0
		-1 * x + y = 0
		1 + -1 * x >= 0
		x >= 0
		Disjunct 1
		-1 * x + y = 0
		51 + -1 * x >= 0
		-2 + x >= 0
		Disjunct 2
		-102 + x + y = 0
		102 + -1 * x >= 0
		-52 + x >= 0
		*/
		if (x <= 50)  {
			y++;
		} else y--;
	

		if (y < 0) break;
		x++;
	}
	
	/* assert OK */
assert(x == 102);
}
