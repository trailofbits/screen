

int f(int x, int y) {
	int res;
	int b = (x || f(y,5));
	b++;
	
	if (b) {
		x ++;
		res = 2;
	} else {
		res = 4;
	}
	return res;
}
