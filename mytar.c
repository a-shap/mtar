#include <stdio.h>
#include "archive.h"

int main(int argc, char * argv[])
{
    argc -= 3;

    int return_code = 0;

    const char * opcode = argv[1];
    const char * filename = argv[2];
    const char ** files = (const char **) &argv[3];

    struct ARCHIVE_ENTRY archive = NULL;
    int fd = -1;

    if(strcmp(opcode, "c") == 0)
    {
        printf("Create mode\n");

        if ((fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
        {
            fprintf(stderr, "Error: Unable to open file %s\n", filename);
            return -1;
        }

        if (archive_write(fd, &archive, argc, files) < 0)
        {
            return_code = -1;
        }
    }
    else if (strcmp(opcode, "x") == 0)
    {
        printf("Extract mode\n");

        if ((fd = open(filename, O_RDWR)) < 0)
        {
            printf("Can't open file %s\n", filename);
            return -1;
        }

        if (archive_extract(fd, archive) < 0)
        {
            printf("Archive extraction failed\n");
            return_code = -1;
        }
    }

    archive_free(archive);
    close(fd);
    return return_code;
}
