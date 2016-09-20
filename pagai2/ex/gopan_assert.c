#include <pagai_assert.h>


int main() {
	int x = 0;
	int y = 0;

	while (1) {
		if (x <= 50)  {
			y++;
		} else y--;
	

		if (y < 0) break;
		x++;
	}
 	
	assert(x+y<=101);
	assert(x <= 102);
}
