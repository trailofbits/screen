void toto() {
  int count=0, phase=0;
  for(int i=0; i<10000; i++) {
    switch (phase) {
    case 0:
      count += 3;
      phase = 1;
      break;
    case 1:
      count += 2;
      phase = 2;
      break;
    default:
      count += 1;
      phase = 0;
      break;
    }
  }
}
