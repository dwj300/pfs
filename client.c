#include "client.h"

int read_block_from_fs(int socket_fd, int block_id, char **buffer)  {
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

int write_block_on_fs(int socket_fd, int block_id, char *data) {
    char command[1042];
    sprintf(command, "WRITE %d", block_id);
    int l = strlen(command);
    void *start = command + (sizeof(char) * l) + 1;
    memcpy(start, data, 1024);
    int n = write(socket_fd, command, (1024 + l + 1));
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

int create_block(char *host, int port, int block_id) {
    int sock_id = connect_socket(host, port);
    create_block_on_fs(sock_id, block_id);
    close(sock_id);
    return 0;
}

int read_block(char *host, int port, int block_id, char **data) {
    int sock_id = connect_socket(host, port);
    read_block_from_fs(sock_id, block_id, data);
    close(sock_id);
    return 0;
}

int write_block(char *host, int port, int block_id, char *data) {
    int sock_id = connect_socket(host, port);
    write_block_on_fs(sock_id, block_id, data);
    close(sock_id);
    return 0;
}

int delete_block(char *host, int port, int block_id) {
    int sock_id = connect_socket(host, port);
    delete_block_on_fs(sock_id, block_id);
    close(sock_id);
    return 0;
}

int main(int argc, char *argv[]) {
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
