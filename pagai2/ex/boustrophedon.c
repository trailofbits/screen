

int main() {
	int x = 0;
	int d = 1;
	while(1) {
		if (x == 0) d=1;
		if (x == 1000) d=-1;
		x +=d;
	}
	return 1;
}
