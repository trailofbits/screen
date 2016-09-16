

int main() {
	int x;
	int d;
	x = 0;
	d = 1;

	while(1) {
		/* invariant:
		1 + -1 * d >= 0
		x >= 0
		1999 + d + -2 * x >= 0
		1 + d >= 0
		*/
		if (x > 0) {
		} else if (x < 0) {
		} else {
			d=1;
		}
		if (x > 1000) {
		} else if (x < 1000) {
		} else {
			d=-1;
		}
		x +=d;
	}
	return 1;
/* UNREACHABLE */}
