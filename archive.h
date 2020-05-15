#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define BLOCKSIZE       512
#define RECORDSIZE      10240 // 20 of BLOCKSIZE = 10kb

#define REGULAR          0
#define NORMAL          '0' // ('0' or (ASCII NUL) - previous clause)
#define HARDLINK        '1'
#define SYMLINK         '2'
#define CHAR            '3'
#define BLOCK           '4'
#define DIRECTORY       '5'
#define FIFO            '6'
#define CONTIGUOUS      '7'

// https://en.wikipedia.org/wiki/Tar_(computing)#Header
struct tar_t {
    char original_name[100];
    unsigned int begin;

    union {
        // File header
        union {
            // Header
            struct {
                char name[100];      // File name
                char mode[8];        // File mode
                char uid[8];         // Owner's numeric user ID
                char gid[8];         // Group's numeric user ID
                char size[12];       // File size in bytes (octal base) 
                char modtime[12];    // Last modification time in numeric Unix time format (octal)
                char check[8];       // Checksum for header record
                char is_link;        // Link indicator (file type)
                char link_name[100]; // Name of linked file
            };

            // UStar format
            struct {
                char old[156];        // (several fields, same as in old format)
                char type;            // Type flag
                char _link_name[100]; // (same field as in old format)
                char ustar[8];        // UStar indicator "ustar" then NUL
                char owner[32];       // Owner user name 
                char group[32];       // Owner group name
                char major[8];        // Device major number
                char minor[8];        // Device minor number
                char prefix[155];     // Filename prefix
            };
        };

        // Padded file data
        char block[512]; // raw data
    };

    struct tar_t * next;
};

// API
int archive_write(const int fd, struct tar_t ** head, const size_t filecount, const char *files[]);
void archive_free(struct tar_t * archive);
// Writing
int write_files(const int fd, struct tar_t ** archive, struct tar_t ** head, const size_t filecount, const char * files[], int * offset);
int write_end(const int fd, int size);

// Util
int make_tar_meta(struct tar_t * entry, const char * filename);
void error(const char * reason);

#endif
