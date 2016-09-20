int toto(int n) {
  if (n >= 10000 || n < 0) n = 0;
  int sum = 0;
  for(int i=0; i<n; i++) {
    sum += i;
  }
  return sum;
}
