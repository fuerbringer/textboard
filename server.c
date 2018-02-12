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
    if(pthread_create(&db_thread, NULL, db_thread_main, (void*)curr_post_list) < 0) {
        printf("Failed to load database thread!\n");
        post_list_destroy(curr_post_list);
        exit(1);
    }
}

static void cleanup_database() {
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
