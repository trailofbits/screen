#include "../../pagai_assert.h"
int input();
void main () {
int i = input();
int j = input();
int from = input();
int to = input();
int k = input();

if (!(k >= 0)) return;
if (!(k <= 100)) return;

if (!(from >= 0)) return;
if (!(from <= k)) return;

i = from;
j = 0;

while (i < k) {
i++;
j++;
}

if (j >= 101)
  goto ERROR;

  return;

ERROR: assert(0);

}

