int main(int argc, char ** argv) {
	int x;
	int y;
	int i;
	
	while (argc > 100000) {
	x = 2;
	y = x + 50;
	y = y + 6;
	y = x + y + 5;



	for (i=0; i < y; i++) {
		x = x + 2;
		x = x++;
		x = x - 2;
	}

	x = x+1;

	for (i=0; i < y; i++) {
		x = x + 2;
	}

	}

	x++;
	return x;
}
