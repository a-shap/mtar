#include "tar.h"

int make_tar_meta(struct archive_t* entry, const char *filename)
{   
    struct stat st;
    if (lstat(filename, &st))
    {
        printf("Could not get file status %s: %s\n", filename, strerror(errno));
        return -1;
    }

    memset(entry, 0, sizeof(struct archive_t));
    
    // File meta
    strncpy(entry->name, filename, 100);
    snprintf(entry->mode, sizeof(entry->mode), "%07o", st.st_mode & 0777);
    snprintf(entry->uid, sizeof(entry->uid), "%07o", st.st_uid);
    snprintf(entry->gid, sizeof(entry->gid), "%07o", st.st_gid);
    snprintf(entry->size, sizeof(entry->size), "%011o", (int)st.st_size);
    snprintf(entry->modtime, sizeof(entry->modtime), "%011o", (int)st.st_mtime);
    
    // "magick number"
    memcpy(entry->ustar, "ustar  \x00", 8);

    // File type
    // @TODO only normal files supported
    switch (st.st_mode & S_IFMT)
    {
        case S_IFREG:
            entry->type = NORMAL;
            break;
        default:
            entry->type = -1;
            printf("Unsupported file type found: %s\n", filename);
            break;
    }

    // username
    struct passwd pwd;
    char buffer[4096];
    struct passwd *result = NULL;
    if (getpwuid_r(st.st_uid, &pwd, buffer, sizeof(buffer), &result))
    {
        printf("Unable to get username of uid %u: %s\n", st.st_uid, strerror(errno));
        strncpy(buffer, "None", 4);
    }

    strncpy(entry->owner, buffer, sizeof(entry->owner) - 1);

    // group name
    struct group *gr_info = getgrgid(st.st_gid);
    if (gr_info)
    {
        strncpy(entry->group, gr_info->gr_name, sizeof(entry->group) - 1);
    }
    else
    {
        strncpy(entry->group, "None", 4);
    }
    
    // cheksum
    memset(entry->check, ' ', 8);

    unsigned int check = 0;
    for(int i = 0; i < 512; i++)
    {
        check += (unsigned char) entry->block[i];
    }

    snprintf(entry->check, sizeof(entry->check), "%06o0", check);

    entry->check[6] = '\0';
    entry->check[7] = ' ';

    return 0;
}
