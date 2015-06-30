
#include "snakebed.h"
#include "lexer.h"

/*
 * ref: https://docs.python.org/2/reference/lexical_analysis.html
 */

struct _SbLexer {
    /* Character buffer. */
    SbLexerChar buffer[3];

    /* How the lexer fetches chars */
    SbFetchProc fetcher;
    void *fetcher_context;

    /* Keep track of where we are */
    Sb_size_t line;
    Sb_size_t column;

    /* True if the next token is the first one */
    int at_line_start;

    /* Keep track of indentation levels */
    Sb_size_t *indent_stack;
    Sb_size_t indent_index;
    Sb_size_t indent_stack_size;

    char *token_buffer;
    Sb_size_t token_length;
    Sb_size_t token_maxlength;
};


STATIC int
indent_push(SbLexer * const lexer, Sb_size_t value)
{
    lexer->indent_index += 1;
    if (lexer->indent_index >= lexer->indent_stack_size) {
        Sb_size_t new_stack_size = lexer->indent_stack_size + 32;
        Sb_size_t *new_stack = (Sb_size_t *)Sb_Realloc(lexer->indent_stack, new_stack_size * sizeof(Sb_size_t));
        if (!new_stack) {
            /* raise MemoryError: OOM */
            return -1;
        }
        lexer->indent_stack = new_stack;
        lexer->indent_stack_size = new_stack_size;
    }

    lexer->indent_stack[lexer->indent_index] = value;
    return 0;
}

/*
 * Character class tests
 */

static int
is_endofinput(SbLexerChar ch)
{
    return ch == SB_LEXER_EOI;
}

static int
is_newline(SbLexerChar ch)
{
    return ch == '\n';
}

static int
is_whitespace(SbLexerChar ch)
{
    return ch == ' ' || ch == '\t';
}

static int
is_lcalpha(SbLexerChar ch)
{
    return (unsigned)(ch - 'a') < 26U;
}

static int
is_ucalpha(SbLexerChar ch)
{
    return (unsigned)(ch - 'A') < 26U;
}

static int
is_alpha(SbLexerChar ch)
{
    return (unsigned)((ch | 0x20) - 'a') < 26U;
}

static int
is_digit(SbLexerChar ch)
{
    return (unsigned)(ch - '0') < 10U;
}

static int
is_bdigit(SbLexerChar ch)
{
    return (unsigned)(ch - '0') < 2U;
}

static int
is_odigit(SbLexerChar ch)
{
    return (unsigned)(ch - '0') < 8U;
}

static int
is_xdigit(SbLexerChar ch)
{
    int r;
    r = (unsigned)(ch - '0') < 10U;
    if (r)
        return r;
    return (unsigned)((ch | 0x20) - 'a') < 6U;
}

STATIC SbLexerChar
next_char(SbLexer * const lexer)
{
    SbLexerChar ch;

    if (is_endofinput(lexer->buffer[0]))
        return SB_LEXER_EOI;

    if (lexer->buffer[0] == '\n') {
        lexer->line += 1;
        lexer->column = 1;
    }
    /* TODO: Handle tabs properly?
    else if (lexer->buffer[0] == '\t') {
    }
    */
    else {
        lexer->column += 1;
    }

    ch = lexer->buffer[1];
    lexer->buffer[1] = lexer->buffer[2];
    lexer->buffer[2] = lexer->fetcher(lexer->fetcher_context);

    /* Normalise line endings to be a single \n */
    if (ch == '\r') {
        if (lexer->buffer[1] == '\n') {
            lexer->buffer[1] = lexer->buffer[2];
            lexer->buffer[2] = lexer->fetcher(lexer->fetcher_context);
        }
        ch = '\n';
    }

    lexer->buffer[0] = ch;
    return ch;
}

/*
 * Lexer methods
 */

STATIC int
SbLexer_ExpandToken(SbLexer * const lexer)
{
    if (lexer->token_length >= lexer->token_maxlength) {
        char *new_buffer;
        Sb_size_t new_length;
        
        new_length = lexer->token_maxlength + 64;
        new_buffer = (char *)Sb_Realloc(lexer->token_buffer, new_length);
        if (!new_buffer) {
            /* raise MemoryError: OOM. */
            return -1;
        }

        lexer->token_buffer = new_buffer;
        lexer->token_maxlength = new_length;
    }
    return 0;
}

