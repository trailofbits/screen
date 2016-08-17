#include <screen.h>
#include <unistd.h>

int step_one(void);
int step_two(void);
int step_three(void);
int step_four_a(void);
int step_four_b(int);
int step_five(void);

int step_one(void)
{
    SCREEN_START(forked);
    return step_two();
}

int step_two(void)
{
    return step_three();
}

int step_three(void)
{
    pid_t self = getpid();

    if (self % 2 == 0) {
        return step_four_a();
    } else {
        return step_four_b(self + 20);
    }
}

int step_four_a(void)
{
    step_five();
    return 0;
}

int step_four_b(int argument)
{
    SCREEN_END(forked_0);
    pid_t self_again = getpid();

    for (int i = 0; i < self_again % 20; i++) {
        argument++;
    }

    step_five();

    return argument;
}

int step_five(void)
{
    SCREEN_END(forked);
    return 0;
}

int main(int argc, char *argv[])
{
    int value;

    SCREEN_START(forked_0);
    value = step_one();
    return value;

}

