

int f(int x, int y) {
	y = 0;

	while (1) {
		if (x <= 50) y++;
		else y--;

		if (y < 0) break;
		//x++;
	}
}
