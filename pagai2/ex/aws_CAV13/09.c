#include "../../pagai_assert.h"

extern int unknown1();
extern int unknown2();
extern int unknown3();
extern int input();


/*
 * "fragtest_simple" from InvGen benchmark suite
 */


void main(){
  int i = input();
  int pvlen = input();
  int t = input();
  int k = 0;
  int n = input();
  int j = input();
  i = 0;

  //  pkt = pktq->tqh_first;
  while (unknown1())
    i = i + 1;
  if (i >= pvlen+1) {
    pvlen = i;
  } else {
    if(i > pvlen)
      return;
  }
  i = 0;

  while (unknown2()) {
    t = i;
    i = i + 1;
    k = k +1;
  }
  while (unknown3());

  j = 0;
  n = i;
  while (1) {
    assert(k > -1);
    k = k -1;
    i = i - 1;
    j = j + 1;
    if (j <= n-1) {
    } else {
      break;
    }
    }

}
