#include "../../pagai_assert.h"
int unknown1();
int unknown2();
int unknown3();
int unknown4();
int input();

void main()
{
  int x = input(); 
  int y = input(); 
  int k = input(); 
  int j = input();
  int i = input(); 
  int n = input();
    int m = 0;
    if((x+y) != k)
      return;
    j = 0;
    while(j<=n-1) {
      if(j==i)
      {
         x++;
         y--;
      }else
      {
         y++;
         x--;
      }
	if(unknown1())
  		m = j;
      j++;
    }
    if(j < n)
      return;
    if(x + y <= k - 1 || x + y >= k + 1 || (n >= 1 && ((m <= -1) || (m >= n))))
    {assert(0);}
}

