#include "pfs.h"

bool check_read_token(file_t* file, int block_index) {
    token_node_t* cur = file->read_tokens;
    while(cur != NULL) {
        token_t* token = cur->token;
        int start = token->start_block;
        int end = (token->end_block == -1) ? file->recipe->num_blocks - 1 : token->end_block;       
        if ((start <= block_index) && (block_index <= end)) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

bool check_write_token(file_t* file, int block_index) {
    token_node_t* cur = file->write_tokens;

    while(cur != NULL) {
        token_t* token = cur->token;
        int start = token->start_block;
        int end = (token->end_block == -1) ? file->recipe->num_blocks - 1 : token->end_block;       
        if ((start <= block_index) && (block_index <= end)) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

void add_token(file_t* file, token_t* token, bool read) {
    token_node_t* node = malloc(sizeof(token_node_t));
    node->token = token;
    node->next = NULL;
    //variable = condition ? value_if_true : value_if_false
    token_node_t* cur = (read == true) ? file->read_tokens : file->write_tokens;

    while(cur != NULL && cur->next != NULL) {
        cur = cur->next;
    }
    if(cur == NULL) {
        if (read == true) {
            file->read_tokens = node;
        }
        else {
            file->write_tokens = node;
        }
    }
    else {
        cur->next = node;
    }
}

bool get_read_token(file_t* file, int block_index) {
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }
    char* command = malloc(269*sizeof(char));
    sprintf(command, "READ_TOKEN %s %d %d", file->filename, block_index, client_id);
    fprintf(stderr, "COMMAND:%sdone\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    token_t* token = malloc(sizeof(token_t));
    read(socket_fd, token, sizeof(token_t));
    fprintf(stderr, "New read token: %d->%d\n", token->start_block, token->end_block);
    add_token(file, token, true);
    close(socket_fd);
    return true;
}

bool get_write_token(file_t* file, int block_index) {
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to open socket\n");
        exit(1);
    }
    char* command = malloc(269*sizeof(char));
    sprintf(command, "WRITE_TOKEN %s %d %d", file->filename, block_index, client_id);
    fprintf(stderr, "COMMAND:%sdone\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    token_t* token = malloc(sizeof(token_t));
    read(socket_fd, token, sizeof(token_t));
    fprintf(stderr, "New write token: %d->%d\n", token->start_block, token->end_block);
    add_token(file, token, false);
    // lets also create a read token
    token_t* read_token = malloc(sizeof(token_t));
    read_token->start_block = token->start_block;
    read_token->end_block = token->end_block;
    read_token->client_id = token->client_id;
    add_token(file, read_token, true);
    close(socket_fd);
    return true;
}
// assume 255 max filename
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
    int fd = current_fd;
    current_fd += 1;
    files[fd].filename = filename;
    fprintf(stderr, "h1\n");

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
    recipe_t* recipe = malloc(sizeof(recipe_t));
    pfs_stat_t *stats = malloc(sizeof(pfs_stat_t));
    read(socket_fd, recipe, sizeof(recipe_t));
    files[fd].recipe = recipe;
    if(files[fd].recipe->num_blocks == -1) {
        fprintf(stderr, "Could not open %s. file does not exist\n", filename);
        return -1;
    }
    read(socket_fd, stats, sizeof(pfs_stat_t));
    close(socket_fd);
    files[fd].stat = stats;
    return fd;
}

void client_create_block(file_t *file) {
    fprintf(stderr, "creating a new block!\n");
    char* command = malloc(269*sizeof(char));
    sprintf(command, "C_BLOCK %s", file->filename);
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    write(socket_fd, command, strlen(command));
    recipe_t* recipe = malloc(sizeof(recipe_t));
    read(socket_fd, recipe, sizeof(recipe_t));
    close(socket_fd);
    file->recipe = recipe;
    if (file->recipe->num_blocks == -1) {
        fprintf(stderr, "Failed to create a new block.");
        exit(1);
    }
    fprintf(stderr, "New num_blocks: %d\n", file->recipe->num_blocks);
}

ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit) {
    *cache_hit = 1;
    file_t* file = &(files[filedes]);
    void* current_pos = buf;
    int start_block_id = offset / (1024*PFS_BLOCK_SIZE);
    int end_block_id = (offset + nbyte - 1) / (1024*PFS_BLOCK_SIZE); // the -1 is for the multiple of 1024 case.
    int current_offset = offset - (start_block_id*1024*PFS_BLOCK_SIZE);
    int bytes_read = 0;
    for(int i = start_block_id; i <= end_block_id; i++) {
        if (i+1 > file->recipe->num_blocks) {
            // Block doesn't exist... fail.
            fprintf(stderr, "block index: %d doesn't exist.\n", i);
            return -1;
        }
        if(!check_read_token(file, i)) {
            get_read_token(file, i);
        }

        int end_position = current_offset + (nbyte - bytes_read);
        if (end_position > 1024) {
            end_position = 1024;
        }
        int hit;
        bool success = ReadBlockFromCache(cache, file->recipe->blocks[i].block_id, &hit, current_offset, (end_position - current_offset), file->recipe->blocks[i].server_id, current_pos);
        if (!success) {
            fprintf(stderr, "Failed to read data\n");
            return -1;
        }
        *cache_hit = (*cache_hit) & hit;
        current_pos += (end_position - current_offset);
        bytes_read += (end_position - current_offset);
        current_offset = 0;
    }
    return bytes_read;
}

// TODO: deal with cache hit ...
// TODO: token logic
ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit) {
    *cache_hit = 1;
    file_t *file = &(files[filedes]);
    const void* current_pos = buf;
    int start_block_id = offset / (1024*PFS_BLOCK_SIZE);
    int end_block_id = (offset + nbyte - 1) / (1024*PFS_BLOCK_SIZE);
    int current_offset = offset - (start_block_id*1024*PFS_BLOCK_SIZE);
    int bytes_written = 0;
    for(int i = start_block_id; i <= end_block_id; i++) {
        if (i+1 >= file->recipe->num_blocks) {
            // Need to create a block on server. (matbe more. TODO: fix dis)
            client_create_block(file);
            fprintf(stderr, "new block id: %d\n", file->recipe->blocks[i].block_id);
            fprintf(stderr, "BLOCK_ID: %d\n", files[filedes].recipe->blocks[i].block_id);
        }
        fprintf(stderr, "we have a problem0\n");
        if(!check_write_token(file, i)) {
            fprintf(stderr, "we have a problem1\n");
            get_write_token(file, i);
            fprintf(stderr, "we have a problem2\n");
        }
        fprintf(stderr, "we have a problem3\n");

        int end_position = current_offset + (nbyte - bytes_written);
        if (end_position > 1024) {
            end_position = 1024;
        }
        int hit;
        WriteToBlockAndMarkDirty(cache, file->recipe->blocks[i].block_id, current_pos, current_offset, end_position, file->recipe->blocks[i].server_id, &hit);
        *cache_hit = *cache_hit & hit;
        current_pos += (end_position - current_offset);
        bytes_written += (end_position - current_offset);
        current_offset = 0;
    }

    // TODO: change cache write to be a constant void * to match API.
    //int gid = file->recipe->blocks[block_id].block_id; // TODO: change api to be offset and size
    //WriteToBlockAndMarkDirty(cache, gid, buf, offset_within_block, offset_within_block+nbyte, file->recipe->blocks[block_id].server_id); // TODO: also pass server????

    //fprintf(stderr, "new recipe blocks!: %d\n", file->recipe->num_blocks);
    return bytes_written;
}

int pfs_close(int filedes) {
    // Tell the grapevine to release all out our tokens
    file_t *file = &(files[filedes]);

    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    char* command = malloc(269*sizeof(char));
    sprintf(command, "CLOSE %s %d", file->filename, client_id);
    fprintf(stderr, "COMMAND:%s\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    close(socket_fd);
    // flush any blocks in the cache to server.
    fprintf(stderr, "closing file: %s\n", file->filename);
    for(int i = 0; i < file->recipe->num_blocks; i++) {
        FlushBlockToServer(cache, file->recipe->blocks[i].block_id);
    }
    // TODO: Free the parts of a file. Maybe we should use a linked list instead of an array to keep track of files on the client.......
    return 1;
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
    // must have a read token for last token in file
    memcpy(buf, files[filedes].stat, sizeof(pfs_stat_t));
    return 1;
}

server_t* get_servers() {
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    servers = malloc(NUM_FILE_SERVERS * sizeof(server_t));
    char* command = malloc(275 * sizeof(char));//"SERVERS";
    sprintf(command, "SERVERS %s %d", hostname, my_port);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    read(socket_fd, &client_id, sizeof(int));
    fprintf(stderr, "my client id: %d\n", client_id);
    read(socket_fd, servers, NUM_FILE_SERVERS * sizeof(server_t));
    close(socket_fd);
    return servers;
}

void print_servers() {
    for(int i = 0; i < NUM_FILE_SERVERS; i++) {
        fprintf(stderr, "server %d: hostname->%s, port->%d\n", i, servers[i].hostname, servers[i].port);
    }
}

// REVOKE FILENAME INDEX
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
    fprintf(stderr, "opcode:%sd\n", (*opcode));
    return 0;
}

void revoke_token(int socket_fd, char* filename, int index, char token_type) {
    // DONE: Also deal with read tokens? I kind of think we should pass in the request which one the client should be revoking

    fprintf(stderr, "start revoke token\n");
    file_t *file = NULL;
    // have to iterate through files cause we are sent a filename not FD
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].filename, filename) == 0) {
            file = &(files[i]);
            break;
        }
    }
    if (file == NULL) {
        fprintf(stderr, "file not found\n");
        exit(1);
    }

    token_node_t *cur;

    if (token_type == 'W') {
        cur = file->write_tokens;
    
        while(cur != NULL && cur->next != NULL) {
            int start = cur->token->start_block;
            int end = (cur->token->end_block == -1) ? file->recipe->num_blocks - 1 : cur->token->end_block;       

            if (start <= index && index <= end) {
                // found our token...
                // by checking strictly > we can't have out of bounds
                if (index > file->last_write) {
                    cur->token->end_block = index-1;
                }
                else if (index < file->last_write) {
                    cur->token->start_block = file->last_write + 1;
                    // TODO: check for out of bounds
                }
                else {
                    fprintf(stderr, "[ERROR]: Some very strange case just happened...\n");
                    exit(1);
                }
                // Send back the new token:
                write(socket_fd, cur->token, sizeof(token_t));
                break;
            }
            cur = cur->next;
        }
    }
    // Deal with read tokens
    else {
        cur = file->read_tokens;
    
        while(cur != NULL && cur->next != NULL) {
            int start = cur->token->start_block;
            int end = (cur->token->end_block == -1) ? file->recipe->num_blocks - 1 : cur->token->end_block;       

            if (start <= index && index <= end) {
                // found our token...
                // by checking strictly > we can't have out of bounds
                if (index > file->last_read) {
                    cur->token->end_block = index-1;
                }
                else if (index < file->last_read) {
                    cur->token->start_block = file->last_read + 1;
                    // TODO: check for out of bounds
                }
                else {
                    fprintf(stderr, "[ERROR]: Some very strange case just happened...\n");
                    exit(1);
                }
                // Send back the new token:
                write(socket_fd, cur->token, sizeof(token_t));
                break;
            }
            cur = cur->next;
        }
    }

    if (cur == NULL) {
        fprintf(stderr, "[ERROR]: We didn't actually revoke a token... so deal with it\n");
        exit(1);
    }
    fprintf(stderr, "end revoke token\n");

}

