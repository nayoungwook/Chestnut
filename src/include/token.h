#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MAX_TOKEN_STR 512

typedef enum {
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
  TokPow = 33,

  TokNot = 34,

  TokVar = 35,
  TokIf = 36,
  TokFor = 37,
  TokFunc = 38,

  TokOr = 39,
  TokAnd = 40,

  TokBitOr = 41,
  TokBitAnd = 42,

  TokReturn = 43,

  TokElse = 44,
  TokClass = 45,
  TokExtends = 46,

  TokPrivate = 47,
  TokProtected = 48,
  TokPublic = 49,

  TokConstructor = 50,
  TokNew = 51,

  TokTrue = 52,
  TokFalse = 53,
} TokenType;

typedef struct _Token {
  wchar_t* str;
  TokenType type;
} Token;

#endif
