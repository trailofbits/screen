extern void print(int i);

void trip_counts() {
  int a=0, i;
  for(i=0; i<100; i++) {
    a++;
  }
  print(i);
  print(a);
}
