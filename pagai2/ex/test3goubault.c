
int main () {
int i ;
int j ;
  i = 150;
  j = 175;
  while ( 100 <= j && j <= 300){ 
    i++; 
    if( j <= i ){ 
      j = j - 2;
      i = i - 1; 
    } 
  }
  return i;
}