STATIC int
SbLexer_AppendChar(SbLexer * const lexer, SbLexerChar ch)
{
    if (SbLexer_ExpandToken(lexer) < 0)
        return -1;

    lexer->token_buffer[lexer->token_length++] = (char)ch;
    return 0;
}

STATIC int
SbLexer_NullTerminate(SbLexer * const lexer)
{
    if (SbLexer_ExpandToken(lexer) < 0)
        return -1;

    lexer->token_buffer[lexer->token_length] = '\0';
    return 0;
}

typedef struct {
    const char *text;
    SbTokenType token;
} SbLexerKeyword;

STATIC const SbLexerKeyword SbLexer_KeywordTab[] = {
    { "and", TOKEN_KW_AND },
    { "as", TOKEN_KW_AS },
    { "assert", TOKEN_KW_ASSERT },
    { "break", TOKEN_KW_BREAK },
    { "class", TOKEN_KW_CLASS },
    { "continue", TOKEN_KW_CONTINUE },
    { "def", TOKEN_KW_DEF },
    { "del", TOKEN_KW_DEL },
    { "elif", TOKEN_KW_ELIF },
    { "else", TOKEN_KW_ELSE },
    { "except", TOKEN_KW_EXCEPT },
    /* { "exec", TOKEN_KW_EXEC }, */ 
    { "finally", TOKEN_KW_FINALLY },
    { "for", TOKEN_KW_FOR },
    { "from", TOKEN_KW_FROM },
    { "global", TOKEN_KW_GLOBAL },
    { "if", TOKEN_KW_IF },
    { "import", TOKEN_KW_IMPORT },
    { "in", TOKEN_KW_IN },
    { "is", TOKEN_KW_IS },
    /* { "lambda", TOKEN_KW_LAMBDA }, */
    { "not", TOKEN_KW_NOT },
    { "or", TOKEN_KW_OR },
    { "pass", TOKEN_KW_PASS },
    { "print", TOKEN_KW_PRINT },
    { "raise", TOKEN_KW_RAISE },
    { "return", TOKEN_KW_RETURN },
    { "try", TOKEN_KW_TRY },
    { "while", TOKEN_KW_WHILE },
    { "with", TOKEN_KW_WITH },
    { "yield", TOKEN_KW_YIELD },
    /* Sentinel */
    { NULL },
};

typedef struct {
    char op;
    char token1; /* No training = */
    char token2; /* With trailing = */
    char spare;
} SbLexerOpTabEntry;

/* Consist of always a single character */
STATIC const SbLexerOpTabEntry SbLexer_OpTab1[] = {
    { '(', TOKEN_DLM_LPAREN, },
    { ')', TOKEN_DLM_RPAREN, },
    { '[', TOKEN_DLM_LBRACKET, },
    { ']', TOKEN_DLM_RBRACKET, },
    { '{', TOKEN_DLM_LBRACE, },
    { '}', TOKEN_DLM_RBRACE, },
    { '@', TOKEN_DLM_AT, },
    { ',', TOKEN_DLM_COMMA, },
    { ':', TOKEN_DLM_COLON, },
    { ';', TOKEN_DLM_SEMICOLON, },
    { '`', TOKEN_DLM_BACKTICK, },
    /* Sentinel */
    { '\0', },
};

/* Consist of 2 or 3 characters, with 3rd being '=' */
STATIC const SbLexerOpTabEntry SbLexer_OpTab2[] = {
    { '*', TOKEN_OP_PWR, TOKEN_DLM_PWR_ASSIGN, },
    { '/', TOKEN_OP_FLDIV, TOKEN_DLM_FLDIV_ASSIGN, },
    { '<', TOKEN_OP_SHL, TOKEN_DLM_SHL_ASSIGN, },
    { '>', TOKEN_OP_SHR, TOKEN_DLM_SHR_ASSIGN, },
    /* Sentinel */
    { '\0', },
};

