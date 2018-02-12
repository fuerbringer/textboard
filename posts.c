#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "posts.h"
#include "helpers.h"
#include "static.h"

#define RENDER_POST(x, length, element) \
    char time_utc[TIME_LENGTH]; \
    memset(time_utc, 0, TIME_LENGTH); \
    strftime(time_utc, TIME_LENGTH, "%F %T", gmtime(&post->created_time)); \
\
    char time[TIME_LENGTH]; \
    memset(time, 0, TIME_LENGTH); \
    strftime(time, TIME_LENGTH, "%F %T", localtime(&post->created_time)); \
\
    snprintf(x, length, element, \
            post->subject, post->author, time_utc, time, \
            post->id, post->id, post->comment);

#define GUESS_POST_RENDER_LENGTH(element) /* generous guesstimate */ \
    (strlen(element) + \
     strlen(post->author) + \
     strlen(post->subject) + \
     strlen(post->comment) + \
     TIME_LENGTH * 2)

// Post list
struct post_list *post_list_create() {
    struct post_list *list = malloc(sizeof(struct post_list));
    list->first = NULL;
    list->last = NULL;
    list->length = 0;
    return list;
}

void post_list_destroy(struct post_list *list) {
    struct post *curr = list->first, *next;
    while(curr != NULL) {
        next = curr->next;
        post_destroy(curr);
        curr = next;
    }
    free(list);
}

void post_list_prepend(struct post_list *list, struct post *post) {
    // Used only for frontpage/catalog list
    if(list->first == NULL) {
        printf("NUL?\n");
        list->first = post;
        list->last = post;
    } else {
        post->next = list->first;
        list->first->prev = post;
        list->first = post;
    }
    
    list->length++;
    
    // Remove from queue once big enough
    while(list->length > MAX_POSTS) {
        struct post *last = list->last->prev;
        post_destroy(list->last);
        last->next = NULL;
        list->last = last;
    }
}

void post_list_append(struct post_list *list, struct post *post) {
    // Used for comment replies
    if(list->first == NULL) {
        list->first = post;
        list->last = post;
    } else {
        post->prev = list->last;
        list->last->next = post;
        list->last = post;
    }
    
    list->length++;
}

char *post_list_render(struct post_list *list, const int is_reply) {
    printf("LEN: %li\n", list->length);
    
    char *element;
    if(is_reply) {
        element = POST_REPLY_ELEMENT;
    } else {
        element = POST_ELEMENT;
    }
    
    // guestimate the html size
    size_t list_length = 0;
    struct post *post = list->first;
    while(post != NULL) {
        list_length += GUESS_POST_RENDER_LENGTH(element);
        post = post->next;
    }
    if(list_length == 0) {
        return NULL;
    }
    
    char *html = malloc(list_length + 1);
    memset(html, 0, list_length+1);
    post = list->first;
    while(post != NULL) {
        const size_t length = GUESS_POST_RENDER_LENGTH(element);
        
        char *tmp = malloc(length);
        memset(tmp, 0, length);
        RENDER_POST(tmp, length, element)
        strcat(html, tmp);
        free(tmp);
        
        post = post->next;
    }
    
    return html;
}

struct post *post_list_find(struct post_list *list, int id) {
    struct post *post = list->first;
    while(post != NULL) {
        if(post->id == id)
            return post;
        post = post->next;
    }
    return NULL;
}

void post_list_debug(struct post_list *list) {
    struct post *curr = list->first;
    while(curr != NULL) {
        post_debug(curr);
        curr = curr->next;
    }
}

// Post
struct post *post_create(const char *author, const char *subject, const char *comment, struct post *parent) {
    struct post *post = malloc(sizeof(struct post));
    post->id = global_id++;
    post->author = encode_html(author);
    post->subject = encode_html(subject);
    post->comment = encode_html(comment);
    post->created_time = time(NULL);
    post->saved = 0;
    post->replies = post_list_create();
    post->prev = NULL;
    post->next = NULL;
    
    if(parent == NULL) {
        post->parent = NULL;
        post_list_prepend(curr_post_list, post);
    } else {
        post->parent = parent;
        post_list_append(parent->replies, post);
    }
    
    post_debug(post);
    return post;
}

void post_destroy(struct post *post) {
    free(post->author);
    free(post->subject);
    free(post->comment);
    post_list_destroy(post->replies);
    if(post->parent == NULL) {
        curr_post_list->length--;
    } else {
        post->parent->replies->length--;
    }
    
    if(post->next != NULL)
        post->next->prev = post->prev;
    if(post->prev != NULL)
        post->prev->next = post->next;
    
    free(post);
}

char *post_render(struct post *post) {
    const size_t length = GUESS_POST_RENDER_LENGTH(POST_ELEMENT);
    char *html = malloc(length);
    memset(html, 0, length);
    RENDER_POST(html, length, POST_ELEMENT)
    return html;
}

void post_debug(struct post *post) {
    printf("Post(%s, %s, %s, %p, %p)\n",post->author,post->subject,post->comment,post->prev,post->next);
}
