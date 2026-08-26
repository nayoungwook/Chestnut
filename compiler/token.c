#include <error.h>
#include <token.h>

#include <assert.h>
#include <stdlib.h>
#include <wctype.h>

static void init_keyword();

struct TokenizerContext *gen_tc(char *file) {
        struct TokenizerContext *tc = (struct TokenizerContext *)S_malloc(
            sizeof(struct TokenizerContext));

        tc->file = file;
        tc->cur_ch = file;
        tc->begin_ch = file;
        tc->token_cache = NULL;
        tc->line_num = 1;

        init_keyword();

        return tc;
}

void free_tc(struct TokenizerContext *tc) { free(tc); }

void flush_tc(struct TokenizerContext *tc) {
        while (peek(tc)->type != TokEOF) {
                struct Token *tok = pull(tc);

                printf("token : %s %d\n", tok->str, tok->type);
        }

        init_tc(tc);
}

static struct HTable *keyword_table;

static struct KeywordEntry *gen_keyword(const char *keyword,
                                        enum TokenType tok_type) {
        struct KeywordEntry *result =
            (struct KeywordEntry *)S_malloc(sizeof(struct KeywordEntry));

        result->keyword = keyword;
        result->type = tok_type;

        return result;
}

static void insert_keyword(const char *keyword, enum TokenType tok_type) {
        assert(keyword_table != NULL);

        ht_insert(keyword_table, keyword, gen_keyword(keyword, tok_type));
}

static void init_keyword() {
        if (keyword_table != NULL) {
                return;
        }

        keyword_table = gen_htable();

        insert_keyword("var", TokVar);
        insert_keyword("if", TokIf);
        insert_keyword("for", TokFor);
        insert_keyword("func", TokFunc);
        insert_keyword("return", TokReturn);
        insert_keyword("else", TokElse);
        insert_keyword("class", TokClass);
        insert_keyword("extends", TokExtends);
        insert_keyword("private", TokPrivate);
        insert_keyword("public", TokPublic);
        insert_keyword("protected", TokProtected);
        insert_keyword("constructor", TokConstructor);
        insert_keyword("new", TokNew);
        insert_keyword("true", TokTrue);
        insert_keyword("false", TokFalse);
        insert_keyword("null", TokNull);
}

void init_tc(struct TokenizerContext *tc) {
        tc->cur_ch = (char *)tc->begin_ch;
        tc->token_cache = NULL;
        tc->line_num = 1;
}

static bool is_sc(const char wc) {
        if (wc == '_')
                return false;

        if ((wc >= '!' && wc <= '/') || (wc >= ':' && wc <= '@') ||
            (wc >= '[' && wc <= '`') || (wc >= '{' && wc <= '~')) {
                return true;
        }
        return false;
}

static struct Token *gen_num_token(struct TokenizerContext *tc) {
        char *str = (char *)S_malloc(MAX_TOKEN_STR * sizeof(char));
        unsigned str_len = 0;
        unsigned dot_count = 0;

        while (iswdigit(*tc->cur_ch) || *tc->cur_ch == '.' || *tc->cur_ch == 'f') {
                str[str_len++] = *tc->cur_ch;

                if (str_len >= MAX_TOKEN_STR - 1)
                        panic("token string buffer overflow", tc);

                tc->cur_ch++;

                if (*tc->cur_ch == '.') {
                        dot_count++;
                }
                if (dot_count >= 2) {
                        panic("Invalid numeric type.", tc);
                }
        }
        str[str_len] = '\0';
	
        struct Token *tok = (struct Token *)S_malloc(sizeof(struct Token));

        tok->length = str_len;
        tok->str = str;
        tok->type = TokNumberLiteral;

        return tok;
}

static enum TokenType check_ident_type(const char *str) {
        struct KeywordEntry *ke = ht_find(keyword_table, str);

        if (ke == NULL) {
                return TokIdent;
        }

        return ke->type;
}

static struct Token *gen_ident_token(struct TokenizerContext *tc) {
        char *str = (char *)S_malloc(MAX_TOKEN_STR * sizeof(char));
        unsigned str_len = 0;

        while (iswalnum(*tc->cur_ch) || *tc->cur_ch == '_') {
                str[str_len++] = *tc->cur_ch;

                if (str_len >= MAX_TOKEN_STR - 1)
                        panic("token string buffer overflow", tc);

                tc->cur_ch++;
        }
        str[str_len] = '\0';

        enum TokenType type = check_ident_type(str);

        struct Token *tok = (struct Token *)S_malloc(sizeof(struct Token));
        tok->str = str;
        tok->type = type;

        return tok;
}

static void get_str_literal(struct TokenizerContext *tc, char *str,
                            unsigned *str_len) {

        while (true) {
                char ch = *(tc->cur_ch);

                if (ch == '\0') {
                        panic("Unterminaled string literal", tc);
                }
                if (ch == '\"') {
                        tc->cur_ch++;
                        break;
                }

                if (ch == '\\') {
                        tc->cur_ch++;
                        ch = *(tc->cur_ch);

                        switch (ch) {
                        case 'n': {
                                ch = '\n';
                                break;
                        }
                        case 'r': {
                                ch = '\r';
                                break;
                        }
                        case 't': {
                                ch = '\t';
                                break;
                        }
                        case '\\': {
                                ch = '\\';
                                break;
                        }
                        case '\"': {
                                ch = '\"';
                                break;
                        }
                        case '\0': {
                                panic("Unterminaled string escape", tc);
                                break;
                        }
                        default: {
                                panic("Unknown string escape sequence", tc);
                                break;
                        }
                        }
                }

                if (*str_len >= MAX_TOKEN_STR - 1)
                        panic("String literal buffer overflow.", tc);

                str[(*str_len)++] = ch;
                tc->cur_ch++;
        }
}

