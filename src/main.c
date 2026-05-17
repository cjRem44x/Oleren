#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "olnas.h"

#define OLEREN_SRC_EXT "oln"   // SRC
#define OLEREN_ASM_EXT "olnas" // ASM SCRIPT

typedef enum CompilingType {
    UNDEF, SRC, ASM,
} CompilingType;

CompilingType compType;

// ... = check_ext(file, "ext") NOT DOT
int check_ext(const char * filePath, const char * ext)
{
    const char * dot = strrchr(filePath, '.');
    if (!dot || dot == filePath) return 0;

    dot++; // skip
    while (*dot && *ext)
    {
        if (tolower((unsigned char)*dot) != tolower((unsigned char)*ext)) {
            return 0;
        }
        dot++;
        ext++;
    }
    return *dot == '\0' && *ext == '\0';
}

int main(int argc, char ** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    if (check_ext(argv[1], OLEREN_SRC_EXT)) {
        compType = SRC;
    } else if (check_ext(argv[1], OLEREN_ASM_EXT)) {
        compType = ASM;
        begin_olnas(argv[1]);
    }
    printf("Compiling Type = %d\n", compType);
    return 0;
}
