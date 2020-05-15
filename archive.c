#include "archive.h"

int archive_write(const int fd, struct tar_t ** head, const size_t filecount, const char *files[])
{
    if (fd < 0)
    {
        printf("Bad descriptor\n");
        return -1;
    }

    int offset = 0;
    struct tar_t ** archive = head;

    if (write_files(fd, archive, head, filecount, files, &offset) < 0)
    {
        printf("Failed to write archive data\n");
        return -1;
    }

    if (write_end(fd, offset) < 0) 
    {
        printf("Failed to write end data\n");
        return -1;
    }

    return offset;
}

void archive_free(struct tar_t * archive){
    while (archive)
    {
        struct tar_t * next = archive -> next;
        free(archive);
        archive = next;
    }
}

int read_size(int fd, char *buf, int size){
    int got = 0, rc;
    
    while ((got < size) && ((rc = read(fd, buf + got, size - got)) > 0)){
        got += rc;
    }
    
    return got;
}

int write_size(int fd, char *buf, int size){
    int wrote = 0, rc;
    
    while ((wrote < size) && ((rc = write(fd, buf + wrote, size - wrote)) > 0)){
        wrote += rc;
    }
    
    return wrote;
}

unsigned int oct2uint(char *oct, unsigned int size){
    unsigned int out = 0;
    int i = 0;
    
    while ((i < size) && oct[i]){
        out = (out << 3) | (unsigned int) (oct[i++] - '0');
    }

    return out;
}

int write_files(const int fd, struct tar_t **archive, struct tar_t **head, const size_t filecount, const char *files[], int *offset)
{
    if (fd < 0)
    {
        printf("Bad file descriptor\n");
        return -1;
    }

    if (filecount && !files)
    {
        printf("Non-zero file count provided, but file list is empty\n");
        return -1;
    }

    struct tar_t ** tar = archive;  // current entry
    
    for(unsigned int i = 0; i < filecount; i++) {
        *tar = malloc(sizeof(struct tar_t));

        if (make_tar_meta(*tar, files[i]) < 0)
        {
            printf("Could not create tar meta for %s\n", files[i]);
            return -1;
        }

        (*tar)->begin = *offset;

        printf("Writing %s\n", (*tar)->name);

        // write meta
        if (write_size(fd, (*tar)->block, 512) != 512){
            printf("Could not write tar meta for %s\n", (*tar)->name);
            return -1;
        }

        // write supported file types
        if (((*tar)->type == REGULAR) || ((*tar)->type == NORMAL) || ((*tar)->type == CONTIGUOUS))
        {
            int f = open((*tar)->name, O_RDONLY);
        
            if (f < 0)
            {
                printf("Could not open %s for reading\n", (*tar)->name);
                return -1;
            }

            int r = 0;
            char buf[512];
            while ((r = read_size(f, buf, 512)) > 0)
            {
                if (write_size(fd, buf, r) != r)
                {
                    printf("Could not write to archive: %s\n", strerror(errno));
                    return -1;
                }
            }

            close(f);
        }
        else
        {
            printf("Skipping unsupported file type %s\n", (*tar)->name);
        }
        
        // add padding
        const unsigned int size = oct2uint((*tar) -> size, 11);
        const unsigned int pad = 512 - size % 512;
        
        if (pad != 512)
        {
            for(unsigned int j = 0; j < pad; j++)
            {
                if (write_size(fd, "\0", 1) != 1)
                {
                    printf("Could not write padding\n");
                    return -1;
                }
            }
            *offset += pad;
        }

        *offset += size;
        tar = &((*tar)->next);

        *offset += 512;
    }

    return 0;
}

int write_end(const int fd, int size)
{
    if (fd < 0)
    {
        printf("Bad file descriptor\n");
        return -1;
    }

    // complete current record
    const int pad = RECORDSIZE - (size % RECORDSIZE);
    for(int i = 0; i < pad; i++)
    {
        if (write(fd, "\0", 1) != 1)
        {
            printf("Unable to write tar end\n");
            return -1;
        }
    }

    // if the current record does not have 2 blocks of zeros, add a whole other record
    if (pad < (2 * BLOCKSIZE))
    {
        for(int i = 0; i < RECORDSIZE; i++)
        {
            if (write(fd, "\0", 1) != 1)
            {
                printf("Unable to write tar end\n");
                return -1;
            }
        }
        return pad + RECORDSIZE;
    }

    return pad;
}

int make_tar_meta(struct tar_t *entry, const char *filename)
{   
    struct stat st;
    if (lstat(filename, &st))
    {
        printf("Could not get file status %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // handle relative path
    int move = 0;
    if (!strncmp(filename, "/", 1))
        move = 1;
    else if (!strncmp(filename, "./", 2))
        move = 2;
    else if (!strncmp(filename, "../", 3))
        move = 3;

    memset(entry, 0, sizeof(struct tar_t));
    
    // File meta
    strncpy(entry->original_name, filename, 100);
    strncpy(entry->name, filename + move, 100);
    snprintf(entry->mode, sizeof(entry->mode), "%07o", st.st_mode & 0777);
    snprintf(entry->uid, sizeof(entry->uid), "%07o", st.st_uid);
    snprintf(entry->gid, sizeof(entry->gid), "%07o", st.st_gid);
    snprintf(entry->size, sizeof(entry->size), "%011o", (int)st.st_size);
    snprintf(entry->modtime, sizeof(entry->modtime), "%011o", (int)st.st_mtime);
    
    // "magick number"
    memcpy(entry->ustar, "ustar  \x00", 8);

    // File type
    // @TODO only normal files supported
    switch (st.st_mode & S_IFMT) {
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
    }

    strncpy(entry->owner, buffer, sizeof(entry->owner) - 1);

    // group name
    struct group *grp = getgrgid(st.st_gid);
    if (grp)
    {
        strncpy(entry->group, grp->gr_name, sizeof(entry->group) - 1);
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
