#include <stdio.h>
#include <stdint.h>
#include <screen.h>

int
main(int argc, char *argv[])
{
    FILE *f;
    uint32_t header32;
    int status;

    SCREEN_START(main);

    f = fopen(argv[0], "rb");
    if (f == NULL) {
        fprintf(stderr, "Could not open: %s\n", argv[0]);
        return 1;
    }

    status = fread(&header32, sizeof(header32), 1, f);
    if (status != 1) {
        fprintf(stderr, "Could not read from: %s\n", argv[0]);
        return 2;
    }

    if (header32 == 0x7f454c46) {
        printf("'%s' is an elf binary.\n", argv[0]);
    } else if (header32 == 0xfeedfacf) {
        printf("'%s' is a mach-o binary.\n", argv[0]);
    } else {
        printf("'%s' is not an elf binary.\n", argv[0]);
    }

    SCREEN_END(main);

    return 0;
}
