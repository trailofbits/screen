#include "../pagai_assert.h"

extern int undet();

void main() {
	int x = 0;
	int y = 0; 
	while(undet()) {
		if (undet()) {
			x++;
			y += 100;
		} else if (undet()) {
			if (x >= 4) {
				x++;
				y++;
			}
		}
	}
	if (x >= 4 && y <= 2)
		assert(0);
}
