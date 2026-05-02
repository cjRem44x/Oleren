#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "expr.h"

#define BUF_SIZE 1024

#define INSTRUC_CHAR '|'
#define LABEL_CHAR   '@'
#define COMMENT_CHAR '#'

void proc_tokens(const char * t)
{
    printf("TOKEN: %s\n", t);
}

int main(int argc, char ** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    FILE * f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    char buf[BUF_SIZE];
    size_t idx = 0;

    int c;
    while ( (c = fgetc(f)) != EOF )
    {
        if (isspace(c)) continue;

        if (c == INSTRUC_CHAR) {
            buf[idx] = '\0';
            if (idx > 0 ) {
                proc_tokens(buf);
                idx = 0;
            }
            continue;
        }

        if (idx < BUF_SIZE - 1) {
            buf[idx++] = (char)c;
        } else {
            fprintf(stderr, "Token too LONG\n");
            idx = 0;
        }
    }

    if (idx > 0) {
        buf[idx] = '\0';
        proc_tokens(buf);
    }
    fclose(f);
    return 0;
}
