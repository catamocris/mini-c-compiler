#include <stdio.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main(int argc, char** argv){

    if(argc != 2) err("not enough arguments: %s <input_file>", argv[0]);

    char *buffer = loadFile(argv[1]);

    Token* tokens = tokenize(buffer);

    //showTokens(tokens);

    parse(tokens);
    printf("tot ok\n");

    free(buffer);
    return 0;
}