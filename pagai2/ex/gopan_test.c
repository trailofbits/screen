
extern int f(int t);

int main() {
	int x0 = 0;
	int y0 = 0;
	int x = x0;
	int y = y0;

	while (1) {
		if (x <= 50) y++;
		else y--;

		if (y < 0) break;
		x++;
	}

	f(x0);
	f(y0);
	f(x);
	f(y);
}
