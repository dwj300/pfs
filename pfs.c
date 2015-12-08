#include "pfs.h"


// assume 255 max filename
// CREATE [filename] [strip] =
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
    if(recipe->num_blocks == -1) {
        fprintf(stderr, "Could not open %s. file does not exist\n", filename);
        return -1;
    }
    //files[current_fd] = malloc(sizeof(file_t)); // todo: maybe use malloc?
    int fd = current_fd;
    current_fd += 1;

    files[fd].recipe = recipe;
    current_fd += 1;
    close(socket_fd);
    return fd;
}

ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit) {
    file_t file = files[filedes];
    // This is the easy, single block case.
    if ((offset + nbyte) <= (PFS_BLOCK_SIZE * 1024)) {
        int block_id = 0;
        int global_block_id = file.recipe->blocks[0].block_id;
        byte* addr = ReadOrReserveBlockAndLock(cache, global_block_id, cache_hit);
        if((*cache_hit) == false) {
            server_t server = servers[file.recipe->blocks[block_id].server_id];
            read_block(server.hostname, server.port, global_block_id, &addr);
            UnlockBlock(cache, global_block_id);
        }
        memcpy(buf, addr+offset, nbyte);
        return nbyte;
    }
    return 0;

}
    /*
    int block_id = (offset / (PFS_BLOCK_SIZE * 1024));
    // TODO: multiple blocks...
    // if block doesn't exist
    if ((block_id + 1) >= file->recipe->num_blocks) {
        // huh?> for write... create (at least 1) block.
        return -1;
    }
    off_t cur_offset = offset % (PFS_BLOCK_SIZE * 1024);
    ssize_t amt_read = 0;
    char *temp = malloc(PFS_BLOCK_SIZE * 1024);
    while(amt_read < nbyte) {
        int global_block_id = file->recipe->blocks[block_id].block_id;
        // block not in cache
        if (get_block_from_cache(temp, global_block_id) == 0) {
            server_t *server = servers[file->recipe->blocks[block_id].server_id];
            if (read_block(server.hostname, server.port, global_block_id, char temp) != 1) {
                fprintf(stderr, "failed to read block from server\n");
                exit(1);
            }
            if (n)
            memcpy(buf, temp+cur_offset, )

        }
        // block in cache
        else {

        }
        block_id += 1;
        amt_read += (PFS_BLOCK_SIZE * 1024);
        buf += (PFS_BLOCK_SIZE * 1024);
    }

    WriteToBlockAndMarkDirty()
    bool WriteToBlockAndMarkDirty(cache_t * cache, global_block_id_t targetBlock, &temp, uint32_t startOffset, uint32_t endPosition ) {


    // if read token:
    //int global_block_id =

    //byte* data =

    // iterate over blocks
    //    check for read token
    //    spawn a thread:
    //        get from cache
    //        if cant
    //            got to network
    //            unlock block
    //    thread join
    return -1;
}*/

ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit) {

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
