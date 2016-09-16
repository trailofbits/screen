

int main(int argc, char ** argv) {
	int x;
	int y;
	int i;

	x = 2;
	y = x + 50;

	for (i=0; i < y; i++) {
		x = x + 2;
	}

	return x;
}
