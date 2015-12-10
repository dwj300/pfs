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
        pfs_stat_t* stat = malloc(sizeof(pfs_stat_t));
        stat->pst_size = 0;
        stat->pst_ctime = time(0);
        stat->pst_mtime = time(0);
        file->stat = stat;

        recipe_t *recipe = malloc(sizeof(recipe_t));
        recipe->num_blocks = 0;
        file->recipe = recipe;

        insert(files, filename, file);
        int success = 1;
        write(socket_fd, &success, sizeof(int));
    }
    return 1;
}

int delete_file(int socket_fd, char* filename) {
    // Check if filename exists
    entry_t* node = lookup(files, filename);
    if (node == NULL) {
        fprintf(stderr, "File with name:%s doesn't exist.\n", filename);
        int success = 0;
        write(socket_fd, &success, sizeof(int));
        return 0;
    }
    else {
        file_t* file = (file_t*)node->value;
        // delete blocks off file servers
        //free(file);
        for(int i = 0; i < file->recipe->num_blocks; i++) {
            server_t *server = &(servers[file->recipe->blocks[i].server_id]);
            fprintf(stderr, "Deleting block: %d from %s:%d\n", file->recipe->blocks[i].block_id, server->hostname, server->port);
            delete_block(server->hostname, server->port, file->recipe->blocks[i].block_id);
        }

        free(file->stat);
        free(file->recipe);
        free(file);
        int success = delete(files, filename);
        write(socket_fd, &success, sizeof(int));
        //close(socket_fd);
    }
    return 1;
}

int open_file(int socket_fd, char* filename) {
    entry_t* e = lookup(files, filename);
    if (e == NULL) {
        fprintf(stderr, "File: %s does not exist\n", filename);
        recipe_t* recipe = malloc(sizeof(recipe_t));
        recipe->num_blocks = -1;
        write(socket_fd, recipe, sizeof(recipe_t));
        close(socket_fd);
        return -1;
    }
    file_t* file = (file_t*)e->value;
    write(socket_fd, file->recipe, sizeof(recipe_t));
    write(socket_fd, file->stat, sizeof(pfs_stat_t));

    close(socket_fd);
    return 1;
}

int fstat(char *filename) {
    fprintf(stderr, "ERROR: not implemented yet\n");
    exit(1);
    return 0;
}

void get_servers(int socket_fd) {
    write(socket_fd, servers, NUM_FILE_SERVERS * sizeof(server_t));
    close(socket_fd);
}

int create_block_gv(int socket_fd, char *filename) {
    fprintf(stderr, "-1\n");
    entry_t* e = lookup(files, filename);
    fprintf(stderr, "0\n");

    if (e == NULL) {
        fprintf(stderr, "file: %s doesn't exist", filename);
        recipe_t* recipe = malloc(sizeof(recipe_t));
        recipe->num_blocks = -1;
        write(socket_fd, recipe, sizeof(recipe_t));
        close(socket_fd);
        return -1;
    }
    fprintf(stderr, "1\n");
    int gid = current_block_id;
    fprintf(stderr, "2\n");
    file_t *file = e->value;
    fprintf(stderr, "3\n");
    fprintf(stderr, "#:%d\n", file->recipe->num_blocks);

    file->recipe->blocks[file->recipe->num_blocks].block_id = gid;
    fprintf(stderr, "1\n");
    fprintf(stderr, "foo2\n");

    int server = (current_server_id % NUM_FILE_SERVERS);
    current_server_id += 1;

    file->recipe->blocks[file->recipe->num_blocks].server_id = server; // TODO: Round robin?
    fprintf(stderr, "assigning server %d\n", server);
    
    file->recipe->num_blocks += 1;
    fprintf(stderr, "block_id:%d\n", file->recipe->blocks[0].block_id);
    // Create block on server.
    create_block(servers[server].hostname, servers[server].port, gid);
    // send back new recipe?
    write(socket_fd, file->recipe, sizeof(recipe_t));
    close(socket_fd);
    file->stat->pst_size += 1024 * PFS_BLOCK_SIZE;
    current_block_id += 1;

    
    return gid;
}

int parse_args(char* buffer, char** opcode, void** data1, void** data2) {
    char* space = strchr(buffer, ' ');
    *opcode = malloc(sizeof(char) * 10);
    if (space == NULL) {
        (*opcode) = buffer;
    }
    else{
        char *end = stpncpy(*opcode, buffer, (space - buffer));
        (*end) = '\0';
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
    }
    fprintf(stderr, "fooopcode:%sd\n", (*opcode));
    return 0;
}

void initialize() {
    files = create_dictionary();
    servers = malloc(NUM_FILE_SERVERS*sizeof(server_t));
    strcpy(servers[0].hostname, "localhost");
    servers[0].port = 8081;
    strcpy(servers[1].hostname, "localhost");
    servers[1].port = 8082;
    current_server_id = 0;
}

int main(int argc, char* argv[]) {
    fprintf(stderr, "size: %lu\n", sizeof(recipe_t));
    int sockfd, newsockfd, port, n;
    socklen_t clilen;
    if (argc < 2) {
        fprintf(stderr, "usage: grapevine [port]\n");
        exit(1);
    }
    else {
        port = atoi(argv[1]);
    }
    initialize();
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

        bzero(buffer,269);
        n = read(newsockfd,buffer,269);
        if (n < 0) {
            fprintf(stderr, "ERROR reading from socket\n");
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
        }
        else if (strcmp(opcode, "SERVERS") == 0) {
            get_servers(newsockfd);
        }
        else if (strcmp(opcode, "OPEN") == 0) {
            char* filename = (char*)data1;
            open_file(newsockfd, filename);
        }
        else if (strcmp(opcode, "C_BLOCK") == 0) {
            char* filename = (char*)data1;
            create_block_gv(newsockfd, filename);
        }
        /*
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
