
int global;

int f (double *x, int * k) {

	double t[10];
	int * p = &global+10;

	for (int i = 0; i < 10; i++) {
		t[i] = t[i] + *x;
	}
	global++;
	(*p)++;
	return global;
}
