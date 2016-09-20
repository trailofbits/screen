extern void use_double(double x);
extern void use_int(int x);
extern void __assumption_declared () __attribute__ ((__noreturn__));

void fill_array_2d(int m, int n, int a, int x, int y) {
  a = 0;
  if (x < 0 || x>=m || y<0 || y>=n)  __assumption_declared ();
  for(int i=0; i<m; i++) {
    for(int j=0; j<n; j++) {
      if (x<i) {}
      else if (x>i) {}
      else {
	if (y<j) {}
	else if (y>j) {}
	else a = 451;
      }
    }
  }
  use_int(x);
  use_int(y);
  use_int(m);
  use_int(n);
  use_double(a);
}