void* revoker(void* arg) {
    fprintf(stderr, "Starting revoker on port: %d\n", my_port);
    int sockfd, newsockfd, n;
    socklen_t clilen;
    char buffer[512];
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening revoker socket\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(my_port);
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

        bzero(buffer,512);
        n = read(newsockfd,buffer,512);
        if (n < 0) {
            fprintf(stderr, "ERROR reading from socket");
            exit(1);
        }
        // REVOKE FILENAME INDEX
        char *opcode = NULL;
        void *data1 = malloc(256 * sizeof(char));
        void *data2 = malloc(20 * sizeof(char));

        if (parse_args(buffer, &opcode, &data1, &data2) < 0) {
            fprintf(stderr, "error parsing revoke request\n");
            exit(1);
        }
        // REVOKE [filename] [index] [R/W]
        if (strcmp(opcode, "REVOKE") == 0) {
            char *filename = (char*)data1;
            char *index_str = (char*)data2;
            char *space = strchr(index_str, ' ');
            *space = '\0';
            char *token_type = space + sizeof(char);
            int index = atoi(index_str);
            revoke_token(newsockfd, filename, index, *token_type);
        }
        else {
            fprintf(stderr, "incorrect opcode");
        }
    }
    return NULL;
}

void initialize(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s [hostname] [port]\n", argv[0]);
        exit(1);
    }

    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    fprintf(stderr, "my hostname: %s\n", hostname);

    grapevine_host = argv[1]; //"localhost";
    grapevine_port = atoi(argv[2]); //9875;
    my_port = atoi(argv[3]); //9875;
    servers = get_servers();
    print_servers();
    current_fd = 0;
    cache = InitializeCache(PFS_BLOCK_SIZE * 1024, 2048, 80, 20);
    pthread_create(&revoker_thread, NULL, revoker, NULL);
}

void cleanup() {
    cache->exiting = true; // TODO: probably not needed
}
