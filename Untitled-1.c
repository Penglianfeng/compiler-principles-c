#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//编译运行实例
//gcc lexer.c -o lexer
//lexer sample.c
typedef enum {
    TK_IDENTIFIER, TK_KEYWORD, TK_INT_LITERAL, TK_HEX_LITERAL,
    TK_FLOAT_LITERAL, TK_STRING_LITERAL, TK_CHAR_LITERAL,
    TK_OPERATOR, TK_PUNCTUATION, TK_COMMENT,
    TK_ERROR, TK_EOF
} TokenKind;

typedef struct {
    TokenKind kind;
    char lexeme[256];
    size_t line;
    size_t column;
} Token;

static const char *keywords[] = {
    "auto","break","case","char","const","continue","default","do","double",
    "else","enum","extern","float","for","goto","if","inline","int","long",
    "register","restrict","return","short","signed","sizeof","static","struct",
    "switch","typedef","union","unsigned","void","volatile","while",NULL
};

static int is_keyword(const char *lexeme) {
    for (size_t i = 0; keywords[i]; ++i) {
        if (strcmp(keywords[i], lexeme) == 0) {
            return 1;
        }
    }
    return 0;
}

static void print_token(const Token *tok) {
    const char *kind_str = NULL;
    switch (tok->kind) {
        case TK_IDENTIFIER: kind_str = "IDENT"; break;
        case TK_KEYWORD: kind_str = "KEYWORD"; break;
        case TK_INT_LITERAL: kind_str = "INT"; break;
        case TK_HEX_LITERAL: kind_str = "HEX"; break;
        case TK_FLOAT_LITERAL: kind_str = "FLOAT"; break;
        case TK_STRING_LITERAL: kind_str = "STRING"; break;
        case TK_CHAR_LITERAL: kind_str = "CHAR"; break;
        case TK_OPERATOR: kind_str = "OP"; break;
        case TK_PUNCTUATION: kind_str = "PUNC"; break;
        case TK_COMMENT: kind_str = "COMMENT"; break;
        case TK_ERROR: kind_str = "ERROR"; break;
        case TK_EOF: kind_str = "EOF"; break;
        default: kind_str = "UNKNOWN"; break;
    }
    printf("(%s, \"%s\", line=%zu, col=%zu)\n", kind_str, tok->lexeme, tok->line, tok->column);
}

typedef struct {
    FILE *fp;
    int current;
    size_t line;
    size_t column;
    size_t last_line_column;
} Scanner;

static void scanner_init(Scanner *s, FILE *fp) {
    s->fp = fp;
    s->current = fgetc(fp);
    s->line = 1;
    s->column = 1;
    s->last_line_column = 1;
}

static void scanner_advance(Scanner *s) {
    if (s->current == '\n') {
        s->line++;
        s->last_line_column = s->column;
        s->column = 1;
    } else {
        s->column++;
    }
    s->current = fgetc(s->fp);
}

static int scanner_peek(Scanner *s) {
    return s->current;
}

static Token make_simple_token(TokenKind kind, const char *lexeme, size_t line, size_t col) {
    Token tok;
    tok.kind = kind;
    tok.line = line;
    tok.column = col;
    strncpy(tok.lexeme, lexeme, sizeof(tok.lexeme) - 1);
    tok.lexeme[sizeof(tok.lexeme) - 1] = '\0';
    return tok;
}

static Token lex_error(const char *message, size_t line, size_t col) {
    return make_simple_token(TK_ERROR, message, line, col);
}

static Token scan_string_literal(Scanner *s, size_t start_line, size_t start_col) {
    char buffer[256];
    size_t idx = 0;
    scanner_advance(s);
    while (scanner_peek(s) != EOF && scanner_peek(s) != '"' && idx < sizeof(buffer) - 1) {
        if (scanner_peek(s) == '\\') {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
            if (scanner_peek(s) == EOF) break;
        }
        buffer[idx++] = (char)scanner_peek(s);
        scanner_advance(s);
    }
    if (scanner_peek(s) != '"') {
        return lex_error("Unterminated string literal", start_line, start_col);
    }
    scanner_advance(s);
    buffer[idx] = '\0';
    return make_simple_token(TK_STRING_LITERAL, buffer, start_line, start_col);
}

