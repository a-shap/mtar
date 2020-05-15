#include <stdio.h>
#include "archive.h"

int main(int argc, char * argv[])
{
    argc -= 2;

    int return_code = 0;

    const char * filename = argv[1];
    const char ** files = (const char **) &argv[2];

    struct tar_t * archive = NULL;
    int fd = -1;

    // Create mode
    if ((fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        return -1;
    }

    if (archive_write(fd, &archive, argc, files) < 0){
        return_code = -1;
    }

    archive_free(archive);
    close(fd);
    return return_code;
}