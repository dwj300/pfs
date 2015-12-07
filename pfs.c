#include "pfs.h"

int connect_socket(char *host, int port) {
    int sockfd;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        return -1;
    }
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR connecting\n");
        return -1;
    }

    return sockfd;
}
// assume 255 max filename
// CREATE [filename] [strip] =
int pfs_create(const char *filename, int stripe_width) {
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }
    char* command = malloc(269*sizeof(char));
    sprintf(command, "CREATE %s %d", filename, stripe_width);
    fprintf(stderr, "COMMAND:%s\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    int success;
    read(socket_fd, &success, sizeof(int));
    fprintf(stderr, "success: %d\n", success);
    close(socket_fd);
    return success;
}

int pfs_open(const char *filename, const char mode) {
    // Connect to grapevine:
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }
    char* command = malloc(269*sizeof(char));
    sprintf(command, "OPEN %s", filename);
    fprintf(stderr, "COMMAND: %s\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1); // +1 for null terminator. Not sure if needed
    //read(socket_fd, &success, sizeof(int)); 
    // TODO ???
    close(socket_fd);
    return -1;
}

ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit) {
    return -1;
}

ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit) {

    return -1;
}

int pfs_close(int filedes) {
    return -1;
}

int pfs_delete(const char *filename) {
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }
    char* command = malloc(269*sizeof(char));
    sprintf(command, "DELETE %s", filename);
    fprintf(stderr, "COMMAND:%s\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    int success;
    read(socket_fd, &success, sizeof(int));
    fprintf(stderr, "success: %d\n", success);
    close(socket_fd);
    return success;
}

int pfs_fstat(int filedes, struct pfs_stat *buf) { // Check the config file for the definition of pfs_stat structure
    return -1;
}

void initialize(int argc, char **argv) {
    grapevine_host = "localhost";
    grapevine_port = 9876;
    current_fd = 0;
}
