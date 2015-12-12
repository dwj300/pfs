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
        if (stripe_width > NUM_FILE_SERVERS) {
            fprintf(stderr, "Error, too many stripes. Instead we will use %d servers.\n", NUM_FILE_SERVERS);
            stripe_width = NUM_FILE_SERVERS;
        }
        file->filename = filename;
        pfs_stat_t* stat = malloc(sizeof(pfs_stat_t));
        stat->pst_size = 0;
        stat->pst_ctime = time(0);
        stat->pst_mtime = time(0);
        file->stat = stat;
        file->read_tokens = NULL;
        file->write_tokens = NULL;
        file->stripe_width = stripe_width; // TODO: actually use this.
        file->current_stripe = 0;
        file->current_stripe = 0;

        recipe_t *recipe = malloc(sizeof(recipe_t));
        recipe->num_blocks = 0;
        printf("numblocks: 0\n");
        file->recipe = recipe;

        insert(files, filename, file);
        int success = 1;
        write(socket_fd, &success, sizeof(int));
    }
    return 1;
}

void revoke_token(char* filename, int index, int client_id, token_t* token, char token_type) {
    // sends a message to the correct client (lookip via clients table)
    // tell them to revoke
    // write their response at their current token.
    fprintf(stderr, "revoking %c token(%d->%d, on client %d\n", token_type, token->start_block, token->end_block, client_id);
    server_t *client = &(clients[client_id]);
    int socket_fd = connect_socket(client->hostname, client->port);
    if (socket_fd < 0) {
        fprintf(stderr, "failed to connect socket to client\n");
        exit(1);
    }
    char* command = malloc(270*sizeof(char));
    sprintf(command, "REVOKE %s %d %c", filename, index, token_type);
    write(socket_fd, command, strlen(command)+1);
    read(socket_fd, token, sizeof(token_t));
    close(socket_fd);
    fprintf(stderr, "new client token %d->%d, on client %d\n", token->start_block, token->end_block, token->client_id);
    fprintf(stderr, "GV done revoking token\n");
    //sleep(5);
}

void get_read_token(int socket_fd, char* filename, int index, int client_id) {
    // easy case for now. we can fix logic once we get tokens sending over the wire
    // TODO
    entry_t* e = lookup(files, filename);

    if (e == NULL) {
        fprintf(stderr, "File with name:%s doesn't exists.\n", filename);
        token_t* token = malloc(sizeof(token_t));
        token->start_block = INVALID_TOKEN; // indicates invalid
        token->end_block = 0;
        write(socket_fd, token, sizeof(token_t));
        close(socket_fd);
        return;
    }
    else {
        file_t* file = e->value;
        int temp_s = 0;
        int temp_e = file->recipe->num_blocks - 1;
        // Only need to check write tokens...
        token_node_t *cur = file->write_tokens;
        while(cur != NULL) {
            token_t* token = cur->token;

            int start = token->start_block;
            int end = token->end_block;
            if (start == INVALID_TOKEN) {
                // its a dud token.
                cur = cur->next;
                continue;
            }
            if (end == INFINITE_TOKEN) {
                end = file->recipe->num_blocks - 1;
            }

            // if t before
            if (end < index) {
                if (end > temp_s) {
                    temp_s = end + 1;
                }
            }
            // if t after
            else if (start > index) {
                if (end <= temp_s) {
                    temp_e = start - 1;
                }
            }
            else if (start <= index && end >= index) {
                // if t overlap
                revoke_token(filename, index, token->client_id, token, 'W');
                start = token->start_block;
                end = token->end_block;
                // look at the new token
                if (start == INVALID_TOKEN) {
                    // its a dud token.
                    cur = cur->next;
                    continue;
                }   
                if (start > index) {
                    temp_e = start - 1;
                }
                else if (end < index) {
                    temp_s = end + 1;    
                }
                else {
                    fprintf(stderr, "ERROR: client sent back a bad token. Dying.\n");
                    exit(1);
                }
            }
            else {
                fprintf(stderr, "ERROR: client sent back a bad token. Dying.\n");
                exit(1);
            }
            cur = cur->next;
        }

        if (temp_e == file->recipe->num_blocks - 1) {
            temp_e = INFINITE_TOKEN; // -1 indicates infinity...
        }

        token_t* new_token = malloc(sizeof(token_t));
        new_token->start_block = temp_s;
        new_token->end_block = temp_e;
        new_token->client_id = client_id;
        token_node_t* node = malloc(sizeof(token_node_t));
        node->token = new_token;
        // Add to head of linked list
        node->next = file->read_tokens;
        file->read_tokens = node;
        write(socket_fd, new_token, sizeof(token_t));
        close(socket_fd);
    }
}

