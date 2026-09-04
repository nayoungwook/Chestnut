#ifndef TOKEN_H
#define TOKEN_H

#include <util.h>

#include <memory.h>
#include <wchar.h>

#define MAX_TOKEN_STR 512

enum TokenType {
    TokEOF = -1,
    TokIdent = 0,
    TokNumberLiteral = 1,
    TokStringLiteral = 2,
    TokCharacterLiteral = 3,
    TokBoolLiteral = 4,
    TokEqual = 5,
    TokNotEqual = 6,
    TokSharp = 7,
    TokLesser = 8,
    TokEqualLesser = 9,
    TokGreater = 10,
    TokEqualGreater = 11,
    TokAdd = 12,
    TokSub = 13,
    TokMul = 14,
    TokDiv = 15,
    TokLParen = 16,
    TokRParen = 17,
    TokLBracket = 18,
    TokRBracket = 19,
    TokLSquareBracket = 20,
    TokRSquareBracket = 21,
    TokAssign = 22,
    TokSemiColon = 24,
    TokColon = 25,
    TokComma = 26,
    TokDot = 27,

    TokPlusAssign = 28,
    TokMinusAssign = 29,
    TokMultAssign = 30,
    TokDivAssign = 31,

    TokIncrease = 32,
    TokDecrease = 33,
    TokPow = 34,

    TokNot = 35,

    TokVar = 36,
    TokIf = 37,
    TokFor = 38,
    TokFunc = 39,

    TokOr = 40,
    TokAnd = 41,

    TokBitOr = 42,
    TokBitAnd = 43,

    TokReturn = 44,

    TokElse = 45,
    TokClass = 46,
    TokExtends = 47,

    TokPrivate = 48,
    TokProtected = 49,
    TokPublic = 50,

    TokConstructor = 51,
    TokNew = 52,

    TokTrue = 53,
    TokFalse = 54,

    TokNull = 55,
};

struct Token {
    const char *str;
    unsigned length;
    enum TokenType type;
    bool managed_by_tokenizer;
    bool owns_str;
};

struct TokenizerContext {
    struct Token *token_cache; // cache token for consume, peek, pull
    char *file;                // full file contents.
    char *cur_ch;              // current ch
    const char *begin_ch;      // begin ch (*initial position of file.)
    unsigned line_num;
    struct Token **tokens;
    unsigned token_count;
    unsigned token_capacity;
};

struct KeywordEntry {
    const char *keyword;
    enum TokenType type;
};

void free_tc(struct TokenizerContext *tc);

struct Token *consume(struct TokenizerContext *tc, enum TokenType tt);
struct Token *peek(struct TokenizerContext *tc);
struct Token *pull(struct TokenizerContext *tc);

void flush_tc(struct TokenizerContext *tc);
void init_tc(struct TokenizerContext *tc);
struct TokenizerContext *gen_tc(char *file);

#endif
