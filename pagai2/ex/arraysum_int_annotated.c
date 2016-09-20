
int sum(unsigned n, int tab[n]) {
	if (n > 0) {
		int s = 0;
		for(unsigned i=0/* invariant:
		                i >= 0
		                n + -1 * i >= 0
		                -1 + n >= 0
		                */
		                ; i<n; i++) {
			/* UNDEFINED BEHAVIOUR
			+= : Signed Addition Overflow */
			s += tab[i];
		}
		return s;
	}
	return 0;
}
