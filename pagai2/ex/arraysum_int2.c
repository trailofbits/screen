
int sum(int n, int tab[n]) {
	if (n > 0) {
		int s = 0;
		for(int i=0; i<n+10; i++) {
			s += tab[i];
		}
		return s;
	}
	return 0;
}