static Token scan_char_literal(Scanner *s, size_t start_line, size_t start_col) {
    char buffer[256];
    size_t idx = 0;
    scanner_advance(s);
    while (scanner_peek(s) != EOF && scanner_peek(s) != '\'' && idx < sizeof(buffer) - 1) {
        if (scanner_peek(s) == '\\') {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
            if (scanner_peek(s) == EOF) break;
        }
        buffer[idx++] = (char)scanner_peek(s);
        scanner_advance(s);
    }
    if (scanner_peek(s) != '\'') {
        return lex_error("Unterminated char literal", start_line, start_col);
    }
    scanner_advance(s);
    buffer[idx] = '\0';
    return make_simple_token(TK_CHAR_LITERAL, buffer, start_line, start_col);
}

static Token scan_identifier_or_keyword(Scanner *s, size_t start_line, size_t start_col) {
    char buffer[256];
    size_t idx = 0;
    while (scanner_peek(s) != EOF &&
           (isalnum(scanner_peek(s)) || scanner_peek(s) == '_') &&
           idx < sizeof(buffer) - 1) {
        buffer[idx++] = (char)scanner_peek(s);
        scanner_advance(s);
    }
    buffer[idx] = '\0';
    if (is_keyword(buffer)) {
        return make_simple_token(TK_KEYWORD, buffer, start_line, start_col);
    }
    return make_simple_token(TK_IDENTIFIER, buffer, start_line, start_col);
}

