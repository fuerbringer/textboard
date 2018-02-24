#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
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
#include "database.h"

#define MAXPENDING 5
#define DEFAULT_PORT 8080

// Definitions
static int sockfd = -1;
static pthread_t db_thread;
extern void *db_thread_main(void*);
extern void handle(const int sockfd);
struct db_thread_params *db_thread_params = NULL;

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
            int colons = strocc(line, ',');

            // OLD (6): id, name, subject, comment, created_time, parent
            // NEW (8): id, name, subject, comment, created_time, parent, delete_passwd, deleted

            char *name, *subject, *comment, *parent_str, *delete_passwd = NULL;
            unsigned int id;
            time_t created_time;
            int deleted = 0;

            if(colons == 5 || colons == 7) {
                char *_saveptr;
                id = atoi(strtok_r(line, ",", &_saveptr));
                name = strtok_r(NULL, ",", &_saveptr);
                subject = strtok_r(NULL, ",", &_saveptr);
                if(streq(subject,DATABASE_DELIM_EMPTY)) subject = "";
                comment = strtok_r(NULL, ",", &_saveptr);
                created_time = atoi(strtok_r(NULL, ",", &_saveptr));
                if(colons == 7) {
                    parent_str = strtok_r(NULL, ",", &_saveptr);
                    delete_passwd = strtok_r(NULL, ",", &_saveptr);
                    deleted = atoi(strtok_r(NULL, "\n", &_saveptr));
                } else {
                    parent_str = strtok_r(NULL, "\n", &_saveptr);
                }
            } else {
                printf("Unexpected number of colons in database (got %i)\n", colons);
                goto end;
            }

            if(deleted) {
                struct post *post = NULL;
                if((post = post_list_findr(curr_post_list, id)) != NULL) {
                    printf("Destroying %i...\n", id);
                    post_destroy(post);
                    continue;
                } else
                    continue;
            }

            #ifndef PRODUCTION
            printf("%i,%s,%s,%s,%li,%s\n", id, name, subject, comment, created_time, parent_str);
            #endif

            if(streq(parent_str, DATABASE_DELIM_EMPTY)) {
                global_id = max(global_id, id);
                if(post_create(id, name, subject, comment, delete_passwd, created_time, NULL) == NULL) {
                    printf("Failed to create post %i\n", id);
                    goto end;
                }
                loaded++;
            } else {
                unsigned int parent_id = atoi(parent_str);
                struct post *parent;
                if((parent = post_list_find(curr_post_list, parent_id)) != NULL) {
                    global_id = max(global_id, id);
                    if(post_create(id, name, subject, comment, delete_passwd, created_time, parent) == NULL) {
                        printf("Failed to create post %i\n", id);
                     goto end;
                    }
                    loaded++;
                } else {
                    printf("Ignoring #%i (no parent id)...\n", id);
                }
            }
        }
        if(loaded > 0)
            global_id++;
        free(line);
        fclose(f);
    }

    // Setup thread params
    db_thread_params = malloc(sizeof(struct db_thread_params));
    memset(db_thread_params, 0, sizeof(struct db_thread_params));
    db_thread_params->curr_post_list = curr_post_list;
    db_thread_params->should_save = 1;
    // Initialise the thread
    if(pthread_create(&db_thread, NULL, db_thread_main, (void*)db_thread_params) < 0) {
        printf("Failed to load database thread!\n");
        post_list_destroy(curr_post_list);
        goto end;
    }

    return;

end:
    fclose(f);
    exit(1);
}

// Cleanup functions
static void cleanup_database() {
    pthread_cancel(db_thread);
    post_list_destroy(curr_post_list);
}

static void cleanup_sockets() {
    if(sockfd > 0 && shutdown(sockfd, 0) < 0)
        printf("WARNING! Failed to close server socket: %s\n", strerror(errno));
}

// Signal handlers
static void cleanup(__attribute__((unused)) const int sig) {
    printf("Cleaning up...\n");
    cleanup_sockets();
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
                cleanup_database();
                exit(1);
            }
        }
    }

    // Initialize database
    init_database();

    // Initialize RNG for delete posts password
    srand((unsigned) time(NULL));

    // Initialize TCP sockets
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        printf("Cannot open socket: %s\n", strerror(errno));
        cleanup_database();
        exit(errno);
    }
    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0 ||
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int))    < 0
    ) {
        printf("Cannot set socket opts: %s\n", strerror(errno));
        close(sockfd);
        cleanup_database();
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
        cleanup_database();
        exit(errno);
    }
    if(listen(sockfd, MAXPENDING) < 0) {
        printf("Failed to listen: %s\n", strerror(errno));
        close(sockfd);
        cleanup_database();
        exit(errno);
    }
    printf("Listening on port %i\n", port);

    // Event loop
    fd_set readfds;

    struct timeval select_timeout;
    memset(&select_timeout, 0, sizeof(struct timeval));

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int activity = select(FD_SETSIZE, &readfds, NULL, NULL, &select_timeout);

        if (activity < 0 && errno != EINTR)
            continue;

        if(FD_ISSET(sockfd, &readfds)) {
            unsigned int clientlen = sizeof(client);
            int clientfd;
            if((clientfd = accept(sockfd, (struct sockaddr *)&client, &clientlen)) < 0) {
                if(errno != MSG_DONTWAIT)
                    printf("Failed to accept client connection: %s\n", strerror(errno));
                continue;
            }
            printf("Client connected: %s\n", inet_ntoa(client.sin_addr));
            handle(clientfd);
         }

         usleep(LOOP_SLEEP);
    }

    // Ended
    cleanup_sockets();
    cleanup_database();
    return 0;
}
