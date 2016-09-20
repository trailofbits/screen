extern void force_use_int(int x);
extern int choice(void);
extern void __assumption_declared () __attribute__ ((__noreturn__));

void fill_from_both_ends(int n, double *a) {
  int i, j;
  if (n <= 0) __assumption_declared ();
  i=0;
  j=n-1;
  while(i <= j) {
    if (choice()) {
      a[i++] = 42;
    } else {
      a[j--] = 42;
    }
  }
  force_use_int(i);
  force_use_int(n);
  force_use_int(j);
}
