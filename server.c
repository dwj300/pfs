#include "server.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[ERROR]: no port provided\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    printf("File server starting on port: %d\n", port);
}
