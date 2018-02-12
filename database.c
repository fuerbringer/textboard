#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "posts.h"
#include "config.h"

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
            fwrite(DATABASE_HEADER, 1, strlen(DATABASE_HEADER), db_file);
            header_saved = 1;
        }
        
        #define STR_VALUE_OR_EMPTY(x) (strlen(x) ? x : DATABASE_DELIM_EMPTY)
        
        struct post *post = curr_post_list->last;
        while(post != NULL) {
            if(!post->saved) {
                fprintf(db_file, "%i,%s,%s,%s,%li," DATABASE_DELIM_EMPTY "\n",
                    post->id, post->author,
                    STR_VALUE_OR_EMPTY(post->subject),
                    post->comment, post->created_time);
                post->saved = 1;
            }
            
            struct post *reply = post->replies->first;
            while(reply != NULL) {
                if(!reply->saved) {
                    fprintf(db_file, "%i,%s,%s,%s,%li,%i\n",
                        reply->id, reply->author,
                        STR_VALUE_OR_EMPTY(reply->subject),
                        reply->comment, reply->created_time,
                        post->id);
                    reply->saved = 1;
                }
                reply = reply->next;
            }
            
            post = post->prev;
        }
        
        fflush(db_file);
        //printf("Database saved.\n");
        
        // Sleep
        sleep(DATABASE_SLEEP);
    }
    fclose(db_file);
    return NULL;
}
