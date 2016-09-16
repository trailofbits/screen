
int f(int x) {
	return x+42;
}

int main(int argc, char ** argv) {
	int x = 100;
	int y;
	int i;
	
	while (x) {
	y = f(x);
	y = y + 6;
	y = x + y + 5;

	for (i=0; i < y; i++) {
		x = x + 2;
	}

	}
	return x;
}
