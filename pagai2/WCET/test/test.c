extern int undet();
int test() {
	int x = undet();
	int count = x % 4;
	if (count == 1) {
		x--;
		x--;
		x--;
		x--;
		x--;
	} else {
	}
	if (count == 2) {
		x--;
		x--;
		x--;
		x--;
		x--;
	} else {
	}
	return 0;
}
