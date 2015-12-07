#include "grapevine.h"

// CREATE FILENAME STRIPE_WIDTH
int create_file(int socket_fd, char* filename, int stripe_width) {
    // Check if filename exists
    // Create it somehow...

    if (lookup(files, filename) != NULL) {
        fprintf(stderr, "File with name:%s already exists.\n", filename);
        int success = 0;
        write(socket_fd, &success, sizeof(int));
        return 0;
    }
    else {
        file_t* file = malloc(sizeof(file_t));
        file->filename = filename;
        insert(files, filename, file);
        int success = 1;
        write(socket_fd, &success, sizeof(int));
    }
    return 1;
}

int delete_file(int socket_fd, char* filename) {
    // Check if filename exists
    entry_t* node = lookup(files, filename);
    if (lookup(files, filename) == NULL) {
        fprintf(stderr, "File with name:%s doesn't exist.\n", filename);
        int success = 0;
        write(socket_fd, &success, sizeof(int));
        return 0;
    }
    else {
        file_t* file = (file_t*)node->value;
        // delete blocks off file servers
        delete(files, filename);
        int success = 1;
        write(socket_fd, &success, sizeof(int));
    }
    return 1;
}

int fstat(char *filename) {
    fprintf(stderr, "ERROR: not implemented yet\n");
    exit(1);
    return 0;
}

int parse_args(char* buffer, char** opcode, void** data1, void** data2) {
    char* space = strchr(buffer, ' ');
    *opcode = malloc(sizeof(char) * 10);
    if (space == NULL) {
        fprintf(stderr, "error: invalid command\n");
        return -1;
    }
    strncpy(*opcode, buffer, (space - buffer));
    space += sizeof(char);
    char* next_space = strchr(space, ' ');
    if (next_space != NULL) {
        (*data1) = malloc(255 * sizeof(char));
        strncpy(*data1, space, (next_space-space));
        (*data2) = (next_space + sizeof(char));
    }
    else {
        (*data1) = space;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int sockfd, newsockfd, port, n;
    socklen_t clilen;
    if (argc < 2) {
        fprintf(stderr, "usage: grapevine [port]\n");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
    }
    files = create_dictionary();
    fprintf(stderr, "Metadata manager starting on port: %d\n", port);

    char buffer[2048];

    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR on binding");
        exit(1);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
          fprintf(stderr, "ERROR on accept");
          exit(1);
        }

        bzero(buffer,269);
        n = read(newsockfd,buffer,269);
        if (n < 0) {
            fprintf(stderr, "ERROR reading from socket");
            exit(1);
        }
        fprintf(stderr, "%s\n", buffer);


        char* opcode = NULL;
        void* data1 = NULL;
        void* data2 = NULL;
        if (parse_args(buffer, &opcode, &data1, &data2) < 0) {
            continue;
        }
        fprintf(stderr, "opcode:%sdone\n", opcode);

        if (strcmp(opcode, "CREATE") == 0) {
            fprintf(stderr, "got create!\n");
            char *filename = (char*)data1;
            char* stripe_str = (char*)data2;
            int stripe_width = atoi(stripe_str);
            create_file(newsockfd, filename, stripe_width);
        }
        else if (strcmp(opcode, "DELETE") == 0) {
            char* filename = (char*)data1;
            delete_file(newsockfd, filename);
        }/*
        else if (strcmp(opcode, "WRITE") == 0) {
            write_block(block_id, data);
        }
        else if (strcmp(opcode, "FSTAT") == 0) {
            delete_block(block_id);
        }*/
        else {
            fprintf(stderr, "incorrect opcode");
        }
    }
    return 0;
}
