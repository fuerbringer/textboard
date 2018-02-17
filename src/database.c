#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "posts.h"
#include "config.h"
#include "database.h"
#include "helpers.h"

static void save_post_to_file(FILE *f, struct post *post) {
    #define STR_VALUE_OR_EMPTY(x) ((x!=NULL&&strlen(x)) ? x : DATABASE_DELIM_EMPTY)

    char *parent_id = NULL;
    if(post->parent != NULL) {
        parent_id = malloc(digits(post->parent->id)+1);
        snprintf(parent_id, digits(post->parent->id)+1, "%i", post->parent->id);
    }
    
    fprintf(f, "%i,%s,%s,%s,%li,%s,%s,%i\n",
                    post->id, post->author,
                    STR_VALUE_OR_EMPTY(post->subject),
                    post->comment, post->created_time,
                    STR_VALUE_OR_EMPTY(parent_id),
                    post->delete_passwd,
                    post->deleted);
    if(parent_id != NULL) free(parent_id);
    post->saved = 1;
    
    if(post->deleted && post->saved)
        post_destroy(post);
}

void db_thread_save(FILE **db_file_ptr, struct post_list **curr_post_list_ptr) {
    FILE *db_file = *db_file_ptr;
    struct post_list *curr_post_list = *curr_post_list_ptr;
        
    struct post *post = curr_post_list->last;
    while(post != NULL) {
        if(!post->saved) save_post_to_file(db_file, post);

        struct post *reply = post->replies->first;
        while(reply != NULL) {
            if(!reply->saved) save_post_to_file(db_file, reply);
            reply = reply->next;
        }
            
        post = post->prev;
    }
    
    fflush(db_file);
}

static void db_thread_cleanup(void *db_thread_params_ptr) {
    struct db_thread_params *params = db_thread_params_ptr;
    if(pthread_mutex_lock(&params->db_lock) == 0) {
        printf("Saving database...\n");
        db_thread_save(&params->db_file, &params->curr_post_list);
        fclose(params->db_file);
        if(pthread_mutex_unlock(&params->db_lock) < 0)
            printf("Unable to release rwlock: %s\n", strerror(errno));
    } else
        printf("Unable to acquire lock: %s\n", strerror(errno));
}

void *db_thread_main(void *db_thread_params_ptr) {
    printf("Database thread loaded!\n");
    
    // Cleanup
    pthread_cleanup_push(db_thread_cleanup, db_thread_params_ptr);
    
    // Params
    struct db_thread_params *params = db_thread_params_ptr;
    struct post_list *curr_post_list = params->curr_post_list;
        
    // Load file
    params->db_file = fopen(DATABASE_FILE, "w+");
    if(params->db_file == NULL) {
        printf("Database init error: %s\n", strerror(errno));
        return NULL;
    }
    
    // Poll for updates
    int header_saved = 0;
    while(1) {
        if(pthread_mutex_lock(&params->db_lock) == 0) {
            // Save the database
            if(!header_saved) {
                fwrite(DATABASE_HEADER, 1, strlen(DATABASE_HEADER), params->db_file);
                header_saved = 1;
            }
            
            if(params->should_save) {
                db_thread_save(&params->db_file, &curr_post_list);
                params->should_save = 0;
                printf("Database saved.\n");
            }
            
            if(pthread_mutex_unlock(&params->db_lock) < 0)
                printf("Unable to release lock: %s\n", strerror(errno));
        } else
            printf("Unable to acquire lock: %s\n", strerror(errno));
        
        usleep(DATABASE_SLEEP);
    }
    
    fclose(params->db_file);

    pthread_cleanup_pop(0);
    
    return NULL;
}
