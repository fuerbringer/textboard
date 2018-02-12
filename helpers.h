#pragma once

#include <math.h>
#define startswith(x,y) (strncmp(x,y,strlen(y)) == 0)
#define streq(x,y) (strcmp(x,y) == 0)
#define sendstr(sockfd,str) send(sockfd,str,strlen(str), 0)
#define digits(x) (floor(log10(abs(x))) + 1)

char *clone_str(const char *x);
void decode_uri(char *dest, const char *src);
char *encode_html(const char *src);
