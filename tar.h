#ifndef __TAR_H__
#define __TAR_H__

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

#define BLOCKSIZE       512
#define RECORDSIZE      10240 // 20 of BLOCKSIZE = 10kb

#define REGULAR          0
#define NORMAL          '0' // ('0' or (ASCII NUL) - previous clause)
#define CONTIGUOUS      '7'
// No other formats are supported

// https://en.wikipedia.org/wiki/Tar_(computing)#Header
struct archive_t {
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

    struct archive_t* next;
};

int make_tar_meta(struct archive_t* entry, const char * filename);

#endif
