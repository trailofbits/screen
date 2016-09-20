/* MDH WCET BENCHMARK SUITE. File version $Id: fac.c,v 1.2 2006/05/19 07:50:27 csg Exp $ */
/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */

int fac (int n)
{
  if (n == 0)
     return 1;
  else
     return (n * fac (n-1));
}

int main (void)
{
  int i;
  int s = 0;
  volatile int n;

  n = 5;
  for (i = 0;  i <= n; i++)
      s += fac (i);

  return (s);
}

