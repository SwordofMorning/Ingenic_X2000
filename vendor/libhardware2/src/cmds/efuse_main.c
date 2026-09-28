#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <libhardware2/efuse.h>

static char *command;
static void usage(int status)
{
    printf("Usage1:%s read_size <segment_name>\n", command);
    printf("Example:\n");
    printf("\t%s read_size CHIP_ID\n", command);
    printf("Usage2:%s read <segment_name>\n", command);
    printf("Example:\n");
    printf("\t%s read CHIP_ID\n", command);
    printf("Usage3:%s write <segment_name> <start> <size> <data...>\n", command);
    printf("Example:\n");
    printf("\t%s write CHIP_ID 0 2 0x10 0x11\n", command);
    printf("Usage4:%s print_segment_info\n", command);
    printf("Example:\n");
    printf("\t%s print_segment_info\n", command);
    printf("Usage5:%s [-h/--help]", command);
    printf("Example:\n");
    printf("\t%s --help\n", command);

    exit(status);
}

static int data_process(char *data)
{
    int ret;
    int temp;

    ret = sscanf(data, "%x", &temp);
    if(ret != 1)
        return -1;


    return temp;
}

int main(int argc, char **argv)
{
    int ret;
    int start;
    int size;
    int max_size;
    int i = 0, j;
    unsigned char *buf = NULL;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if ((strcmp(argv[1], "write") == 0)) {
        if (argc <= 5)
            usage(-1);

        ret = sscanf(argv[3], "%d", &start);
        if (ret != 1)
            usage(-1);

        ret = sscanf(argv[4], "%d", &size);
        if (ret != 1)
            usage(-1);

        max_size = efuse_read_seg_size(argv[2]);
        if (size > max_size)
            usage(-1);

        buf = malloc(size);
        for (i = 0; i < size; i++) {
            ret = data_process(argv[5+i]);
            if (ret < 0 || ret > 0xFF) {
                free(buf);
                usage(-1);
            }

            buf[i] = ret;
        }

        ret = efuse_write(argv[2], buf, start, size);
        if (ret < 0) {
            free(buf);
            usage(-1);
        }

        free(buf);
        return 0;
    }

    if ((strcmp(argv[1], "read_size") == 0)) {
        if (argc != 3)
            usage(-1);

        size = efuse_read_seg_size(argv[2]);
        if(size < 0)
            usage(-1);

        printf("%s size = %d\n", argv[2], size);
        return 0;
    }

    if ((strcmp(argv[1], "read") == 0)) {
        if (argc != 3)
            usage(-1);

        size = efuse_read_seg_size(argv[2]);
        if(size < 0)
            usage(-1);

        buf = malloc(size);
        start = 0;

        ret = efuse_read(argv[2], buf, start, size);
        if(ret < 0) {
            free(buf);
            return -1;
        }

        for (i = 0; i < size; i++) {
            printf("0x%02x ", buf[i]);
        }
        printf("\n");

        printf("bit(0) --> bit(16)\n");
        for (i = 0; i < size; i++) {
            for (j = 0; j < 8; j ++)
                printf("%d", !!((1 << j) & buf[i]));
            printf(" ");
            i ++;
            if (i < size)
                for (j = 0; j < 8; j ++)
                    printf("%d", !!((1 << j) & buf[i]));
            printf("\n");
        }

        free(buf);
        return 0;
    }

    if((strcmp(argv[1], "print_segment_info") == 0)) {
        if (argc != 2)
            usage(-1);

        struct efuse_segment_info *p;
        p = efuse_get_segment_information();
        if(p == NULL) {
            printf("Read segment information failed!\n");
            return -1;
        }

        while(p[i].segment_name != NULL){
            printf("name=%s, size=%d\n", p[i].segment_name, p[i].seg_size);
            i++;
        }

        efuse_free_information(p);

        return 0;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        usage((argc != 2) ? -1: 0);

    usage(-1);
    return -1;
}