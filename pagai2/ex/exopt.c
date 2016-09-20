
int f(int i) {
	int r;
//	if (i == 0)
//		r = 0;
//	else 
//		r = f(r-1)+1;
	return r;
}

int main() {
	int i = 0;
	int d = 0;

	while (1) {
		if (i < 0) {
		} else if (i > 0) {
		} else {
			d=1;
		}

		if (i < 1000) {
		} else if (i > 1000) {
		} else {
			d=-1;
		}

		while (d == 0 || i > 2000) {
			d = f(i);
			i = f(d);
		}
		
		i += d;
	}
}
