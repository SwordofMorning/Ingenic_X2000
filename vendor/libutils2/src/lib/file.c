#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int64_t file_get_size(const char *file)
{
    struct stat s;
    if (stat(file, &s) < 0)
        return -1;
    return s.st_size;
}

int file_read_data(const char *input_file, int64_t offset, void *data, int64_t size)
{
    FILE *file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "file_utils: failed to open file: %s (%s)\n",
            input_file, strerror(errno));
        return -1;
    }

    if (offset) {
        if (fseek(file, offset, SEEK_SET) < 0) {
            fprintf(stderr, "file_utils: failed to seek file: %s (%s)\n",
                input_file, strerror(errno));
            fclose(file);
            return -1;
        }
    }

    int64_t n = 0;
    while (size) {
        int N = 1024*1024;
        int ret = fread(data, 1, size > N ? N : size, file);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to read file: %s (%s) %d\n",
                input_file, strerror(errno), ret);
            fclose(file);
        }
        if (ret == 0)
            break;

        size -= ret;
        data += ret;
        n += ret;
    }

    fclose(file);
    return n;
}

int file_write_data(const char *out_file, void *data, unsigned long data_size)
{
    int ret = 0;
    FILE *file = fopen(out_file, "w");
    if (!file) {
        fprintf(stderr, "file_utils: failed to open file: %s (%s)\n",
            out_file, strerror(errno));
        return -1;
    }

    int N = 1024*1024;

    while (data_size) {
        int size = data_size < N ? data_size : N;
        ret = fwrite(data, 1, size, file);
        if (ret < 0) {
            fprintf(stderr, "file_utils: failed to write file: %s (%s) %d\n",
                out_file, strerror(errno), size);
            goto close_file;
        }

        data_size -= ret;
        data += ret;
    }

    ret = 0;
    fflush(file);

close_file:
    fclose(file);

    return ret;
}
