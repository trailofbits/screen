
int main() {
int i ; 
int j ;
  i = 150 ;
  j = 175;
  
  while (j >= 100){ 
    i++; 
    if( j - i <= 0 ){ 
      j = j - 2;
      i = i - 1; 
    } 
  }
  return i;
}
