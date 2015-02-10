#ifndef __SNAKEBED_LEXER_H
#define __SNAKEBED_LEXER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int SbLexerChar;

#define SB_LEXER_EOI ((SbLexerChar)-1)

typedef SbLexerChar (*SbFetchProc)(void *context);

struct _SbLexer;
typedef struct _SbLexer SbLexer;

typedef enum {
    TOKEN_ERROR = -1,
    TOKEN_EOI,

    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NEWLINE,

    /* Keywords */
    TOKEN_KW_AND,
    TOKEN_KW_AS,
    TOKEN_KW_ASSERT,
    TOKEN_KW_BREAK,
    TOKEN_KW_CLASS,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_DEF,
    TOKEN_KW_DEL,
    TOKEN_KW_ELIF,
    TOKEN_KW_ELSE,
    TOKEN_KW_EXCEPT,
    TOKEN_KW_EXEC, /* NOTE: might drop */
    TOKEN_KW_FINALLY,
    TOKEN_KW_FOR,
    TOKEN_KW_FROM,
    TOKEN_KW_GLOBAL,
    TOKEN_KW_IF,
    TOKEN_KW_IMPORT,
    TOKEN_KW_IN,
    TOKEN_KW_IS,
    TOKEN_KW_LAMBDA, /* NOTE: might drop */
    TOKEN_KW_NOT,
    TOKEN_KW_OR,
    TOKEN_KW_PASS,
    TOKEN_KW_PRINT, /* NOTE: might drop */
    TOKEN_KW_RAISE,
    TOKEN_KW_RETURN,
    TOKEN_KW_TRY,
    TOKEN_KW_WHILE,
    TOKEN_KW_WITH,
    TOKEN_KW_YIELD,

    /* Literals */
    TOKEN_STR,
    TOKEN_NAME,
    TOKEN_INTEGER,
    TOKEN_LONGINT,
    TOKEN_FLOAT,

    /* Operators */
    TOKEN_OP_PLUS,
    TOKEN_OP_MINUS,
    TOKEN_OP_MUL,
    TOKEN_OP_PWR,
    TOKEN_OP_DIV,
    TOKEN_OP_FLDIV,
    TOKEN_OP_MOD,
    TOKEN_OP_SHL,
    TOKEN_OP_SHR,
    TOKEN_OP_AND,
    TOKEN_OP_OR,
    TOKEN_OP_XOR,
    TOKEN_OP_NOT,
    TOKEN_OP_LT,
    TOKEN_OP_GT,
    TOKEN_OP_LE,
    TOKEN_OP_GE,
    TOKEN_OP_EQ,
    TOKEN_OP_NE,
    
    /* Delimiters */
    TOKEN_DLM_LPAREN,
    TOKEN_DLM_RPAREN,
    TOKEN_DLM_LBRACKET,
    TOKEN_DLM_RBRACKET,
    TOKEN_DLM_LBRACE,
    TOKEN_DLM_RBRACE,
    TOKEN_DLM_AT,
    TOKEN_DLM_COMMA,
    TOKEN_DLM_COLON,
    TOKEN_DLM_DOT,
    TOKEN_DLM_ELLIPSIS,
    TOKEN_DLM_BACKTICK,
    TOKEN_DLM_ASSIGN,
    TOKEN_DLM_SEMICOLON,
    TOKEN_DLM_PLUS_ASSIGN,
    TOKEN_DLM_MINUS_ASSIGN,
    TOKEN_DLM_MUL_ASSIGN,
    TOKEN_DLM_PWR_ASSIGN,
    TOKEN_DLM_DIV_ASSIGN,
    TOKEN_DLM_FLDIV_ASSIGN,
    TOKEN_DLM_MOD_ASSIGN,
    TOKEN_DLM_SHL_ASSIGN,
    TOKEN_DLM_SHR_ASSIGN,
    TOKEN_DLM_AND_ASSIGN,
    TOKEN_DLM_OR_ASSIGN,
    TOKEN_DLM_XOR_ASSIGN,

} SbTokenType;

typedef struct {
    Sb_size_t line;
    Sb_size_t column;
    SbTokenType type;
    union {
        struct {
            char *buffer; /* String buffer (static) */
            Sb_size_t length;
        } str;
    } value;
} SbToken;


SbLexer *
SbLexer_Init(SbFetchProc fetcher, void *context);

SbLexer *
SbLexer_InitFromStr(const char *str);

void
SbLexer_Free(SbLexer * const lexer);

int
SbLexer_NextToken(SbLexer * const lexer, SbToken * const token);

#ifdef __cplusplus
}
#endif
#endif // __SNAKEBED_LEXER_H
