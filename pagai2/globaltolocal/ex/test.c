


int global = 10;
static int alsoglobal;
float fl;


int f() {
	int x = 0;

	for (int i = 0; i < global; i++) {
		i++;
		alsoglobal++;
	}
	fl += 1.;

	return alsoglobal;
}

