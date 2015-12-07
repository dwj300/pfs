#include "grapevine.h"

int main(int argc, char* argv[]) {
int sockfd, newsockfd, port, n;
    socklen_t clilen;
    if (argc < 2) {
        fprintf(stderr, "usage: grapevine [port]\n"); //[directory]");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
        //char *directory = argv[2]; //todo
    }
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
