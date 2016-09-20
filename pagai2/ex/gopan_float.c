


int main() {
	double x = 0.0;
	double y = 0.0;

	while (1) {
		if (x <= 50.0) y = y + 1.0;
		else y = y - 1.0;

		if (y < 0.0) break;
		x = x + 1.0;
	}
}
