extern void force_use_double(double x);
extern void force_use_int(int x);

void fill_with_index_scalar(int n, double a, int x) {
  if (n <= 0) {
    while(1) {}
  }
  if (x < 0) {
    while(1) {}
  }
  if (x >= n) {
    while(1) {}
  }
  for(int i=0; i<n; i++) {
    if (x < i) { }
    else if (x > i) { }
    else a = i;
  }
  force_use_double(a);
  force_use_int(x);
}
