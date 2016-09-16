#include "../../pagai_assert.h"
int unknown1();
int unknown2();
int unknown3();
int unknown4();

void main()
{
 int n = unknown1();
 int k=1;
 int i=1;
 int j=0;
 while(i<=n-1) {
  j=0;
  while(j<=i-1) {
      k += (i-j);
      j++;
  }
  if(j<i)
    return;
  i++;
 }
 if(i < n)
   return;
 assert(k > n-1);
 
}