/* Consist of 1 or 2 characters, with 2nd being '=' */
STATIC const SbLexerOpTabEntry SbLexer_OpTab3[] = {
    { '+', TOKEN_OP_PLUS, TOKEN_DLM_PLUS_ASSIGN, },
    { '-', TOKEN_OP_MINUS, TOKEN_DLM_MINUS_ASSIGN, },
    { '*', TOKEN_OP_MUL, TOKEN_DLM_MUL_ASSIGN, },
    { '/', TOKEN_OP_DIV, TOKEN_DLM_DIV_ASSIGN, },
    { '%', TOKEN_OP_MOD, TOKEN_DLM_MOD_ASSIGN, },
    { '&', TOKEN_OP_AND, TOKEN_DLM_AND_ASSIGN, },
    { '|', TOKEN_OP_OR, TOKEN_DLM_OR_ASSIGN, },
    { '^', TOKEN_OP_XOR, TOKEN_DLM_XOR_ASSIGN, },
    { '<', TOKEN_OP_LT, TOKEN_OP_LE, },
    { '>', TOKEN_OP_GT, TOKEN_OP_GE, },
    { '=', TOKEN_DLM_ASSIGN, TOKEN_OP_EQ, },
    /* Sentinel */
    { '\0', },
};

int
SbLexer_NextToken(SbLexer * const lexer, SbToken * const token)
{
    SbLexerChar ch;

    /* Handle whitespace and comments */
    ch = lexer->buffer[0];
    for (;;) {
        if (is_endofinput(ch))
            break;

        /* Consume whitespace */
        if (is_whitespace(ch)) {
            ch = next_char(lexer);
            continue;
        }

        /* Consume comments */
        if (ch == '#') {
            while (!is_endofinput(ch) && !is_newline(ch)) {
                ch = next_char(lexer);
           }
            continue;
        }

        /* Check for explicit line joining. */
        if (ch == '\\') {
            if (!is_newline(lexer->buffer[1])) {
                /* raise SyntaxError: backslash not at EOL */
                return -1;
            }
            next_char(lexer);
            ch = next_char(lexer);
            continue;
        }

        /* A logical line that contains only spaces, tabs, formfeeds 
           and possibly a comment, is ignored */
        if (is_newline(ch) && lexer->at_line_start) {
            ch = next_char(lexer);
            continue;
        }

        /* Not a whitespace/comment. */
        break;
    }

    token->line = lexer->line;
    token->column = lexer->column;

    /* Handle indentation */
    if (lexer->at_line_start) {
        Sb_size_t token_indent = lexer->column;
        Sb_size_t top_indent = lexer->indent_stack[lexer->indent_index];
        /* If it is larger, it is pushed on the stack,
           and one INDENT token is generated. */
        if (token_indent > top_indent) {
            indent_push(lexer, token->column);
            token->type = TOKEN_INDENT;
            return 1;
        }

        /* If it is smaller, it must be one of the numbers occurring on the stack; 
           all numbers on the stack that are larger are popped off, 
           and for each number popped off a DEDENT token is generated. */
        if (token_indent < top_indent) {
            lexer->indent_index--;
            if (lexer->indent_stack[lexer->indent_index] > token_indent) {
                /* raise SyntaxError: inconsistent dedent */
                return -1;
            }

            token->type = TOKEN_DEDENT;
            return 1;
        }

        lexer->at_line_start = 0;
    }

    if (is_newline(ch)) {
        /* Explicit line joining was handled above. */
        /* TODO: handle implicit line joining */

        next_char(lexer);
        lexer->at_line_start = 1;
        token->type = TOKEN_NEWLINE;
        return 1;
    }

    lexer->token_length = 0;

    /* Handle string literals */
    /* NOTE: No plans for supporting Unicode. */
    if (ch == '"' || ch == '\'' || ((ch == 'r' || ch == 'R') && (ch == '"' || ch == '\''))) {
        int is_raw = 0;
        int is_triple_q = 0;
        SbLexerChar quote_ch;

        /* Check if a raw string; consume 'R' if so */
        if (ch == 'r' || ch == 'R') {
            is_raw = 1;
            ch = next_char(lexer);
        }

        /* Check if a triple-quoted string; consume quotes */
        if (lexer->buffer[1] == ch && lexer->buffer[2] == ch) {
            is_triple_q = 1;
            next_char(lexer);
            next_char(lexer);
        }
        quote_ch = ch;
        next_char(lexer);

        for (;;) {
            ch = lexer->buffer[0];

            /* Check matching quotes */
            if (ch == quote_ch && (!is_triple_q || (is_triple_q && lexer->buffer[1] == ch && lexer->buffer[2] == ch))) {
                if (is_triple_q) {
                    next_char(lexer);
                    next_char(lexer);
                }
                next_char(lexer);
                
                break;
            }

            if (!is_triple_q && is_newline(ch)) {
                /* raise SyntaxError: no matching quote at EOL */
                return -1;
            }

            if (is_endofinput(ch)) {
                /* raise SyntaxError: string is not closed before EOF */
                return -1;
            }

            /* Check for escapes */
            if (!is_raw && ch == '\\') {
                int recognised = 1;

                if (is_endofinput(lexer->buffer[1])) {
                    /* Let it hit EOI handling */
                    continue;
                }
                /* TODO: verify size/speed against a loop over a string */
                switch (lexer->buffer[1]) {
                case '\\':
                case '\'':
                case '"':
                    break;
                case 'a':
                    ch = '\x07';
                    break;
                case 'b':
                    ch = '\x08';
                    break;
                case 't':
                    ch = '\x09';
                    break;
                case 'n':
                    ch = '\x0a';
                    break;
                case 'v':
                    ch = '\x0b';
                    break;
                case 'f':
                    ch = '\x0c';
                    break;
                case 'r':
                    ch = '\x0d';
                    break;
                case 'x':
                case 'X':
                    /* TODO: implement */
                    next_char(lexer);
                    break;

                default:
                    if (is_odigit(lexer->buffer[1])) {
                        /* Treat as octal: up to 3 digits */
                        next_char(lexer);
                        ch = lexer->buffer[0] - '0';
                        if (is_odigit(lexer->buffer[1])) {
                            next_char(lexer);
                            ch |= (ch << 3) | (lexer->buffer[0] - '0');
                            if (is_odigit(lexer->buffer[1])) {
                                next_char(lexer);
                                ch |= (ch << 3) | (lexer->buffer[0] - '0');
                            }
                        }
                    }
                    else {
                        recognised = 0;
                    }
                }

                /* Consume backslash if the char is recognised. */
                if (recognised) {
                    next_char(lexer);
                }
            }

            /* Append character */
            if (SbLexer_AppendChar(lexer, ch) < 0)
                return -1;

            /* Move on */
            next_char(lexer);
        }

        token->type = TOKEN_STR;
        token->value.str.buffer = lexer->token_buffer;
        token->value.str.length = lexer->token_length;
        return 1;
    }

    /* Handle names */
    if (is_alpha(ch) || ch == '_') {
        do {
            if (SbLexer_AppendChar(lexer, ch) < 0)
                return -1;
            ch = next_char(lexer);
        } while (is_alpha(ch) || is_digit(ch) || ch == '_');
        /* Safe: no name may contain a NUL char */
        if (SbLexer_NullTerminate(lexer) < 0)
            return -1;

        /* Longest keyword is 8 chars */
        if (lexer->token_length <= 8) {
            const SbLexerKeyword *kw = SbLexer_KeywordTab;

            while (kw->text) {
                if (!Sb_StrCmp(kw->text, lexer->token_buffer)) {
                    token->type = kw->token;
                    return 1;
                }
                ++kw;
            }
        }

        token->type = TOKEN_NAME;
        token->value.str.buffer = lexer->token_buffer;
        token->value.str.length = lexer->token_length;
        return 1;
    }

    /* Handle numeric literals */
    /* NOTE: This is in no way a robust parser -- it allows broken literals! */
    if (is_digit(ch) || (ch == '.' && is_digit(lexer->buffer[1]))) {
        /* Either an integer or a float's intpart */
        while (is_digit(ch) || is_alpha(ch)) {
            if (SbLexer_AppendChar(lexer, ch) < 0)
                return -1;
            ch = next_char(lexer);
        }
        if (ch == '.') {
            if (SbLexer_AppendChar(lexer, ch) < 0)
                return -1;
            ch = next_char(lexer);
            while (is_digit(ch)) {
                if (SbLexer_AppendChar(lexer, ch) < 0)
                    return -1;
                ch = next_char(lexer);
            }
        }
        if (ch == 'e' || ch == 'E') {
            if (SbLexer_AppendChar(lexer, 'e') < 0)
                return -1;
            ch = next_char(lexer);
            if (ch == '+' || ch == '-') {
                if (SbLexer_AppendChar(lexer, ch) < 0)
                    return -1;
                ch = next_char(lexer);
            }
            while (is_digit(ch)) {
                if (SbLexer_AppendChar(lexer, ch) < 0)
                    return -1;
                ch = next_char(lexer);
            }
        }
        /* Safe: no numeric may contain a NUL char */
        if (SbLexer_NullTerminate(lexer) < 0)
            return -1;

        token->type = TOKEN_NUMBER;
        token->value.str.buffer = lexer->token_buffer;
        token->value.str.length = lexer->token_length;
        return 1;
    }

    /* Handle operators/delimiters */
    /* dot/ellipsis is special-cased. */
    if (ch == '.') {
        next_char(lexer);
        if (lexer->buffer[0] == '.' && lexer->buffer[1] == '.') {
            next_char(lexer);
            next_char(lexer);
            token->type = TOKEN_DLM_ELLIPSIS;
        }
        else {
            token->type = TOKEN_DLM_DOT;
        }
        return 1;
    }
    else {
        const SbLexerOpTabEntry *e;

        e = SbLexer_OpTab1;
        while (e->op != '\0') {
            if (ch == e->op) {
                next_char(lexer);
                /* TODO: cover bracket counting */
                token->type = e->token1;
                return 1;
            }
            ++e;
        }

        e = SbLexer_OpTab2;
        while (e->op != '\0') {
            if (ch == e->op && lexer->buffer[1] == e->op) {
                next_char(lexer);
                next_char(lexer);
                if (lexer->buffer[2] == '=') {
                    next_char(lexer);
                    token->type = e->token2;
                } else {
                    token->type = e->token1;
                }
                return 1;
            }
            ++e;
        }

        e = SbLexer_OpTab3;
        while (e->op != '\0') {
            if (ch == e->op && lexer->buffer[1] == e->op) {
                next_char(lexer);
                if (lexer->buffer[2] == '=') {
                    next_char(lexer);
                    token->type = e->token2;
                } else {
                    token->type = e->token1;
                }
                return 1;
            }
            ++e;
        }
    }

    if (is_endofinput(ch)) {
        /* TODO: Handle dedents at the EOI */
        token->type = TOKEN_ENDMARKER;
        return 1;
    }

    return 1;
}

