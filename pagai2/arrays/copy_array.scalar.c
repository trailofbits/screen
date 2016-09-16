extern int choice();
extern void use(double x);

extern void __assumption_declared () __attribute__ ((__noreturn__));

void copy_array() {
  int a = choice(), b = choice();
  int x = choice(), y = choice();
  int n = choice();
  if (n <= 0) __assumption_declared ();
  if (x < 0) __assumption_declared ();
  if (x >= n) __assumption_declared ();
  if (y < 0) __assumption_declared ();
  if (y >= n) __assumption_declared ();
  if (x < y) {} else if (x > y) {} else {}
  for(int i=0; i<n; i++) {
    int tmp=choice();
    if (x > i) {
    } else if (x < i) {
    } else {
      tmp = a; 
    }
    if (y > i) {
    } else if (y < i) {
    } else {
      b = tmp; 
    }
  }
  use(x);
  use(y);
  use(a);
  use(b);
  use(n);
}
