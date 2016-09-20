

void f() {
	int k = 0;
	int i = 0;
	int j = 0;
	while (k < 100) {
		i = 0;
		j = k;
		while (i < j) {
			i++;
			j--;
		}
		k++;
	}
}
