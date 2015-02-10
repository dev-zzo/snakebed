
#include "snakebed.h"
#include "lexer.h"

int main()
{
    SbToken tk;
    SbLexer *lexer;

    lexer = SbLexer_InitFromStr("def x(a):\n  pass\nx(0)");
    while (SbLexer_NextToken(lexer, &tk) > 0) {
        printf("Token %d at %d:%d\n", tk.type, tk.line, tk.column);
    }
    return 0;
}
