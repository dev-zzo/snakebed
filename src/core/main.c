
#include "snakebed.h"
#include "lexer.h"

int main()
{
    SbToken tk;
    SbLexer *lexer;

    lexer = SbLexer_InitFromStr("1.1e-13");
    while (SbLexer_NextToken(lexer, &tk) > 0) {
        printf("Token %d at %d:%d\n", tk.type, tk.line, tk.column);
        switch (tk.type) {
        case TOKEN_STR:
        case TOKEN_NAME:
        case TOKEN_NUMBER:
            printf("Value: %s\n", tk.value.str.buffer);
        }
    }
    return 0;
}
