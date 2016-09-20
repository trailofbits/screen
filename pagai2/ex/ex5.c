int main(int argc, char ** argv) {
	int x = 0;
	int y;
	int i;
	
	while (x < 1000) {
		y = x+1;

		for (i=0; i < y; i++) {
			x++;
		}

		for (i=0; i < y; i++) {
			x = x + 2;
		}

	}
	return x;
}
