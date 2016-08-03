#include <screen.h>

int step_one(void);
int step_two(void);
int step_three(void);
int step_four(void);

int step_one(void)
{
    SCREEN_START(steps);
    return step_two();
}

int step_two(void)
{
    return step_three();
}

int step_three(void)
{
    return step_four();
}

int step_four(void)
{
    SCREEN_END(steps);
    return 0;
}

int main(int argc, char *argv[])
{
    int value;

    value = step_one();
    return value;

}
