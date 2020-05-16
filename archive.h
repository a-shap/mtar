#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "tar.h"

// convenience type hacks
#define ARCHIVE_P archive_t**
#define ARCHIVE_ENTRY archive_t*

// API
int archive_read(const int fd, struct ARCHIVE_P archive);
int archive_write(const int fd, struct ARCHIVE_P head, const size_t filecount, const char *files[]);

int archive_extract(const int fd, struct ARCHIVE_ENTRY archive);

void archive_free(struct ARCHIVE_ENTRY archive);

// Writing
int write_files(const int fd, struct ARCHIVE_P archive, struct ARCHIVE_P head, const size_t filecount, const char *files[], int *offset);
int write_end(const int fd, int size);

// Extracting
int extract_file(const int fd, struct ARCHIVE_ENTRY file);

// Util
int read_size(int fd, char *buf, int size);
int write_size(int fd, char *buf, int size);
unsigned int octal_to_uint(char *oct, unsigned int size);
int check_zeroes(char *buf, size_t size);

#endif
