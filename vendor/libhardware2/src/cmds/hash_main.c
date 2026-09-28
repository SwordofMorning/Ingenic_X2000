#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libhardware2/hash.h>
#define MAXBUFSIZE 100


static void usage(char *command) {
    printf("Usage1: \t%s <mode> <pathname>\n", command);
    printf("Example1:\n");
    printf("\t%s MD5 <pathname>\n", command);
    printf("\t%s SHA1 <pathname>\n", command);
    printf("\t%s SHA224 <pathname>\n", command);
    printf("\t%s SHA256 <pathname>\n", command);
    printf("Usage2:%s [-h/--help]\n", command);
    printf("Example2:\n");
    printf("\t%s --help\n", command);
    exit(-1);
}

int hash_writeFile(int fd, const char *pathname) {
    int handle;
    int num ;
    int ret = 0;
    unsigned char buff[MAXBUFSIZE];
    handle = open(pathname, O_RDONLY);
    if (handle < 0) {
        fprintf(stderr, "HASH:open file %s failed: %s\n", pathname, strerror(errno));
        return -1;
    }
    while ((num = read(handle, buff, MAXBUFSIZE)) > 0) {
        ret = hash_write(fd, buff, num);
        if (ret < 0) {
           close(handle);
           return -1;
        }
    }

    close(handle);
    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int fd;
    char *command = argv[0];
    const char *pathname = argv[2];
    int mode;
    unsigned long hash_size;
    unsigned char rec[50];
    memset(rec, 0, sizeof(rec));

    if (argc != 3)
        usage(command);

    if (strcmp(argv[1], "MD5") == 0) {
        mode = MD5;
        hash_size = MD5_byte;
    }else if (strcmp(argv[1], "SHA1") == 0) {
        mode = SHA1;
        hash_size = SHA1_byte;
    }else if (strcmp(argv[1], "SHA224") == 0) {
        mode = SHA224;
        hash_size = SHA224_byte;
    }else if (strcmp(argv[1], "SHA256") == 0) {
        mode = SHA256;
        hash_size = SHA256_byte;
    }else {
        usage(command);
    }
    if (access(pathname, F_OK) != 0) {
        printf("ERROR:file %s is not exist\n", pathname);
        usage(command);
    }

    fd = hash_init(mode);
    if(fd < 0) {
        printf("ERROR:hash_init error\n");
        return -1;
    }
    ret = hash_writeFile(fd, pathname);
    if(ret < 0) {
        printf("ERROR:hash_writeFile error\n");
        hash_deinit(fd, rec, hash_size);
        return -1;
    }
    ret = hash_deinit(fd, rec, hash_size);
    if (ret < 0) {
        printf("ERROR:hash_deinit error\n");
        return -1;
    }
    for (int i = 0;i < hash_size;i++) {
        printf("%02x", rec[i]);
    }
    printf("\n");
    return 0;
}
