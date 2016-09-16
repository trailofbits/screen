/* $*************** KCG Version 6.1.3 (build i6) ****************
** Command: s2c -target C -static -global_root_context -O 3 -expall -node ex_sum_up test2.scade
** Generation date: 2013-07-29T10:12:50
*************************************************************$ */
extern int fonct();



/* ex_sum_up */
void ex_sum_up(void){
 int cptr_ex_sum_up9=0; int cptr_ex_sum_up8=0; int cptr_ex_sum_up7=0; int cptr_ex_sum_up6=0; int cptr_ex_sum_up5=0; int cptr_ex_sum_up4=0; int cptr_ex_sum_up3=0; int cptr_ex_sum_up2=0; int cptr_ex_sum_up1=0;
 cptr_ex_sum_up1++; 
  int tmp;
  int i;
  int noname,s1;
  int max_i=fonct(),s2,f;
  
  s1 = 0;
  for (i = 0; i < 10; i++) {
 cptr_ex_sum_up2++; 
    s1 = 1;
    noname = i + 1;
    if (!(i < max_i)) {
 cptr_ex_sum_up3++; 
      break;
    }
  }
  s2 = s1;
  if (noname < 6) {
 cptr_ex_sum_up4++; 
    for (i = 0; i < 10; i++) {
 cptr_ex_sum_up5++; 
      tmp = s2;
      s2 = tmp + 2;
      if (!(tmp < 4)) {
 cptr_ex_sum_up6++; 
        break;
      }
    }
  }
  f = 0;
  if (2 * s1 > s2) {
 cptr_ex_sum_up7++; 
    for (i = 0; i < 10; i++) {
 cptr_ex_sum_up8++; 
      tmp = f;
      f = tmp + 2;
      if (!(tmp < 4)) {
 cptr_ex_sum_up9++; 
        break;
      }
    }
  }
printf(" THE counters, main: %d, %d, %d, %d, %d, %d,  %d, %d, %d,\n ",cptr_ex_sum_up1,cptr_ex_sum_up2,cptr_ex_sum_up3,cptr_ex_sum_up4,cptr_ex_sum_up5,cptr_ex_sum_up6, cptr_ex_sum_up7,cptr_ex_sum_up8,cptr_ex_sum_up9);
}

/* $*************** KCG Version 6.1.3 (build i6) ****************
** ex_sum_up.c
** Generation date: 2013-07-29T10:12:50
*************************************************************$ */

