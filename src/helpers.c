#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HTML_ESCAPE_LENGTH 14

char *clone_str(const char *x) {
    char *str = malloc(strlen(x) + 1);
    strcpy(str, x);
    return str;
}

void decode_uri(char *dest, const char *src) {
    for(size_t i = 0, j = 0; src[i];) {
        if(src[i] == '%') {
            i++;
            const size_t prev = i;
            const char
                c1 = i < strlen(src) ? src[i++] : 0,
                c2 = i < strlen(src) ? src[i++] : 0;
            if(isxdigit(c1) && isxdigit(c2)) {
                
                const char hex[3] = { c1, c2, 0 };
                dest[j++] = strtol(hex, NULL, 16);
                
            } else {
                dest[j++] = '%';
                i = prev;
            }
        } else if(src[i] == '+') {
            dest[j++] = ' ';
            i++;
        } else {
            dest[j++] = src[i++];
        }
    }
}

char *encode_html(const char *src) {
    size_t length = strlen(src)+1; // +null terminator
    char *dest = malloc(length);
    if(dest == NULL) return NULL;
    memset(dest, 0, length);
    size_t i = 0, j = 0;
    while(src[i]) {
        if(src[i] > 127 || src[i] < 0) {
            free(dest);
            return NULL;
        }
        // don't escape these characters
        if(isalnum(src[i]) || // [A-Za-z0-9]
            (32 <= src[i] && src[i] <= 47 && src[i] != 44) || // [space] to / without ,
            // (makes converting from/to csv A LOT fucking easier
            (src[i] == 61 || src[i] == 63 || src[i] == 64) || // =, ?, @
            (123 <= src[i] && src[i] <= 126)) { // { to ~
            dest[j++] = src[i++];
        }
        // break line
        else if(src[i] == '\n') {
            length += strlen("<br>")-1; // replaces 1 character with 1 string
            dest = realloc(dest, length);
            dest[j++] = '<';
            dest[j++] = 'b';
            dest[j++] = 'r';
            dest[j++] = '>';
            i++;
        }
        // escape the others
        else {
            char num[HTML_ESCAPE_LENGTH];
            memset(num, 0, HTML_ESCAPE_LENGTH);
            snprintf(num, HTML_ESCAPE_LENGTH, "&#%i;", src[i++]);
            
            length += strlen(num)-1; // replaces 1 character with 1 string
            dest = realloc(dest, length);
            for(size_t x = 0; x < strlen(num); x++)
                dest[j+x] = num[x];
            j += strlen(num);
        }
    }
    dest[j] = 0;
    return dest;
}
