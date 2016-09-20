extern void force_use_double(double x);
extern void force_use_int(int x);
extern int choice(void);
extern void __assumption_declared () __attribute__ ((__noreturn__));

void fill_from_both_ends(int n, double a, int x) {
  int i, j;
  if (n <= 0) __assumption_declared ();
  if (x < 0) __assumption_declared ();
  if (x >= n) __assumption_declared ();
  i=0;
  j=n-1;
  while(i <= j) {
    if (choice()) {
      if (x > i) {}
      else if (x < i) {}
      else a = 42;
      i++;
    } else {
      if (x > j) {}
      else if (x < j) {}
      else a = 42;
      j--;
    }
  }
  force_use_double(a);
  force_use_int(x);
  force_use_int(i);
  force_use_int(j);
}
