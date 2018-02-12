#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "posts.h"

#define DATABASE_FILE "database.csv"
#define DATABASE_SLEEP 1

void *db_thread_main(void *curr_post_list_ptr) {
    printf("Database thread loaded!\n");
    
    FILE *db_file = fopen(DATABASE_FILE, "w+");
    if(db_file == NULL) {
        printf("Database init error: %s\n", strerror(errno));
        return NULL;
    }
    
    struct post_list *curr_post_list = (struct post_list *)curr_post_list_ptr;
    int header_saved = 0;
    while(1) {
        // Save the database
        if(!header_saved) {
            const char *header = "id,author,subject,comment,created_time,parent\n";
            fwrite(header, 1, strlen(header), db_file);
            header_saved = 1;
        }
        
        struct post *post = curr_post_list->first;
        while(post != NULL) {
            if(!post->saved) {
                fprintf(db_file, "%i,%s,%s,%s,%li,\n",
                    post->id, post->author, post->subject,
                    post->comment, post->created_time);
                post->saved = 1;
            }
            
            struct post *reply = post->replies->first;
            while(reply != NULL) {
                if(!reply->saved) {
                    fprintf(db_file, "%i,%s,%s,%s,%li,%i\n",
                        reply->id, reply->author, reply->subject,
                        reply->comment, reply->created_time,
                        post->id);
                    reply->saved = 1;
                }
                reply = reply->next;
            }
            
            post = post->next;
        }
        
        fflush(db_file);
        printf("Database saved.\n");
        
        // Sleep
        sleep(DATABASE_SLEEP);
    }
    fclose(db_file);
    return NULL;
}
