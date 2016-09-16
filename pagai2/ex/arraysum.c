
double sum(unsigned n) {
  double tab[10];
  double s = 0.0;
  for(unsigned i=0; i<n; i++) {
    s += tab[i];
  }

  for(unsigned i=0; i<5; i++) {
    s += tab[i];
  }
  return s;
}

