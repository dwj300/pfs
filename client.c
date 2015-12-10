#include "client.h"

int read_block_from_fs(int socket_fd, int block_id, byte** buffer)  {
    char command[17];
    sprintf(command, "READ %d", block_id);
    int n = write(socket_fd, command, strlen(command));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket\n");
        return -1;
    }
    bzero((*buffer),1024);
    n = read(socket_fd, (*buffer), 1024);
    if (n < 0) {
         fprintf(stderr, "ERROR reading from socket\n");
         return -1;
    }
    return 0;
}

int write_block_on_fs(int socket_fd, int block_id, byte* data) {
    fprintf(stderr, "data:%.*s\n", 10, data);
    char* command = calloc(1042,sizeof(char));
    sprintf(command, "WRITE %d ", block_id); // the weird space is cause the null terminator is being weird
    int l = strlen(command);
    fprintf(stderr, "length:%d\n", l);
    void *start = command + l;// + (sizeof(char) * l) + 1;
    memcpy(start, data, 1024);
    fprintf(stderr, "%.*s\n", 20, command);
    
    int n = write(socket_fd, command, 1024 + l); //(1024 + l + 1));
    if (n < 0) {
        fprintf(stderr, "error writing data");
        return -1;
    }
    return 0;
}

int create_block_on_fs(int socket_fd, int block_id) {
    char command[17];
    sprintf(command, "CREATE %d", block_id);
    int n = write(socket_fd, command, strlen(command));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket\n");
        return -1;
    }
    return 0;
}

int delete_block_on_fs(int socket_fd, int block_id) {
    char command[17];
    sprintf(command, "DELETE %d", block_id);
    int n = write(socket_fd, command, strlen(command));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket\n");
        return -1;
    }
    return 0;
}

int create_block(char *host, int port, int block_id) {
    int sock_id = connect_socket(host, port);
    create_block_on_fs(sock_id, block_id);
    close(sock_id);
    return 1;
}

int read_block(char *host, int port, int block_id, byte** data) {
    int sock_id = connect_socket(host, port);
    read_block_from_fs(sock_id, block_id, data);
    close(sock_id);
    return 1;
}

int write_block(char *host, int port, int block_id, byte* data) {
    int sock_id = connect_socket(host, port);
    write_block_on_fs(sock_id, block_id, data);
    close(sock_id);
    return 1;
}

int delete_block(char *host, int port, int block_id) {
    int sock_id = connect_socket(host, port);
    delete_block_on_fs(sock_id, block_id);
    close(sock_id);
    return 1;
}

int test_client(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: client [port]\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    char *data = malloc(sizeof(char)*1024);
    read_block("localhost", port, 99, &data);
    fprintf(stderr, "%s", data);

    /*for(int i=0; i < 1024; i++) {
        data[i] = 'a';
    }*/

    // test file server
    return 0;
}
