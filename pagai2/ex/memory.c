
int global;

int f() {
	int tab[5];

	global = 1;
	for (int i = 0; i < 10; i++) {
		tab[i] = tab[i-1] + global;
	}
	global = global+ 5;
	return 0;

}
