
extern double input();
void rate_limiter() {
  double x_old;
  x_old = 0;
  while (1) {
    double x = input();
    if (x <= -1000 || x >= 1000) x=0;
    if (x >= x_old+1)
        x = x_old+1;
    if (x <= x_old-1)
        x = x_old-1;
    x_old = x;
  }
}

