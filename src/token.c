#include <token.h>

TokenizerContext* create_tc(wchar_t* file){
  TokenizerContext* tc = (TokenizerContext*)S_malloc(sizeof(TokenizerContext));

  tc->file = file;
  tc->cur_c = file;
  tc->token_cache = NULL;
  
  return tc;
}

short is_sc(const wchar_t wc) {
  if (wc == L'_') return 0;

  if ((wc >= L'!' && wc <= L'/') ||
      (wc >= L':' && wc <= L'@') ||
      (wc >= L'[' && wc <= L'`') ||
      (wc >= L'{' && wc <= L'~')) {
    return 1;
  }
  return 0;
}

Token* get_sc_token(TokenizerContext* tc) {
  TokenType type = TokEOF;
  wchar_t str[MAX_TOKEN_STR];
  unsigned str_len = 0;
  
  char* c = tc->cur_c;
  
  str[str_len] = c;
  str_len++;
  
  switch (*c) {
  case L'\"': {
    type = TokStringLiteral;

    while (*(c + 1) != L'\"') {
      c++;

      str[str_len] = *c;
      str_len++;
    }

    c++;

    str[str_len] = *c;
    str_len++;


    break;
  }

  case L'=': {
    type = TokAssign;
    if (*(c + 1) == L'=') {
      type = TokEqual;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    break;
  }

  case L'!': {
    type = TokNot;
    if (*(c + 1) == L'=') {
      type = TokNotEqual;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    break;
  }

  case L'#': {
    type = TokSharp;
    break;
  }

  case L'<': {
    type = TokLesser;

    if (*(c + 1) == L'=') {
      type = TokEqualLesser;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    break;
  }

  case L'>': {
    type = TokGreater;
    if (*(c + 1) == L'=') {
      type = TokEqualGreater;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    break;
  }

  case L'(': {
    type = TokLParen;
    break;
  }

  case L')': {
    type = TokRParen;
    break;
  }

  case L'{': {
    type = TokLBracket;
    break;
  }

  case L'}': {
    type = TokRBracket;
    break;
  }

  case L'[': {
    type = TokLSquareBracket;
    break;
  }

  case L']': {
    type = TokRSquareBracket;
    break;
  }

  case L':': {
    type = TokColon;
    break;
  }

  case L';': {
    type = TokSemiColon;
    break;
  }

  case L'.': {
    type = TokDot;
    break;
  }

  case L',': {
    type = TokComma;
    break;
  }

  case L'|': {
    type = TokBitOr;

    if (*(c + 1) == L'|') {
      type = TokOr;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    break;
  }

  case L'&': {
    type = TokBitAnd;

    if (*(c + 1) == L'&') {
      type = TokAnd;
      c++;
    }
    break;
  }

  case L'+': {
    type = TokAdd;
    if (*(c + 1) == L'=') {
      type = TokPlusAssign;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    if (*(c + 1) == L'+') {
      type = TokIncrease;
      c++;

      str[str_len] = *c;
      str_len++;
    }

    break;
  }

  case L'-': {
    type = TokSub;
    if (*(c + 1) == L'=') {
      type = TokMinusAssign;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    if (*(c + 1) == L'-') {
      type = TokDecrease;
      c++;

      str[str_len] = *c;
      str_len++;
    }

    break;
  }

  case L'*': {
    type = TokMul;
    if (*(c + 1) == L'=') {
      type = TokMultAssign;
      c++;

      str[str_len] = *c;
      str_len++;
    }
    if (*(c + 1) == L'*') {
      type = TokPow;
      c++;

      str[str_len] = *c;
      str_len++;
    }

    break;
  }

  case L'/': {
    type = TokDiv;
    if (*(c + 1) == L'=') {
      type = TokDivAssign;
      c++;

      str[str_len] = c;
      str_len++;
    }
    break;
  }

  }

  str[str_len] = '\0';
  str_len++;

  Token* res = (Token*) S_malloc(sizeof(Token));
  res->str = str;
  res->type = type;
  
  return res;
}

