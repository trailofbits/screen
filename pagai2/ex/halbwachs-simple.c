

int f() {
	int i = 0;
	int j = 0;

	while (i < 100) {
		j = 0;
		while (j < 100) {
			j++;
		}
		i++;
	}
	return i+j;
}