static int is_hex_digit(int ch) {
    return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static Token scan_number(Scanner *s, size_t start_line, size_t start_col) {
    char buffer[256];
    size_t idx = 0;
    int ch = scanner_peek(s);
    int is_hex = 0;
    int saw_dot = 0;
    int saw_exp = 0;
    if (ch == '0') {
        buffer[idx++] = (char)ch;
        scanner_advance(s);
        if (scanner_peek(s) == 'x' || scanner_peek(s) == 'X') {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
            is_hex = 1;
            while (is_hex_digit(scanner_peek(s)) && idx < sizeof(buffer) - 1) {
                buffer[idx++] = (char)scanner_peek(s);
                scanner_advance(s);
            }
            buffer[idx] = '\0';
            return make_simple_token(TK_HEX_LITERAL, buffer, start_line, start_col);
        }
    }
    while (isdigit(scanner_peek(s)) && idx < sizeof(buffer) - 1) {
        buffer[idx++] = (char)scanner_peek(s);
        scanner_advance(s);
    }
    if (scanner_peek(s) == '.' && idx < sizeof(buffer) - 1) {
        saw_dot = 1;
        buffer[idx++] = '.';
        scanner_advance(s);
        while (isdigit(scanner_peek(s)) && idx < sizeof(buffer) - 1) {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
        }
    }
    if ((scanner_peek(s) == 'e' || scanner_peek(s) == 'E') && idx < sizeof(buffer) - 1) {
        saw_exp = 1;
        buffer[idx++] = (char)scanner_peek(s);
        scanner_advance(s);
        if ((scanner_peek(s) == '+' || scanner_peek(s) == '-') && idx < sizeof(buffer) - 1) {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
        }
        while (isdigit(scanner_peek(s)) && idx < sizeof(buffer) - 1) {
            buffer[idx++] = (char)scanner_peek(s);
            scanner_advance(s);
        }
    }
    buffer[idx] = '\0';
    if (saw_dot || saw_exp) {
        return make_simple_token(TK_FLOAT_LITERAL, buffer, start_line, start_col);
    }
    return make_simple_token(TK_INT_LITERAL, buffer, start_line, start_col);
}

static Token scan_comment_or_operator(Scanner *s, size_t start_line, size_t start_col) {
    int ch = scanner_peek(s);
    scanner_advance(s);
    if (ch == '/') {
        if (scanner_peek(s) == '/') {
            scanner_advance(s);
            char buffer[256];
            size_t idx = 0;
            while (scanner_peek(s) != EOF && scanner_peek(s) != '\n' && idx < sizeof(buffer) - 1) {
                buffer[idx++] = (char)scanner_peek(s);
                scanner_advance(s);
            }
            buffer[idx] = '\0';
            return make_simple_token(TK_COMMENT, buffer, start_line, start_col);
        } else if (scanner_peek(s) == '*') {
            scanner_advance(s);
            char buffer[256];
            size_t idx = 0;
            int terminated = 0;
            while (scanner_peek(s) != EOF && idx < sizeof(buffer) - 2) {
                if (scanner_peek(s) == '*') {
                    scanner_advance(s);
                    if (scanner_peek(s) == '/') {
                        scanner_advance(s);
                        terminated = 1;
                        break;
                    }
                    buffer[idx++] = '*';
                    continue;
                }
                buffer[idx++] = (char)scanner_peek(s);
                scanner_advance(s);
            }
            buffer[idx] = '\0';
            if (!terminated) {
                return lex_error("Unterminated block comment", start_line, start_col);
            }
            return make_simple_token(TK_COMMENT, buffer, start_line, start_col);
        }
        return make_simple_token(TK_OPERATOR, "/", start_line, start_col);
    }
    char lexeme[3] = {(char)ch, '\0', '\0'};
    if ((ch == '+' || ch == '-' || ch == '=' || ch == '!' || ch == '<' || ch == '>') &&
        (scanner_peek(s) == '=' || (ch == '+' && scanner_peek(s) == '+') ||
         (ch == '-' && scanner_peek(s) == '-'))) {
        lexeme[1] = (char)scanner_peek(s);
        lexeme[2] = '\0';
        scanner_advance(s);
    }
    return make_simple_token(TK_OPERATOR, lexeme, start_line, start_col);
}

static Token scanner_next_token(Scanner *s) {
    while (isspace(scanner_peek(s))) {
        scanner_advance(s);
    }
    if (scanner_peek(s) == EOF) {
        return make_simple_token(TK_EOF, "EOF", s->line, s->column);
    }
    size_t start_line = s->line;
    size_t start_col = s->column;
    int ch = scanner_peek(s);
    if (isalpha(ch) || ch == '_') {
        return scan_identifier_or_keyword(s, start_line, start_col);
    }
    if (isdigit(ch)) {
        return scan_number(s, start_line, start_col);
    }
    if (ch == '"') {
        return scan_string_literal(s, start_line, start_col);
    }
    if (ch == '\'') {
        return scan_char_literal(s, start_line, start_col);
    }
    if (ch == '/' || ch == '+' || ch == '-' || ch == '*' || ch == '%' ||
        ch == '=' || ch == '!' || ch == '<' || ch == '>') {
        return scan_comment_or_operator(s, start_line, start_col);
    }
    if (strchr("(){}[];,.:?&|^~#", ch)) {
        char lexeme[2] = {(char)ch, '\0'};
        scanner_advance(s);
        return make_simple_token(TK_PUNCTUATION, lexeme, start_line, start_col);
    }
    if (ch == '@' || ch == '$' || ch == '`') {
        char bad[2] = {(char)ch, '\0'};
        scanner_advance(s);
        return make_simple_token(TK_ERROR, bad, start_line, start_col);
    }
    char unknown[2] = {(char)ch, '\0'};
    scanner_advance(s);
    return make_simple_token(TK_ERROR, unknown, start_line, start_col);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <source-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    Scanner scanner;
    scanner_init(&scanner, fp);
    for (;;) {
        Token tok = scanner_next_token(&scanner);
        print_token(&tok);
        if (tok.kind == TK_EOF) {
            break;
        }
        if (tok.kind == TK_ERROR) {
            fprintf(stderr, "Lexical error at line %zu col %zu: %s\n",
                    tok.line, tok.column, tok.lexeme);
        }
    }

    fclose(fp);
    return EXIT_SUCCESS;
}

