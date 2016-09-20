


int main() {
	int x = 0;
	int y = 0;

	while (1) {
		/* invariant:
		102 + -1 * x + -1 * y >= 0
		y >= 0
		x + -1 * y >= 0
		*/
		if (x <= 50) y++;
		else y--;

		if (y < 0) break;
		x++;
	}
}
