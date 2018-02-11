#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#include "helpers.h"
#include "static.h"
#include "posts.h"

#define MAXPENDING 5
#define BUFFSIZE 1024
#define PORT 8080

// need this header for *gasps* ie6 compatibility
#define CNT_TEXT_HEADER "Content-Type: text/plain\n"

// Definitions
int sockfd = -1;
int clientfd = -1;
int exiting = 0;

// Handler
void handle(const int sockfd) {
    char buffer[BUFFSIZE];
    int received = -1;
    if ((received = recv(sockfd, buffer, BUFFSIZE, 0)) < 0) {
        printf("Failed to receive\n");
        goto end;
    }
    printf("---\n%s\n:::\n", buffer);
    
    // copy body
    char *body = NULL;
    for(size_t i = 0; i < strlen(buffer); i++) {
        const char
            c1 = buffer[i],
            c2 = i+1<strlen(buffer) ? buffer[i+1] : 0,
            c3 = i+2<strlen(buffer) ? buffer[i+2] : 0,
            c4 = i+3<strlen(buffer) ? buffer[i+3] : 0;
        if(c1 == '\r' && c2 == '\n' && c3 == '\r' && c4 == '\n') {
            size_t length = strlen(buffer) - i - 4;
            body = malloc(length + 1);
            strncpy(body, buffer+i+4, length);
            body[length] = 0;
            break;
        }
    }
    
    // parse headers
    // initial line
    char *_saveptr;
    char *method = strtok_r(buffer, " ", &_saveptr);
    char *path = strtok_r(NULL, " ", &_saveptr);
    strtok_r(NULL, "\n", &_saveptr); // skip rest of initial line
    // header lines
    int content_length = -1;
    for(char *line = strtok_r(NULL, "\n", &_saveptr);
        line != NULL && !streq(line, "");
        line = strtok_r(NULL, "\n", &_saveptr)) {
        
        char *_line = clone_str(line);
        
        char *_saveptr1;
        const char *key = strtok_r(_line, ":", &_saveptr1);
        const char *value = _line + strlen(key) + 2; // skip : and space
        
        if(streq(key, "Content-Length"))
            content_length = atoi(value);
        
        free(_line);
    }

    #define DATE_HEADER_LINE \
        char date[TIME_LENGTH]; \
        time_t _time = time(NULL); \
        strftime(date, TIME_LENGTH, "Date: %a, %d %b %Y %T GMT", gmtime(&_time));

    #define malloc_or_fail(type, left, length, cleanup) \
        type left = malloc(length); \
        if(left == NULL) { \
            sendstr(sockfd, \
"HTTP/1.1 200 OK\n" \
CNT_TEXT_HEADER \
"\n" \
"malloc failed (check stdout)"); \
            printf("ERROR: malloc failed at %s:%i\n", __FILE__, __LINE__); \
            cleanup \
            goto end; \
        }

    // response
    // /
    if(streq(method, "GET") && streq(path, "/")) {
        DATE_HEADER_LINE
        
        // posts html
        char *html = post_list_render(curr_post_list, 0);
        
        const size_t length =
            strlen(RESPONSE_HEADER) + // HTML/1.1 (...)
            digits( // Content-Length: ??
                strlen(INDEX_FILE_HEADER) +
                (html == NULL ? 0 : strlen(html)) +
                strlen(FOOTER_FILE)
            ) +
            1 + // '\n'
            strlen(date) + // Date: [date]
            2 + // '\n\n'
            strlen(INDEX_FILE_HEADER) + // index file
            (html == NULL ? 0 : strlen(html)) + // posts
            strlen(FOOTER_FILE) +
            2; // '\0'
        
        malloc_or_fail(char *, response, length, {
            if(html != NULL)
                free(html);
        });
        memset(response, 0, length);
        snprintf(response, length,
            RESPONSE_HEADER "%li\n%s\n\n" INDEX_FILE_HEADER "%s" FOOTER_FILE,
            length,
            date,
            html == NULL ? "" : html);
        printf("RESPONSE\n%s", response);
        
        sendstr(sockfd, response);
        
        if(html != NULL)
            free(html);
        free(response);
    }
    // GET /post
    else if(streq(method, "GET") && streq(path, "/post")) {
        sendstr(sockfd,
"HTTP/1.1 200 OK\n"
"Content-Type: text/html\n"
"\n"
"<h1>Usage:</h1>\n"
"<p>POST /post HTTP/1.1</p>"
"<p>with content: name=[name]&subject=[subject]&comment=[comment]</p>");
    }
    // GET /post/*
    else if(streq(method, "GET") && startswith(path, "/post/")) {
        
        char *post_id_str = strrchr(path, '/')+1;
        unsigned int post_id = (unsigned int)strtol(post_id_str, NULL, 10);
        struct post *post;
        
        if((post = post_list_find(curr_post_list, post_id)) != NULL) {
            
            char *html = post_render(post);
            char *replies_html = post_list_render(post->replies, 1);
            
            DATE_HEADER_LINE
        
            const size_t length =
                strlen(RESPONSE_HEADER) + // HTML/1.1 (...)
                digits( // Content-Length: ??
                    strlen(INDEX_FILE_HEADER) +
                    strlen(html) +
                    (replies_html == NULL ? 0 : strlen(replies_html)) +
                    strlen(FOOTER_FILE)
                ) +
                1 + // \n
                strlen(date) + // Date: [date]
                2 + // '\n\n'
                strlen(REPLY_FILE_HEADER) + // index file
                strlen(post_id_str) + // for form action
                strlen(html) + // post
                (replies_html == NULL ? 0 : strlen(replies_html)) +
                strlen(FOOTER_FILE) +
                2; // '\0'
        
            malloc_or_fail(char *, response, length, {
                free(html);
                if(replies_html != NULL)
                   free(replies_html);
            });
            memset(response, 0, length);
            snprintf(response, length,
                RESPONSE_HEADER "%li\n%s\n\n" REPLY_FILE_HEADER "%s%s" FOOTER_FILE,
                length,
                date,
                post_id_str,
                html,
                (replies_html == NULL ? "" : replies_html));
            printf("RESPONSE\n%s", response);
            
            sendstr(sockfd, response);
            
            free(html);
            if(replies_html != NULL)
                free(replies_html);
            free(response);
            
        } else {
            sendstr(sockfd, 
"HTTP/1.1 404 Not Found\n"
CNT_TEXT_HEADER
"\n"
"there is no post by this id! ;n;");
        }
        
    }
    // POST /post
    else if(streq(method, "POST") && streq(path, "/post")) {
        if(content_length > -1) {
            if(content_length <= strlen(body)) // prevents overflow
                body[content_length] = 0;
            
            char *name = NULL;
            char *subject = NULL;
            char *comment = NULL;
            char *reply_to = NULL;
            
            #define cleanup_fields() { \
                free(name); \
                free(subject); \
                free(comment); \
                if(reply_to != NULL) free(reply_to); \
            }
            
            for(char *pair = strtok_r(body, "&", &_saveptr);
                pair != NULL;
                pair = strtok_r(NULL, "&", &_saveptr)) {
                
                char *_pair = clone_str(pair);
                const size_t pair_length = strlen(_pair);
                printf("PAIR: %s\n", _pair);
                
                char *_saveptr1;
                char *key = strtok_r(_pair, "=", &_saveptr1);
                
                const size_t value_length = pair_length - strlen(key);
                char *_value = malloc(value_length + 1);
                strncpy(_value, _pair + strlen(key) + 1, value_length);
                _value[value_length] = 0;
                
                char *value = malloc(value_length + 1);
                memset(value, 0, value_length);
                decode_uri(value, _value);
                free(_value);
                printf("%s, %s\n", key, value);
                
                if(streq(key, "name"))
                    name = clone_str(value);
                else if(streq(key, "subject"))
                    subject = clone_str(value);
                else if(streq(key, "comment"))
                    comment = clone_str(value);
                else if(streq(key, "reply_to"))
                    reply_to = clone_str(value);
                
                free(value);
                free(_pair);
            }
            
            free(body);

            if(name == NULL || !strlen(name)) {
                if(!strlen(name)) free(name);
                name = clone_str("anonymous");
            }
            if(comment == NULL) {
                sendstr(sockfd, 
"HTTP/1.1 200 OK\n"
CNT_TEXT_HEADER
"\n"
"there should be a comment field here! >_<");
                cleanup_fields();
                goto end;
            } else if(!strlen(comment)) {
                printf("%s\n",comment);
                sendstr(sockfd,
"HTTP/1.1 200 OK\n"
CNT_TEXT_HEADER
"\n"
"comment field shouldn't be empty! >~<");
                cleanup_fields();
                goto end;
            }
            
            printf("!!!\nName: %s\nSubject: %s\nComment: %s\n!!!\n", name, subject, comment);
            // Put it in the database
            if(reply_to == NULL)
                post_create(name, subject, comment, NULL);
            else {
                unsigned int reply_to_id = (unsigned int)strtol(reply_to, NULL, 10);
                struct post *parent;
                if((parent = post_list_find(curr_post_list, reply_to_id)) != NULL) {
                    post_create(name, subject, comment, parent);
                } else {
                    sendstr(sockfd,
"HTTP/1.1 404 Not Found\n"
CNT_TEXT_HEADER
"\n"
"there is no post by this id! ;n;");
                }
            }
            
            cleanup_fields();
            
            sendstr(sockfd,
"HTTP/1.1 200 OK\n"
"Content-Type: text/html\n"
"\n"
"<meta http-equiv=refresh content='1; url=/' />"
"OK! owo<br>"
"<p><a href=/>&#8810; Back</a></p>");
            
        } else {
            sendstr(sockfd,
"HTTP/1.1 200 OK\n"
CNT_TEXT_HEADER
"\n"
"can't post without arguments! ;n;");
        }
    }
    
    printf("---\n");