void get_write_token(int socket_fd, char* filename, int index, int client_id) {
    fprintf(stderr, "starting temp sleep\n");
    //sleep(10);
    fprintf(stderr, "wokeup from temp sleep\n");
    // this is complicated. psuedocode for now
    // TODO
    entry_t* e = lookup(files, filename);

    if (e == NULL) {
        fprintf(stderr, "File with name:%s doesn't exists.\n", filename);
        token_t* token = malloc(sizeof(token_t));
        token->start_block = INVALID_TOKEN;
        token->end_block = 0;
        write(socket_fd, token, sizeof(token_t));
        close(socket_fd);
        return;
    }
    else {
        fprintf(stderr, "started from the bottom\n");
        file_t *file = e->value;
        // First look through write tokens - there should only be one at most overlapping.
        int temp_s = 0;
        int temp_e = file->recipe->num_blocks - 1;
        token_node_t *cur = file->write_tokens;

        while(cur != NULL) {
            token_t* token = cur->token;
            // if t before
            int start = token->start_block;
            int end = token->end_block;
            if (end == INFINITE_TOKEN) {
                end = file->recipe->num_blocks - 1;
            }

            if (end < index) {
                if (end > temp_s) {
                    temp_s = end + 1;
                    if(temp_s > file->recipe->num_blocks - 1) { 
                        temp_s = file->recipe->num_blocks - 1;
                    }
                }
            }
            // if t after
            else if (start > index) {
                if (end <= temp_s) {
                    temp_e = start - 1;
                    if (temp_e < 0) {
                        temp_e = 0;
                    }
                }
            }
            else if (start <= index && end >=index) {
                // if t overlap
                revoke_token(filename, index, token->client_id, token, 'W');
                // look at the new token
                start = token->start_block;
                end = token->end_block;
                if (start > index) {
                    temp_e = start - 1;
                }
                else if (end < index) {
                    temp_s = end + 1;    
                }
                else {
                    fprintf(stderr, "ERROR: client sent back a bad token. Dying.\n");
                    exit(1);
                }
            }
            else {
                fprintf(stderr, "some other type of token found\n");
                exit(1);
            }
            cur = cur->next;
        }

        // After we have our temp_s and temp_e, look through read tokens doing the same thing?
        cur = file->read_tokens;
        while(cur != NULL) {
            token_t* token = cur->token;
            
            int start = token->start_block;
            int end = token->end_block;
            if (end == INFINITE_TOKEN) {
                end = file->recipe->num_blocks - 1;
            }

            // if t before
            if (end < index) {
                if (end > temp_s) {
                    temp_s = end + 1;
                }
            }
            // if t after
            else if (end > index) {
                if (end <= temp_s) {
                    temp_e = start - 1;
                }
            }
            else if (start <= index && end >=index) {
                // if t overlap
                revoke_token(filename, index, token->client_id, token, 'R');
                start = token->start_block;
                end = token->end_block;
                // look at the new token
                if (start > index) {
                    temp_e = start - 1;
                }
                else if (end < index) {
                    temp_s = end + 1;    
                }
                else {
                    fprintf(stderr, "ERROR: client sent back a bad token. Dying.\n");
                    exit(1);
                }
            }
            else {
                fprintf(stderr, "token is invalid.\n");
                exit(1);
            }
            cur = cur->next;
        }

        if (temp_e == file->recipe->num_blocks - 1) {
            temp_e = INFINITE_TOKEN; // -1 indicates infinity...
        }
        token_t* new_token = malloc(sizeof(token_t));
        token_t* read_token = malloc(sizeof(token_t));

        new_token->start_block = temp_s;
        new_token->end_block = temp_e;
        new_token->client_id = client_id;

        read_token->start_block = temp_s;
        read_token->end_block = temp_e;
        read_token->client_id = client_id;

        // Add to head of write token linked list
        token_node_t* node = malloc(sizeof(token_node_t));
        node->token = new_token;
        node->next = file->write_tokens;
        file->write_tokens = node;
        // Add to head of read token linked list
        token_node_t* read_node = malloc(sizeof(token_node_t));
        read_node->token = read_token;
        read_node->next = file->read_tokens;
        file->read_tokens = read_node;

        write(socket_fd, new_token, sizeof(token_t));
        close(socket_fd);
        fprintf(stderr, "now we're here\n");

    }
}

