


int global = 10;
int alsoglobal = 20;


int f() {
	int x = 0;
	global++;

	while (x < 42) x++;

	return global;
}

int g() {
	int x = 0;
	while (global < 100) {
	global+=f();
	}

	return x+global+alsoglobal;
}
