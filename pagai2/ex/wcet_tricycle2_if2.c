void toto() {
  int count=0, phase=0;
  for(int i=0; i<1000; i++) {
    if (phase == 0) { count += 3; phase = 1; }
    else if (phase == 1) { count += 2; phase = 2; }
    else if (phase == 2) { count += 1; phase = 0; }
  }
}
