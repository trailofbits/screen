#include "../../pagai_assert.h"
int input();
int main (){

  int x = -50;
  int y = input();
  while (x < 0){
     x = x + y;
     y++;
  }

  if (y < 0)
    goto ERROR;

  return 1;

ERROR: assert(0);


}
