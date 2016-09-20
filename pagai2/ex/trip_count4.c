
void print(int x) {
	int y = x+1;
}

void trip_counts() {
  int a=0, b=0, c=0, d=0, e=0, i;
  for(i=0; i<100; i++) {
//    if (4*b > a) { // essai pour forcer 4b<=a
//      while(1) {}
//    }
    a++;
    if (i%4 == 0) b++;
    else c++;
    if (i%4 == 0) d++;
    if (i%2 == 0) e++;
  }
  print(i);
  print(a);
  print(b);
  print(c);
  print(d);
  print(e);
}

void f(int x) {
	int y = x+1;
}
