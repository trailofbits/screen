int func1() {
  __attribute__((annotate("screen_invariant_start"))) int a = 0;
  int b = 1;
  __attribute__((annotate("screen_invariant_end"))) int c = 3;
  __attribute__((annotate("screen_invariant_start"))) int res = a + b + c;
    return res;
}

int func2() {
  int a = 0;
  int b = 1;
  __attribute__((annotate("screen_invariant_end"))) int c = 2;
  return a + b + c;
}

int func3() {
  return func2();
}
