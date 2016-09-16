
extern int wait();
int main() {
	int x;
	int d;
	x = 0;
	d = 1;

	while(1) {
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
		while (wait()) {}
		x +=d;
	}
	return 1;
}
