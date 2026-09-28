#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libutils2/boot_time.h>

void usage(int status)
{
    fprintf(stderr, "usage: of show_boot_time\n");
    fprintf(stderr, "-d : this is default option\n");
    fprintf(stderr, "     show boot time in digital num, in usec\n");
    fprintf(stderr, "-f : show boot time in float num, in sec\n");

    exit(status);
}

int main(int argc, char *argv[])
{
    int in_usec;

    if (argc > 2)
        usage(-1);
    if (argc == 1)
        in_usec = 1;
    else if (!strcmp(argv[1], "-d"))
        in_usec = 1;
    else if (!strcmp(argv[1], "-f"))
        in_usec = 0;
    else if (!strcmp(argv[1], "-h"))
        usage(0);
    else if (!strcmp(argv[1], "--help"))
        usage(0);
    else
        usage(-1);

    if (in_usec)
        printf("%llu\n", (unsigned long long) boot_time_usecs());
    else
        printf("%f\n", boot_time_secs());

    return 0;
}
