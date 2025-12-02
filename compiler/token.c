#include <token.h>
#include <error.h>

static KeywordEntry keyword_table[] = {
  {L"var", TokVar},
  {L"if", TokIf},
  {L"for", TokFor},
  {L"func", TokFunc},
  {L"return", TokReturn},
  {L"else", TokElse},
  {L"class", TokClass},
  {L"extends", TokExtends},
  {L"private", TokPrivate},
  {L"public", TokPublic},
  {L"protected", TokProtected},
  {L"constructor", TokConstructor},
  {L"new", TokNew},
  {L"true", TokTrue},
  {L"false", TokFalse},
  {NULL, TokEOF},
};

Token* peek(TokenizerContext* tc) {
  if(!tc->token_cache){
    pull(tc);
  }

  return tc->token_cache;
}

Token* pull(TokenizerContext* tc) {
  
  while (*tc->cur_ch != '\0') {
    if (iswspace(*tc->cur_ch)) { // skip white space
      tc->cur_ch++;
      continue;
    }

    if (iswalpha(*tc->cur_ch)) {
      return tc->token_cache = gen_ident_token(tc);
    }
    
    if (is_sc(*tc->cur_ch)) {
      return tc->token_cache = gen_sc_token(tc);
    }
    
    if (iswdigit(*tc->cur_ch)) {
      return tc->token_cache = gen_num_token(tc);
    }
  }

  Token* eof_token = (Token*)S_malloc(sizeof(Token));
  eof_token->str = L"EOF";
  eof_token->type = TokEOF;

  return tc->token_cache = eof_token;
}

TokenizerContext* gen_tc(wchar_t* file){
  TokenizerContext* tc = (TokenizerContext*)S_malloc(sizeof(TokenizerContext));

  tc->file = file;
  tc->cur_ch = file;
  tc->token_cache = NULL;
  tc->line_num = 0;
  
  return tc;
}

bool is_sc(const wchar_t wc) {
  if (wc == L'_') return false;

  if ((wc >= L'!' && wc <= L'/') ||
      (wc >= L':' && wc <= L'@') ||
      (wc >= L'[' && wc <= L'`') ||
      (wc >= L'{' && wc <= L'~')) {
    return true;
  }
  return false;
}

Token* gen_num_token(TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;
  unsigned dot_count = 0;
  
  while (iswdigit(*tc->cur_ch) || *tc->cur_ch == L'.') {
    str[str_len++] = *tc->cur_ch;

    if(str_len >= MAX_TOKEN_STR - 1) panic(L"token string buffer overflow", tc);

    tc->cur_ch++;
    
    if(*tc->cur_ch == L'.') {
      dot_count++;
    }
    if(dot_count >= 2){
      panic(L"Invalid numeric type.", tc);
    }
  }
  str[str_len] = L'\0';

  Token* tok = (Token*)S_malloc(sizeof(Token));
  tok->str = str;
  tok->type = TokNumberLiteral;
  
  return tok;
}

Token* gen_ident_token(TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  TokenType type = TokIdent;

  while (iswalnum(*tc->cur_ch) || *tc->cur_ch == L'_') {
    str[str_len++] = *tc->cur_ch;

    if(str_len >= MAX_TOKEN_STR - 1) panic(L"token string buffer overflow", tc);
    
    tc->cur_ch++;
  }
  str[str_len] = L'\0';

  int i;
  for (i = 0; keyword_table[i].keyword != NULL; i++) {
    if (wcscmp(str, keyword_table[i].keyword) == 0) {
      type = keyword_table[i].type;
      break;
    }
  }
  
  Token* tok = (Token*)S_malloc(sizeof(Token));
  tok->str = str;
  tok->type = type;

  return tok;
}

void get_str_literal(TokenizerContext* tc, wchar_t* str, unsigned *str_len){
  
  while (true) {
    wchar_t ch = *(tc->cur_ch);
    if(ch == L'\0'){
      panic(L"Unterminaled string literal", tc);
    }
    if(ch == L'\"'){
      tc->cur_ch++;
      break;
    }
    if(*str_len >= MAX_TOKEN_STR - 1) panic(L"String literal buffer overflow.", tc);

    str[(*str_len)++] = ch;
    tc->cur_ch++;
  }
  
  str[(*str_len)++] = L'\"';
  tc->cur_ch++;
}

Token* gen_sc_token(TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  TokenType type;

  wchar_t ch = *tc->cur_ch;
  str[str_len++] = ch;
  tc->cur_ch++;
  
  switch (ch) {
  case L'\"': {
    type = TokStringLiteral;
    get_str_literal(tc, str, &str_len);
    
    break;
  }

  case L'=': {
    type = TokAssign;
    
    if (*(tc->cur_ch) == L'=') {
      type = TokEqual;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  case L'!': {
    type = TokNot;
    
    if (*(tc->cur_ch) == L'=') {
      type = TokNotEqual;
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  case L'#': {
    type = TokSharp;
    break;
  }

  case L'<': {
    type = TokLesser;

    if (*(tc->cur_ch) == L'=') {
      type = TokEqualLesser;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  case L'>': {
    type = TokGreater;
    
    if (*(tc->cur_ch) == L'=') {
      type = TokEqualGreater;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
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

    if (*(tc->cur_ch) == L'|') {
      type = TokOr;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  case L'&': {
    type = TokBitAnd;

    if (*(tc->cur_ch) == L'&') {
      type = TokAnd;
	    
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  case L'+': {
    type = TokAdd;
    
    if (*(tc->cur_ch) == L'=') {
      type = TokPlusAssign;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    else if (*(tc->cur_ch) == L'+') {
      type = TokIncrease;
	    
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }

    break;
  }

  case L'-': {
    type = TokSub;
    
    if (*(tc->cur_ch) == L'=') {
      type = TokMinusAssign;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    else if (*(tc->cur_ch) == L'-') {
      type = TokDecrease;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }

    break;
  }

  case L'*': {
    type = TokMul;
    if (*(tc->cur_ch) == L'=') {
      type = TokMultAssign;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    else if (*(tc->cur_ch) == L'*') {
      type = TokPow;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }

    break;
  }

  case L'/': {
    type = TokDiv;
    if (*(tc->cur_ch) == L'=') {
      type = TokDivAssign;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
  }

  default: {
    panic(L"Undefined special character", tc);
    break;
  }
  }

  str[str_len] = L'\0';

  Token* tok = (Token*) S_malloc(sizeof(Token));
  tok->str = str;
  tok->type = type;
  
  return tok;
}

