#include "pfs.h"

int pfs_create(const char *filename, int stripe_width)
{
    return -1;
}

int pfs_open(const char *filename, const char mode)
{
    return -1;
}

ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit)
{
    return -1;
}

ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit)
{
    return -1;
}

int pfs_close(int filedes)
{
    return -1;
}

int pfs_delete(const char *filename)
{
    return -1;
}

int pfs_fstat(int filedes, struct pfs_stat *buf) // Check the config file for the definition of pfs_stat structure
{
    return -1;
}

void initialize(int argc, char **argv)
{
    current_fd = 0;
    // We can probably delete all of this code
    // Set up cache 2 / (1*1024)
    int num_blocks = (CLIENT_CACHE_SIZE / PFS_BLOCK_SIZE) * 1024;
    void *cache = malloc(num_blocks * PFS_BLOCK_SIZE);
    free_list = malloc(sizeof(list_t));
    free_list->length = 0;
    free_list->head = cache;
    //void* temp = cache;
}
