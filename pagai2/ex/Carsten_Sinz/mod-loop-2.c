#include <assert.h>
#include <limits.h>

int foo(int n, int m)
{
    if (m <= 0 || m >= INT_MAX/m || m % 2 == 0 || n > INT_MAX - m )
        return -1;
        
    int i, s = 0;
    for (i = n; i < n+m; i++) {
        s += i % m;
    }
    
    assert (s % m == 0);

    return s;
}