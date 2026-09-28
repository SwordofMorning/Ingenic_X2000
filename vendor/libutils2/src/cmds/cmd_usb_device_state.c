#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

static void usage(char *prg_name)
{
    fprintf(stderr, "%s usage [state]\n", prg_name);
    fprintf(stderr, "usb state list:\n");
    fprintf(stderr, "\t not attached\n");
    fprintf(stderr, "\t attached\n");
    fprintf(stderr, "\t powered\n");
    fprintf(stderr, "\t reconnecting\n");
    fprintf(stderr, "\t unauthenticated\n");
    fprintf(stderr, "\t default\n");
    fprintf(stderr, "\t addresssed\n");
    fprintf(stderr, "\t configured\n");
    fprintf(stderr, "\t suspended\n");

    exit(-1);
}


int main(int argc, char *argv[])
{
    int fd;
    fd_set efds;
    int ret, len;
    char buf[128];
    DIR *dirptr = NULL;
    struct dirent *entry;
    char *usb_state = NULL;

    if (argc > 2)
        usage(argv[0]);

    usb_state = argv[1];

    dirptr = opendir("/sys/class/udc/");
    if (dirptr == NULL) {
        fprintf(stderr, "%s opendir /sys/class/udc/ fail\n", argv[0]);
        return -1;
    }

	while((entry = readdir(dirptr)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        break;
    }
    if (entry == NULL) {
        fprintf(stderr, "%s /sys/class/udc/ directory is empty \n", argv[0]);
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "/sys/class/udc/%s/state", entry->d_name);

    closedir(dirptr);

    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s open %s fail %d\n", argv[0], buf, errno);
        return -1;
    }

    while (1) {
        FD_ZERO(&efds);
        FD_SET(fd, &efds);

        ret = select(fd + 1, NULL, NULL, &efds, NULL);
        if (ret == -1) {
            fprintf(stderr, "%s select fail %d\n", argv[0], errno);
            break;
        }

        if (FD_ISSET(fd, &efds)) {
            memset(buf, 0, sizeof(buf));
            len = read(fd, buf, sizeof(buf));
            if (len > 0) {
                if (usb_state) {
                    if (strncmp(buf, usb_state, len - 1) == 0)
                        break;
                } else {
                    printf("usb state : %s\n", buf);
                }
            }

            lseek(fd, 0, SEEK_SET);
        }
    }

    close(fd);

    return 0;
}
