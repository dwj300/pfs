#include "server.h"

int create_block(char* block_id) {
    FILE *file = fopen(block_id, "w+");
    if (file == NULL) {
        fprintf(stderr, "cannot create file\n");
        return -1;
    }
    fclose(file);
    return 0;
}

int write_block(char *block_id, char *data){
    FILE *file = fopen(block_id, "w");
    if (file == NULL) {
        fprintf(stderr, "file does not exist\n");
        return -1;
    }
    fwrite(data, 1, 1024, file);
    fclose(file);
    return 0;
}

int read_block(int socket_fd, char* block_id) {
    FILE *file = fopen(block_id, "r");
    if (file == NULL) {
        fprintf(stderr, "file does not exist\n");
        return -1;
    }
    char buffer[1024];
    fread(buffer, 1, 1024, file);
    int n = write(socket_fd, buffer, 1024);
    if (n < 0) {
        fprintf(stderr, "error writing to socket\n");
        return -1;
    }
    fclose(file);
    return 0;
}

int delete_block(char* block_id){
    int n = remove(block_id);
    if (n < 0) {
        fprintf(stderr, "Error deleting file\n");
        return -1;
    }
    return 0;
}

int parse_args(char* buffer, char** opcode, char** block_id, char** data) {
    char* space = strchr(buffer, ' ');
    *opcode = malloc(sizeof(char) * 10);
    strncpy(*opcode, buffer, (space - buffer));
    *block_id = space + sizeof(char);

    if (space == NULL) {
        fprintf(stderr, "request not formatted correctly\n");
        return -1;
    }
    // fprintf(stderr, "block id:%sd\n", block_id);
    char* last_space = strchr(*block_id, ' ');
    if (last_space == NULL) { // No data
        size_t ln = strlen(*block_id) - 1;
        if ((*block_id)[ln] == '\n') {
            (*block_id)[ln] = '\0';
        }
    }
    else {
        (*data) = malloc(sizeof(char)*1024);
        char *data_start = last_space + sizeof(char);
        *last_space = '\0';
        memcpy((*data), data_start, 1024);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, port, n;
    socklen_t clilen;
    if (argc < 2) {
        fprintf(stderr, "usage: server [port]\n"); //[directory]");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
        //char *directory = argv[2]; //todo
    }
    printf("File server starting on port: %d\n", port);

    char buffer[2048];

    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
          fprintf(stderr, "ERROR on accept\n");
          exit(1);
        }

        bzero(buffer,1040);
        n = read(newsockfd,buffer,1040);
        if (n < 0) {
            fprintf(stderr, "ERROR reading from socket");
            exit(1);
        }
        
        char *opcode = NULL;
        char *block_id = NULL;
        char *data = NULL;
        if (parse_args(buffer, &opcode, &block_id, &data) < 0) {
            continue;
        }

        if (strcmp(opcode, "CREATE") == 0) {
            create_block(block_id);
        }
        else if (strcmp(opcode, "READ") == 0) {
            read_block(newsockfd, block_id);
        }
        else if (strcmp(opcode, "WRITE") == 0) {
            write_block(block_id, data);
        }
        else if (strcmp(opcode, "DELETE") == 0) {
            delete_block(block_id);   
        }
        else {
            fprintf(stderr, "incorrect opcode");
        }
    }
    return 0;
}
