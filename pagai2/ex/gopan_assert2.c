#include <assert.h>
#include <vector.h>


int main() {
	int x = 0;
	int y = 0;
	std::vector<int> map;

	while (1) {
		if (x <= 50)  {
			y++;
		} else y--;
	

		if (y < 0) break;
		x++;
	}
	
	assert(x <= 100);
}
