extern int read_int();
extern void print_int(int);

int main(void) {
  int n = read_int();
  if (n < 1 || n > 100000000) n=1;
  for(int i=0; i<n; i++) {
    for(int j=0; j<i; j++) {
      print_int(i);
      print_int(j); 
    }
  }
  return 0;
}
