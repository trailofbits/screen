extern int f(int c);

double fabs(double x) {
	double res;
	int c = 0;
	c += 6;
	if (x >= 0) {
		res = x;
		c += 3;
	} else {
		res = -x;
		c += 4;
	}
	c += 2;
	f(c);
	return res;
}
