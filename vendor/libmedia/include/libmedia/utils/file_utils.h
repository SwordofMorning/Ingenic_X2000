#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

int file_read_data(const char *input_file, void **data_p, long *size_p);

int file_write_data(const char *out_file, void *data, unsigned long data_size);

int read_timeout(int fd, void *buf, int size, int timeout);

int write_timeout(int fd, void *buf, int size, int timeout);

#endif /* __FILE_UTILS_H__ */
