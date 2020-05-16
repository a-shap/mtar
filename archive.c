#include "archive.h"

int archive_read(const int fd, struct ARCHIVE_P archive)
{    
    if (fd < 0)
    {
        printf("Bad file descriptor\n");
        return -1;
    }

    unsigned int offset = 0;
    int count = 0;

    struct ARCHIVE_P entry = archive;
    char update = 1;

    for(count = 0; ; count++)
    {
        *entry = malloc(sizeof(struct archive_t));

        if (update && (read_size(fd, (*entry)->block, BLOCKSIZE) != BLOCKSIZE))
        {
            printf("Could not read next block. IO error.\n");
            archive_free(*entry);
            *entry = NULL;
            return -1;
        }

        update = 1;
        // this block is all zeroes
        if (check_zeroes((*entry)->block, BLOCKSIZE))
        {
            if (read_size(fd, (*entry)->block, BLOCKSIZE) != BLOCKSIZE)
            {
                printf("Could not read next block. IO error.\n");
                archive_free(*entry);
                *entry = NULL;
                return -1;
            }

            // next one is too
            if (check_zeroes((*entry)->block, BLOCKSIZE))
            {
                archive_free(*entry);
                *entry = NULL;

                // skip to end of record
                if (lseek(fd, RECORDSIZE - (offset % RECORDSIZE), SEEK_CUR) == (off_t) (-1))
                {
                    printf("Can't seek file: %s \n", strerror(errno));
                    return -1;
                }

                break;
            }

            update = 0;
        }

        // set current entry's file offset
        (*entry)->begin = offset;

        // skip over data and unfilled block
        unsigned int jump = octal_to_uint((*entry)->size, 11);
        if (jump % BLOCKSIZE)
        {
            jump += BLOCKSIZE - (jump % BLOCKSIZE);
        }

        // move fd
        offset += BLOCKSIZE + jump;
        if (lseek(fd, jump, SEEK_CUR) == (off_t) (-1))
        {
            printf("Can't seek file: %s \n", strerror(errno));
            return -1;
        }

        entry = &((*entry)->next);
    }

    return count;
}

int archive_write(const int fd, struct ARCHIVE_P head, const size_t filecount, const char *files[])
{
    if (fd < 0)
    {
        printf("Bad descriptor\n");
        return -1;
    }

    int offset = 0;
    struct ARCHIVE_P archive = head;

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

void archive_free(struct ARCHIVE_ENTRY archive_entry)
{
    while (archive_entry)
    {
        struct ARCHIVE_ENTRY next = archive_entry->next;
        free(archive_entry);
        archive_entry = next;
    }
}

int archive_extract(const int fd, struct ARCHIVE_ENTRY archive)
{
    if (archive_read(fd, &archive) < 0)
    {
        archive_free(archive);
        close(fd);
        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) == (off_t) (-1)){
        printf("Can't seek file: %s \n", strerror(errno));
        return -1;
    }

    while (archive)
    {
        if (extract_file(fd, archive) < 0)
        {
            printf("Failed to extract file \n");
            return -1;
        }
        archive = archive -> next;
    }

    return 0;
}

int write_files(const int fd, struct ARCHIVE_P archive, struct ARCHIVE_P head, const size_t filecount, const char *files[], int *offset)
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

    struct ARCHIVE_P entry = archive;
    
    for(unsigned int i = 0; i < filecount; i++)
    {
        *entry = malloc(sizeof(struct archive_t));

        if (make_tar_meta(*entry, files[i]) < 0)
        {
            printf("Could not create tar meta for %s\n", files[i]);
            return -1;
        }

        (*entry)->begin = *offset;

        printf("Writing %s\n", (*entry)->name);

        // write meta
        if (write_size(fd, (*entry)->block, 512) != 512)
        {
            printf("Could not write tar meta for %s\n", (*entry)->name);
            return -1;
        }

        // write supported file types
        if (((*entry)->type == REGULAR) || ((*entry)->type == NORMAL) || ((*entry)->type == CONTIGUOUS))
        {
            int f = open((*entry)->name, O_RDONLY);
        
            if (f < 0)
            {
                printf("Could not open %s for reading\n", (*entry)->name);
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
            printf("Skipping unsupported file type %s\n", (*entry)->name);
        }
        
        // add padding
        const unsigned int size = octal_to_uint((*entry)->size, 11);
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
        entry = &((*entry)->next);

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

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

int extract_file(const int fd, struct ARCHIVE_ENTRY entry)
{
    printf("Extracting %s\n", entry->name);

    if ((entry->type == REGULAR) || (entry->type == NORMAL) || (entry->type == CONTIGUOUS))
    {
        size_t len = strlen(entry->name);
        if (!len)
        {
            printf("Entry with an empty name detected, exiting.\n");
            return -1;
        }

        // create file
        const unsigned int size = octal_to_uint(entry->size, 11);
        int f = open(entry->name, O_WRONLY | O_CREAT | O_TRUNC, octal_to_uint(entry->mode, 7) & 0777);
        if (f < 0)
        {
            printf("Could not create file %s: %s\n", entry->name, strerror(errno));
            return -1;
        }

        if (lseek(fd, 512 + entry->begin, SEEK_SET) == (off_t) (-1))
        {
            printf("Can't seek file: %s \n", strerror(errno));
            return -1;
        }

        char buf[512];
        int got = 0;
        while (got < size)
        {
            int r;

            if ((r = read_size(fd, buf, MIN(size - got, 512))) < 0)
            {
                printf("Can't read from archive: %s \n", strerror(errno));
                return -1;
            }

            if (write(f, buf, r) != r)
            {
                printf("Can't write to %s: %s", entry->name, strerror(errno));
                return -1;
            }

            got += r;
        }

        close(f);
    }
    return 0;
}

int read_size(int fd, char *buf, int size)
{
    int got = 0, rc;
    
    while ((got < size) && ((rc = read(fd, buf + got, size - got)) > 0))
    {
        got += rc;
    }
    
    return got;
}

int write_size(int fd, char *buf, int size)
{
    int wrote = 0, rc;
    
    while ((wrote < size) && ((rc = write(fd, buf + wrote, size - wrote)) > 0))
    {
        wrote += rc;
    }
    
    return wrote;
}

unsigned int octal_to_uint(char *oct, unsigned int size)
{
    unsigned int out = 0;
    int i = 0;
    
    while ((i < size) && oct[i])
    {
        out = (out << 3) | (unsigned int) (oct[i++] - '0');
    }

    return out;
}

int check_zeroes(char *buf, size_t size)
{
    for(size_t i = 0; i < size; buf++, i++)
    {
        if (*(char*)buf)
        {
            return 0;
        }
    }
    return 1;
}
