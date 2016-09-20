

int main() {
	int x;
	int d1,d2;
	int * p;
	x = 0;
	d1 = 1;
	d2 = -1;
	p = &d1;	

	while(1) {
		if (x > 0) {
		} else if (x < 0) {
		} else {
			p =&d1;
		}
		if (x > 1000) {
		} else if (x < 1000) {
		} else {
			p =&d2;
		}
		x +=*p;
	}
	return 1;
}