void close_file(char* filename, int client_id) {
    entry_t* e = lookup(files, filename);
    if (e == NULL) {
        fprintf(stderr, "File with name:%s doesn't exists.\n", filename);
        return;
    }
    file_t *file = e->value;
    token_node_t *prev = NULL;
    token_node_t *cur = file->write_tokens;
    while(cur != NULL) {
        if (cur->token->client_id == client_id) {
            if (prev == NULL) {
                file->write_tokens = cur->next;
            }
            else {
                prev->next = cur->next;
            }
            
        }
        else {
        }
        prev = cur;
        cur = cur->next;
    }
    // Do the same for any read tokens open
    prev = NULL;
    cur = file->read_tokens;
    while(cur != NULL) {
        if (cur->token->client_id == client_id) {
            if (prev == NULL) {
                file->read_tokens = cur->next;
            }
            else {
                prev->next = cur->next;
            }
        }
        else {
        }
        prev = cur;
        cur = cur->next;
    }
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
        recipe->num_blocks = INVALID_FILE;
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

void get_servers_gv(int socket_fd, char* hostname, int port) {
    fprintf(stderr, "new client id:%d, %s:%d\n", current_cid, hostname, port);
    strcpy(clients[current_cid].hostname, hostname);
    clients[current_cid].port = port;
    write(socket_fd, &current_cid, sizeof(int));
    write(socket_fd, servers, NUM_FILE_SERVERS * sizeof(server_t));
    close(socket_fd);
    current_cid += 1;
}

int create_block_gv(int socket_fd, char *filename) {
    fprintf(stderr, "-1\n");
    entry_t* e = lookup(files, filename);
    fprintf(stderr, "0\n");

    if (e == NULL) {
        fprintf(stderr, "file: %s doesn't exist", filename);
        recipe_t* recipe = malloc(sizeof(recipe_t));
        recipe->num_blocks = INVALID_FILE;
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

    //int server = (current_server_id % NUM_FILE_SERVERS);
    //current_server_id += 1;

    // Round robin
    if (file->current_stripe == STRIP_SIZE - 1) {
        file->current_stripe = 0;
        file->current_server += 1;
        file->current_server = file->current_server % file->stripe_width;
    }

    file->recipe->blocks[file->recipe->num_blocks].server_id = file->current_server;
    file->current_stripe += 1;

    fprintf(stderr, "assigning server %d\n", file->current_server);
    
    file->recipe->num_blocks += 1;
    printf("numblocks: %d\n", file->recipe->num_blocks);
    fprintf(stderr, "block_id:%d\n", file->recipe->blocks[0].block_id);
    // Create block on server.
    create_block(servers[file->current_server].hostname, servers[file->current_server].port, gid);
    // send back new recipe?
    write(socket_fd, file->recipe, sizeof(recipe_t));
    close(socket_fd);
    file->stat->pst_size += 1024 * PFS_BLOCK_SIZE;
    current_block_id += 1;

    return gid;
}

int parse_args(char* buffer, char** opcode, void** data1, void** data2) {
    fprintf(stderr, "in parse_args, command:%sd\n", buffer);
    char* space = strchr(buffer, ' ');
    *opcode = malloc(sizeof(char) * 20);
    if (space == NULL) {
        (*opcode) = buffer;
    }
    else{
        char *end = stpncpy(*opcode, buffer, (space - buffer));
        (*end) = '\0';
        space += sizeof(char); // increment space pointer to first char of filename
        char* next_space = strchr(space, ' ');
        if (next_space != NULL) {
            (*data1) = malloc(255 * sizeof(char));
            char *end2 = stpncpy(*data1, space, (next_space - space));
            (*end2) = '\0';

            //strncpy(*data1, space, (next_space-space));
            (*data2) = (next_space + sizeof(char));
        }
        else {
            (*data1) = space; // this might be a problem, idk
        }
    }
    fprintf(stderr, "fooopcode:%sd\n", (*opcode));
    fprintf(stderr, "filename:%sd\n", (char*)(*data1));
    return 0;
}

void initialize() {
    files = create_dictionary();
    servers = malloc(NUM_FILE_SERVERS*sizeof(server_t));
    strcpy(servers[0].hostname, "localhost");
    servers[0].port = 8081;
    strcpy(servers[1].hostname, "localhost");
    servers[1].port = 8082;
    //current_server_id = 0;
    clients = calloc(MAX_CLIENTS, sizeof(server_t));
    current_cid = 0;
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
            int success = delete_file(newsockfd, filename);
            fprintf(stderr, "success: %d\n", success);
        }
        else if (strcmp(opcode, "SERVERS") == 0) {
            char* hostname = (char*)data1;
            char* port_str = (char*)data2;
            int port = atoi(port_str);
            get_servers_gv(newsockfd, hostname, port);
        }
        else if (strcmp(opcode, "OPEN") == 0) {
            char* filename = (char*)data1;
            open_file(newsockfd, filename);
        }
        else if (strcmp(opcode, "C_BLOCK") == 0) {
            char* filename = (char*)data1;
            create_block_gv(newsockfd, filename);
        }
        
        else if (strcmp(opcode, "READ_TOKEN") == 0) {
            char* filename = (char*)data1;
            char* int_str = (char*)data2;
            char* space = strchr(int_str, ' ');
            *space = '\0';
            space += sizeof(char);
            int block_index = atoi(int_str);
            int client_id = atoi(space);
            get_read_token(newsockfd, filename, block_index, client_id);
        }
        else if (strcmp(opcode, "WRITE_TOKEN") == 0) {
            char* filename = (char*)data1;
            char* int_str = (char*)data2;
            char* space = strchr(int_str, ' ');
            *space = '\0';
            space += sizeof(char);
            int block_index = atoi(int_str);
            int client_id = atoi(space);
            get_write_token(newsockfd, filename, block_index, client_id);
        }
        else if (strcmp(opcode, "CLOSE") == 0) {
            char* filename = (char*)data1;
            char* client_id_str = (char*)data2;
            int client_id = atoi(client_id_str);
            close_file(filename, client_id);
        }/*
        else if (strcmp(opcode, "FSTAT") == 0) {
            delete_block(block_id);
        }*/
        else {
            fprintf(stderr, "incorrect opcode");
        }
    }
    return 0;
}
