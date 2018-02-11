#pragma once

#include <time.h>
#include <stdlib.h>

#define MAX_POSTS 90

// Post list
struct post_list {
    size_t length;
    struct post *first, *last;
};

struct post_list *curr_post_list; // top level
struct post_list *full_post_list;

struct post_list *post_list_create();
void post_list_destroy(struct post_list *list);
void post_list_prepend(struct post_list *list, struct post *post);
void post_list_append(struct post_list *list, struct post *post);
char *post_list_render(struct post_list *list, const int is_reply);
struct post *post_list_find(struct post_list *list, int id);

// Post
struct post {
    unsigned int id;
    char *author;
    char *subject;
    char *comment;
    time_t created_time;
    struct post_list *replies;
    struct post *prev, *next;
    struct post *parent; // don't free me
};
unsigned int global_id;

struct post *post_create(const char *author, const char *subject, const char *comment, struct post *parent);
void post_destroy(struct post *post);
char *post_render(struct post *post);
