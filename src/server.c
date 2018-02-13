#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "helpers.h"
#include "posts.h"
#include "config.h"

#define MAXPENDING 5
#define DEFAULT_PORT 8080

// Definitions
static int sockfd = -1;
static pthread_t db_thread;
extern void *db_thread_main(void*);
extern void handle(const int sockfd);

// Database actions
static void init_database() {
    global_id = 0;
    
    curr_post_list = post_list_create();
    if(curr_post_list == NULL) {
        printf("Failed to create curr_post_list\n");
        exit(1);
    }
    
    // Load database from file
    FILE *f = fopen(DATABASE_FILE, "r");
    if(f == NULL) {
        printf("Cannot open database file!\n");
    } else {
        char *line = NULL;
        size_t len = 0;
        int header = 0;
        unsigned int loaded = 0;
        while (getline(&line, &len, f) != -1) {
            if(!header) {
                header = 1;
                continue;
            }
            
            #ifndef PRODUCTION
            printf("%s\n",line);
            #endif
            
            char *_saveptr;
            char *id_str = strtok_r(line, ",", &_saveptr);
            unsigned int id = atoi(id_str);
            char *name = strtok_r(NULL, ",", &_saveptr);
            char *subject = strtok_r(NULL, ",", &_saveptr);
            if(streq(subject,DATABASE_DELIM_EMPTY)) subject = "";
            char *comment = strtok_r(NULL, ",", &_saveptr);
            char *created_time_str = strtok_r(NULL, ",", &_saveptr);
            time_t created_time = atoi(created_time_str);
            char *parent_str = strtok_r(NULL, "\n", &_saveptr);
            
            #ifndef PRODUCTION
            printf("%i,%s,%s,%s,%li,%s\n", id, name, subject, comment, created_time, parent_str);
            #endif
            
            if(streq(parent_str, DATABASE_DELIM_EMPTY)) {
                global_id = max(global_id, id);
                if(post_create(id, name, subject, comment, created_time, NULL) == NULL) {
                    printf("Failed to create post %i\n", id);
                    goto end;
                }
                loaded++;
            } else {
                unsigned int parent_id = atoi(parent_str);
                struct post *parent;
                if((parent = post_list_find(curr_post_list, parent_id)) != NULL) {
                    global_id = max(global_id, id);
                    if(post_create(id, name, subject, comment, created_time, parent) == NULL) {
                        printf("Failed to create post %i\n", id);
                     goto end;
                    }
                    loaded++;
                } else {
                    printf("Ignoring #%i...\n", id);
                }
            }
        }
        if(loaded > 0)
            global_id++;
    }
    
    // Initialise the thread
    if(pthread_create(&db_thread, NULL, db_thread_main, (void*)curr_post_list) < 0) {
        printf("Failed to load database thread!\n");
        post_list_destroy(curr_post_list);
        goto end;
    }
    
    return;

end:
    fclose(f);
    exit(1);
}

static void cleanup_database() {
    pthread_cancel(db_thread);
    post_list_destroy(curr_post_list);
}

// Signal handlers
static void cleanup(const int sig) {
    printf("Cleaning up...\n");
    if(sockfd > 0 && shutdown(sockfd, 0) < 0)
        printf("WARNING! Failed to close server socket: %s\n", strerror(errno));
    cleanup_database();
    exit(0);
}

// Main
int main(const int argc, const char *argv[]) {
    // Initialize signals
    signal(SIGINT, cleanup);
    signal(SIGKILL, cleanup);
    
    // Parse arguments
    int port = DEFAULT_PORT;
    if(argc > 1) {
        if(streq(argv[1], "-h")) {
            printf("%s [-h|port number]\n", argv[0]);
            exit(0);
        } else {
            char *endptr = "";
            port = strtol(argv[1], &endptr, 10);
            if(strlen(endptr)) {
                printf("Invalid port number!\n");
                exit(1);
            }
        }
    }
    
    // Initialize database
    init_database();
    
    // Initialize TCP sockets
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        printf("Cannot open socket: %s\n", strerror(errno));
        exit(errno);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        printf("Cannot set socket opts: %s\n", strerror(errno));
        close(sockfd);
        exit(errno);
    }
    struct sockaddr_in server, client;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Failed to bind: %s\n", strerror(errno));
        close(sockfd);
        exit(errno);
    }
    if(listen(sockfd, MAXPENDING) < 0) {
        printf("Failed to listen: %s\n", strerror(errno));
        close(sockfd);
        exit(errno);
    }
    printf("Listening on port %i\n", port);
    
    // Event loop
    while(1) {
        unsigned int clientlen = sizeof(client);
        int clientfd;
        if((clientfd = accept(sockfd, (struct sockaddr *)&client, &clientlen)) < 0) {
            printf("Failed to accept client connection: %s\n", strerror(errno));
            break;
        }
        printf("Client connected: %s\n", inet_ntoa(client.sin_addr));
        handle(clientfd);
    }
    
    // Fin
    if(sockfd > 0 && shutdown(sockfd, 0) < 0)
        printf("WARNING! Failed to close server socket: %s\n", strerror(errno));
    cleanup_database();
    return 0;
}
