#include <assert.h>
#include <limits.h>

int foo(int n)
{
    // avoid arithmetic overflows
    if (n < INT_MIN / 6 || n >= (INT_MAX - 15) / 6)
        return -1;
    
    // compute s = n + (n+1) + ... + (n+5)  [ = 6n + 15]
    int i, s = 0;
    for (i = n; i < n+6; i++) {
        s += i;
    }
    
    // show that s is a multiple of 3
    assert (s % 3 == 0);

    return s;
}