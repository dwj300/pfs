#include "pfs.h"

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
    recipe_t *recipe = malloc(sizeof(recipe_t));
    read(socket_fd, recipe, sizeof(recipe_t));
    files[fd].recipe = recipe;
    if(files[fd].recipe->num_blocks == -1) {
        fprintf(stderr, "Could not open %s. file does not exist\n", filename);
        return -1;
    }
    
    close(socket_fd);
    return fd;
}

ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit) {
    file_t file = files[filedes];
    // This is the easy, single block case.
    // assume read token...
    if ((offset + nbyte) <= (PFS_BLOCK_SIZE * 1024)) {
        int block_id = 0;
        int global_block_id = file.recipe->blocks[block_id].block_id;
        bool success = ReadBlockFromCache(cache, global_block_id, cache_hit, offset, nbyte, file.recipe->blocks[block_id].server_id, buf);
        if (!success) {
            fprintf(stderr, "Failed to read data\n");
            return -1;
        }
        if((*cache_hit) == false) {
            //server_t server = servers[file.recipe->blocks[block_id].server_id];
            //read_block(server.hostname, server.port, global_block_id, &addr);
            //UnlockBlock(cache, global_block_id);
        }
        //memcpy(buf, addr+offset, nbyte);
        return nbyte;
    }
    return 0;

}

void client_create_block(file_t *file) {
    char* command = malloc(269*sizeof(char));
    sprintf(command, "C_BLOCK %s", file->filename);
    int socket_fd = connect_socket(grapevine_host, grapevine_port);
    write(socket_fd, command, strlen(command)+1);
    recipe_t* recipe = malloc(sizeof(recipe_t));
    read(socket_fd, recipe, sizeof(recipe_t));
    close(socket_fd);
    file->recipe = recipe;
    if (file->recipe->num_blocks == -1) {
        fprintf(stderr, "Failed to create a new block.");
        exit(1);
    }
    fprintf(stderr, "try1: %d\n", file->recipe->blocks[0].block_id);
}

ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit) {
    file_t *file = &(files[filedes]);

    int block_id = offset / (1024*PFS_BLOCK_SIZE);
    int offset_within_block = offset - (block_id*1024*PFS_BLOCK_SIZE);
    if (block_id+1 >= file->recipe->num_blocks) {
        // Need to create a block. (matbe more. TODO: fix dis)
        client_create_block(file);
        fprintf(stderr, "try1: %d\n", file->recipe->blocks[0].block_id);

        fprintf(stderr, "BLOCK_ID: %d\n", files[filedes].recipe->blocks[block_id].block_id);

    }
    // TODO: change cache write to be a constant void * to match API.
    int gid = file->recipe->blocks[block_id].block_id; // TODO: change api to be offset and size
    WriteToBlockAndMarkDirty(cache, gid, buf, offset_within_block, offset_within_block+nbyte, file->recipe->blocks[block_id].server_id); // TODO: also pass server????

    fprintf(stderr, "new recipe blocks!: %d", file->recipe->num_blocks);
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
    // must have a read token for last token in file
    return -1;
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
