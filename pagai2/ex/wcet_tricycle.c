void toto() {
  int count=0;
  for(int i=0; i<10000; i++) {
    switch (i % 3) {
    case 0:
      count += 2;
      break;
    case 1:
      count += 1;
      break;
    case 2:
      count += 3;
      break;
    }
  }
}
