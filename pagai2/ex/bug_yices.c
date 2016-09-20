
extern double input();

void yices_bug() {
  double x_old = 0.0;
  while (1) {
    double x = input();
	if ( x <= x_old)
		x_old = x;
  }
}

