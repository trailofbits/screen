#include "../../pagai_assert.h"
int unknown1();
int unknown2();
int unknown3();
int unknown4();
int input();

/*
 * From InvGen test suite
 */

void main() {
  int i = input();
  int j = input();
  int k = input();
  int n = input();
  
  for (i=0;i<n;i++)
    for (j=i;j<n;j++)
      for (k=j;k<n;k++){
	if(k <= i-1)
	{assert(0);}
      }
}
