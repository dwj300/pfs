#include "server.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void create_block(){}

void update_block(){}

//READ [id]
void read_block(int socket_fd, char* block_id)
{
    FILE *file = fopen(block_id, "r");
    if (file == NULL)
    {
        fprintf(stderr, "file does not exist\n");
        return;
    }
    char buffer[1024];
    fread(buffer, 1, 1024, file);
    int n = write(socket_fd, buffer, 1024);
    if (n < 0)
    {
        fprintf(stderr, "error writing to socket");   
    }
}

void delete_block(){}


int main(int argc, char *argv[])
{
    int sockfd, newsockfd, port, clilen, n;
    if (argc < 3)
        error("usage: server [port] [directory]");
    port = atoi(argv[1]);
    char *directory = argv[2];

    printf("File server starting on port: %d\n", port);

    char buffer[2048];

    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
          error("ERROR on binding");

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
          error("ERROR on accept");
        bzero(buffer,256);
        n = read(newsockfd,buffer,2048);
        if (n < 0)
            error("ERROR reading from socket");

        printf("Here is the message: %s\n",buffer);
            //n = write(newsockfd,"I got your message",18);
        
        char *space = strchr(buffer, ' ');
        char opcode[10];
        strncpy(opcode, buffer, (space - buffer));
        space += sizeof(char);

        if (space == NULL)
        {
            fprintf(stderr, "request not formatted correctly\n");
            continue;
        }
        if (strcmp(opcode, "CREATE") == 0)
        {

        }
        else if (strcmp(opcode, "READ") == 0)
        {
            /*char *next_space = strchr(space, ' ');
            fprintf(stderr, next_space);
            if (next_space == NULL)
            {
                fprintf(stderr, "request not formatted correctly\n");
                continue;
            }
            char block_id[20];
            strncpy(block_id, space, (next_space - space));
            fprintf(stderr, "id:%sg\n", block_id);*/
            //int block_id = atoi(space);

            fprintf(stderr, "block id:%sd\n", space);
            size_t ln = strlen(space) - 1;
            if (space[ln] == '\n')
                space[ln] = '\0';
            read_block(newsockfd, space);
        }
        else if (strcmp(opcode, "UPDATE") == 0)
        {
            
        }
        else if (strcmp(opcode, "DELETE") == 0)
        {
            
        }
        else
        {
            fprintf(stderr, "incorrect opcode");
        }   
        fprintf(stderr, "opcode: %s\n", opcode);

        if (n < 0)
            error("ERROR writing to socket");
    }
    return 0;
}