SbLexer *
SbLexer_Init(SbFetchProc fetcher, void *context)
{
    SbLexer *lexer;

    lexer = Sb_Malloc(sizeof(SbLexer));
    if (!lexer) {
        return NULL;
    }

    lexer->fetcher = fetcher;
    lexer->fetcher_context = context;

    lexer->line = 1;
    lexer->column = 1;

    lexer->token_buffer = NULL;
    lexer->token_maxlength = 0;

    lexer->indent_stack = NULL;
    lexer->indent_stack_size = 0;
    lexer->indent_index = -1;
    indent_push(lexer, 0);

    lexer->at_line_start = 1;

    lexer->buffer[0] = lexer->fetcher(lexer->fetcher_context);
    lexer->buffer[1] = lexer->fetcher(lexer->fetcher_context);
    lexer->buffer[2] = lexer->fetcher(lexer->fetcher_context);

    return lexer;
}

typedef struct {
    const char *str;
} StringFetcherContext;

STATIC SbLexerChar
SbLexer_StringFetcher(void *context)
{
    StringFetcherContext *ctx = (StringFetcherContext *)context;
    char ch = *ctx->str;
    if (ch == '\0') {
        return SB_LEXER_EOI;
    }

    ctx->str++;
    return ch;
}

SbLexer *
SbLexer_InitFromStr(const char *str)
{
    StringFetcherContext *ctx;

    ctx = (StringFetcherContext *)Sb_Malloc(sizeof(*ctx));
    if (!ctx) {
        /* OOM */
        return NULL;
    }

    ctx->str = str;
    return SbLexer_Init(SbLexer_StringFetcher, ctx);
}

void
SbLexer_Free(SbLexer * const lexer)
{
    if (!lexer)
        return;

    Sb_Free(lexer->fetcher_context);
    Sb_Free(lexer->indent_stack);
    Sb_Free(lexer->token_buffer);
    Sb_Free(lexer);
}

