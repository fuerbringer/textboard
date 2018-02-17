#pragma once
#include <pthread.h>
#include <stdio.h>

struct db_thread_params {
    struct post_list *curr_post_list;
    int should_save;
    pthread_mutex_t db_lock;
    FILE *db_file;
};

void db_thread_save(FILE **db_file_ptr, struct post_list **curr_post_list_ptr);
void *db_thread_main(void *db_thread_params_ptr);
