#include "pfs.h"

int main(int argc, char* argv[]) {
    initialize(argc, argv);
    pfs_create("foo.txt", 1);
    pfs_create("bar.txt", 1);
    pfs_create("foo.txt", 1);
    pfs_delete("foo.txt");
    pfs_create("foo.txt", 1);
    return 0;
}
