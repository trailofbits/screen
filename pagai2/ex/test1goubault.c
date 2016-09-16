
    int main() {
    int i;
    int j;
      i = 1;
      j = 10; 
      while (i <= j){ 
	i = i + 2;
	j = j - 1; 
      }
	return i;
    }

    /*
    M0 = [| 0 0 0  ; 0 0 0  ; 0 0 0 |]
    M1 = Guard(1, 0, -1)(Guard(0, 1, 1)(Forget(1)(M0)))
    M2 = Guard(2, 0, -10)(Guard(0, 2, 10)(Forget(2)(M1)))
    M3 = (NF(Guard(2, 1, 0)(M2))) U (NF(Guard(2, 1, 0)(M5)))
    M4 = Update(1, 2)(M3)
    M5 = Update(2, -1)(M4)
    M6 = (NF(Guard(1, 2, 0)(M2))) U (NF(Guard(1, 2, 0)(M5)))
    */



    
