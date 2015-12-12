#include "pfs.h"

int main(int argc, char* argv[]) {
    initialize(argc, argv);
    //pfs_create("foo.txt", 1);
    int fd = pfs_open("pfs_file1", 'w');
    //char *data = "test data123";
    char *data = malloc(1024);
    //int cache_hit;
    int cache_hit;
    //pfs_write(fd, data, strlen(data)+1, 0, &cache_hit);
    //sleep(30);
    pfs_read(fd, data, 1024, 1025, &cache_hit);
    fprintf(stderr, "data:%s\n", data);
    sleep(10);
    pfs_close(fd);
    //cleanup();*/
    /*if (fd == -1) {

    }
    fd = pfs_open("foo.txt", 'r');
    fprintf(stderr, "fd: %d\n", fd);
    pfs_create("foo.txt", 1);
    pfs_create("bar.txt", 1);
    pfs_create("foo.txt", 1);
    pfs_delete("foo.txt");
    pfs_create("foo.txt", 1);*/
    /*int cache_hit2;
    void* data1 = malloc(10*1024);
    int fd = pfs_open("pfs_file1", 'r');
    pfs_read(fd, data1, 4*1024, 0, &cache_hit2);
    fprintf(stderr, "DATA%.*s\n", 4096, (char*)data1);*/
    return 0;
}