end:
    close(sockfd);
}

// Database actions
void init_database() {
    global_id = 0;
    curr_post_list = post_list_create();
}

void cleanup_database() {
    post_list_destroy(curr_post_list);
}

// Signal handlers
void cleanup(const int sig) {
    printf("Cleaning up...\n");
    if(sockfd > 0 && shutdown(sockfd, 0) < 0)
        printf("WARNING! Failed to close server socket; %i\n", errno);
    cleanup_database();
    exit(0);
}

// Main
int main() {
    // Initialize signals
    signal(SIGINT, cleanup);
    signal(SIGKILL, cleanup);
    
    // Initialize database
    init_database();
    
    // Initialize TCP sockets
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if(sockfd < 0) {
        printf("Cannot open socket\n");
        exit(1);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        printf("Cannot set socket opts\n");
        close(sockfd);
        exit(1);
    }
    struct sockaddr_in server, client;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Failed to bind! %i\n", errno);
        close(sockfd);
        exit(1);
    }
    if(listen(sockfd, MAXPENDING) < 0) {
        printf("Failed to listen!\n");
        close(sockfd);
        exit(1);
    }
    printf("Listening on port %i\n", PORT);
    
    // Event loop
    while(1) {
        unsigned int clientlen = sizeof(client);
        if((clientfd = accept(sockfd, (struct sockaddr *)&client, &clientlen)) < 0) {
            printf("Failed to accept client connection\n");
            break;
        }
        printf("Client connected: %s\n", inet_ntoa(client.sin_addr));
        handle(clientfd);
    }
    
    // Fin
    if(sockfd > 0 && shutdown(sockfd, 0) < 0)
        printf("WARNING! Failed to close server socket; %i\n", errno);
    cleanup_database();
    return 0;
}
