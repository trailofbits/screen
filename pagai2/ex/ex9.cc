
#include<iostream>

int main() {
	int x = 0;
	int i = 0;
	int r = 0;

	while (i < 100) {
		if (i < 50) {
			x = -1;
		} else {
			x = 1;
		}

		if (x > 0) {
		} else if (x < 0) {
		} else {
			r++;
		}	
		i++;
	}

	return r;
}
