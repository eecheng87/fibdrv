#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
typedef enum { false, true } bool;

unsigned long long double_fib(int n, bool clz)
{
    unsigned long long a = 0, b = 1;
    int i = clz ? 31 - __builtin_clz(n) : 31;
    for (; i >= 0; i--) {
        unsigned long long t1, t2;
        t1 = a * (b * 2 - a);
        t2 = b * b + a * a;
        a = t1;
        b = t2;
        if ((n & (1 << i)) > 0) {
            t1 = a + b;
            a = b;
            b = t1;
        }
    }
    return a;
}

unsigned long long easy_fib(int n)
{
    unsigned long long series[92];
    series[0] = 0;
    series[1] = 1;
    for (int i = 2; i < n + 1; i++) {
        series[i] = series[i - 1] + series[i - 2];
    }
    return series[n];
}
long elapse(struct timespec start, struct timespec end)
{
    return ((long) 1.0e+9 * end.tv_sec + end.tv_nsec) -
           ((long) 1.0e+9 * start.tv_sec + start.tv_nsec);
}
int main()
{
    struct timespec t1, t2;

    for (int i = 2; i < 93; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        // printf("%llu ",easy_fib(i));
        (void) easy_fib(i);
        // printf("%llu ",double_fib(i, true));
        // printf("%llu\n",double_fib(i, false));
        clock_gettime(CLOCK_REALTIME, &t2);
        printf("%d ", i);
        printf("%ld ", elapse(t1, t2));

        clock_gettime(CLOCK_REALTIME, &t1);
        (void) double_fib(i, false);
        clock_gettime(CLOCK_REALTIME, &t2);
        printf("%ld ", elapse(t1, t2));

        clock_gettime(CLOCK_REALTIME, &t1);
        (void) double_fib(i, true);
        clock_gettime(CLOCK_REALTIME, &t2);
        printf("%ld\n", elapse(t1, t2));
    }
    return 0;
}