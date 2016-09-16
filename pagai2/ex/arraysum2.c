double sum(unsigned n, double tab[n]) {
	double s = 0.0;
	if (n > 0) {
		unsigned i;
		for(i=0; i<n; i++) {
			s += tab[i];
		}
	}
	return s;
}