static struct Token *gen_sc_token(struct TokenizerContext *tc) {
        char *str = (char *)S_malloc(MAX_TOKEN_STR * sizeof(char));
        unsigned str_len = 0;

        enum TokenType type;
	
        char ch = *tc->cur_ch;
        str[str_len++] = ch;
        tc->cur_ch++;

        switch (ch) {
        case '\"': {
                type = TokStringLiteral;
                str_len = 0;
                get_str_literal(tc, str, &str_len);
                break;
        }

        case '=': {
                type = TokAssign;

                if (*(tc->cur_ch) == '=') {
                        type = TokEqual;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '!': {
                type = TokNot;

                if (*(tc->cur_ch) == '=') {
                        type = TokNotEqual;
                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '#': {
                type = TokSharp;
                break;
        }

        case '<': {
                type = TokLesser;

                if (*(tc->cur_ch) == '=') {
                        type = TokEqualLesser;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '>': {
                type = TokGreater;

                if (*(tc->cur_ch) == '=') {
                        type = TokEqualGreater;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '(': {
                type = TokLParen;
                break;
        }

        case ')': {
                type = TokRParen;
                break;
        }

        case '{': {
                type = TokLBracket;
                break;
        }

        case '}': {
                type = TokRBracket;
                break;
        }

        case '[': {
                type = TokLSquareBracket;
                break;
        }

        case ']': {
                type = TokRSquareBracket;
                break;
        }

        case ':': {
                type = TokColon;
                break;
        }

        case ';': {
                type = TokSemiColon;
                break;
        }

        case '.': {
                type = TokDot;
                break;
        }

        case ',': {
                type = TokComma;
                break;
        }

        case '|': {
                type = TokBitOr;

                if (*(tc->cur_ch) == '|') {
                        type = TokOr;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '&': {
                type = TokBitAnd;

                if (*(tc->cur_ch) == '&') {
                        type = TokAnd;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '+': {
                type = TokAdd;

                if (*(tc->cur_ch) == '=') {
                        type = TokPlusAssign;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }

		bool is_incre = *(tc->cur_ch) == '+';
		bool is_add_and_incre = *(tc->cur_ch) == '+' && *(tc->cur_ch + 1) == '+'; // if +++ we have to tokenize it into + ++
		if (is_incre && !is_add_and_incre) {
                        type = TokIncrease;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '-': {
                type = TokSub;

                if (*(tc->cur_ch) == '=') {
                        type = TokMinusAssign;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }

		bool is_decre = *(tc->cur_ch) == '-';
		bool is_sub_and_decre = *(tc->cur_ch) == '-' && *(tc->cur_ch + 1) == '-'; // if --- we have to tokenize it into - --
		if (is_decre && !is_sub_and_decre) {
                        type = TokDecrease;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '*': {
                type = TokMul;
                if (*(tc->cur_ch) == '=') {
                        type = TokMultAssign;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                } else if (*(tc->cur_ch) == '*') {
                        type = TokPow;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        case '/': {
                type = TokDiv;
                if (*(tc->cur_ch) == '=') {
                        type = TokDivAssign;

                        str[str_len++] = *tc->cur_ch;
                        tc->cur_ch++;
                }
                break;
        }

        default: {
                panic("Undefined special character", tc);
                break;
        }
        }

        str[str_len] = '\0';

        struct Token *tok = (struct Token *)S_malloc(sizeof(struct Token));
        tok->str = str;
        tok->type = type;
	tok->length = str_len;

        return tok;
}

struct Token *peek(struct TokenizerContext *tc) {
        if (tc->token_cache != NULL) {
                return tc->token_cache;
        }

        return tc->token_cache = pull(tc);
}

struct Token *pull(struct TokenizerContext *tc) {
        struct Token *result = NULL;

        // resolve cached token.
        if (tc->token_cache != NULL) {
                result = tc->token_cache;
                tc->token_cache = NULL;
                return result;
        }

        // skip white space
        while (iswspace(*tc->cur_ch)) {
                if (*tc->cur_ch == '\n') {
                        tc->line_num++;
                }

                tc->cur_ch++;
        }

        if (iswalpha(*tc->cur_ch)) {
                return gen_ident_token(tc);
        }

        if (is_sc(*tc->cur_ch)) {
                return gen_sc_token(tc);
        }

        if (iswdigit(*tc->cur_ch)) {
                return gen_num_token(tc);
        }

        struct Token *eof_token =
            (struct Token *)S_malloc(sizeof(struct Token));
        eof_token->str = "EOF";
	eof_token->length = 0;
        eof_token->type = TokEOF;

        return eof_token;
}

struct Token *consume(struct TokenizerContext *tc, enum TokenType tt) {
        struct Token *tok = pull(tc);

        if (tok->type != tt) {
                printf("\n%d , expected : %d\n", tok->type, tt);
                panic("Wrong token type consumed.\n", tc);
        }

        return tok;
}
