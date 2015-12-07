#include "pfs.h"

int main(int argc, char* argv[]) {
    initialize(argc, argv);
    int fd = pfs_open("foo.txt", 'r');
    if (fd == -1) {
        pfs_create("foo.txt", 1);    
    }
    fd = pfs_open("foo.txt", 'r');
    fprintf(stderr, "fd: %d\n", fd);
    /*pfs_create("foo.txt", 1);
    pfs_create("bar.txt", 1);
    pfs_create("foo.txt", 1);
    pfs_delete("foo.txt");
    pfs_create("foo.txt", 1);*/
    return 0;
}
