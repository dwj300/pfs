#include "pfs.h"

bool check_read_token(file_t* file, int block_index) {
    token_node_t* cur = file->read_tokens;
    while(cur != NULL) {
        token_t* token = cur->token;
        if ((token->start_block <= block_index) && (block_index <= token->end_block)) {
            return true;
        }
    }
    return false;
}

bool check_write_token(file_t* file, int block_index) {
    token_node_t* cur = file->write_tokens;
    while(cur != NULL) {
        token_t* token = cur->token;
        if ((token->start_block <= block_index) && (block_index <= token->end_block)) {
            return true;
        }
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
    sprintf(command, "READ_TOKEN %s %d", file->filename, block_index);
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
    sprintf(command, "WRITE_TOKEN %s %d", file->filename, block_index);
    fprintf(stderr, "COMMAND:%sdone\n", command);
    int length = strlen(command);
    write(socket_fd, command, length+1);
    token_t* token = malloc(sizeof(token_t));
    read(socket_fd, token, sizeof(token_t));
    fprintf(stderr, "New write token: %d->%d\n", token->start_block, token->end_block);
    add_token(file, token, false);
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

// TODO: token logic
ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit) {
    *cache_hit = 1;
    file_t* file = &(files[filedes]);
    void* current_pos = buf;
    int start_block_id = offset / (1024*PFS_BLOCK_SIZE);
    int end_block_id = (offset + nbyte - 1) / (1024*PFS_BLOCK_SIZE); // the -1 is for the multiple of 1024 case.
    int current_offset = offset - (start_block_id*1024*PFS_BLOCK_SIZE);
    int bytes_read = 0;
    for(int i = start_block_id; i <= end_block_id; i++) {
        if (i+1 >= file->recipe->num_blocks) {
            // Block doesn't exist... fail.
            fprintf(stderr, "block doesn't exist.\n");
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

        if(!check_write_token(file, i)) {
            get_write_token(file, i);
        }

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
    // TODO: something with tokens
    // flush any blocks in the cache to server. 
    file_t* file = &(files[filedes]);
    fprintf(stderr, "closing file: %s\n", file->filename);
    for(int i = 0; i < file->recipe->num_blocks; i++) {
        FlushBlockToServer(cache, file->recipe->blocks[i].block_id);
    }
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
    char* command = "SERVERS";
    int length = strlen(command);
    write(socket_fd, command, length+1);
    read(socket_fd, servers, NUM_FILE_SERVERS * sizeof(server_t));
    close(socket_fd);
    return servers;
}

void print_servers() {
    for(int i = 0; i < NUM_FILE_SERVERS; i++) {
        fprintf(stderr, "server %d: hostname->%s, port->%d\n", i, servers[i].hostname, servers[i].port);
    }
}

//void print_rec

void initialize(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s [hostname] [port]\n", argv[0]);
        exit(1);
    }
    grapevine_host = argv[1]; //"localhost";
    grapevine_port = atoi(argv[2]); //9875;
    servers = get_servers();
    print_servers();
    current_fd = 0;
    cache = InitializeCache(PFS_BLOCK_SIZE * 1024, 2048, 80, 20);
}

void cleanup() {
    cache->exiting = true; // TODO: probably not needed 
}
