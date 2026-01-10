#include <token.h>
#include <error.h>

static void init_keyword();

TokenizerContext* gen_tc(wchar_t* file){
  TokenizerContext* tc = (TokenizerContext*)S_malloc(sizeof(TokenizerContext));

  tc->file = file;
  tc->cur_ch = file;
  tc->begin_ch = file;
  tc->token_cache = NULL;
  tc->line_num = 1;

  init_keyword();  
  
  return tc;
}

static HTable *keyword_table;

static KeywordEntry *gen_keyword(const wchar_t *keyword, TokenType tok_type) {
  KeywordEntry *result = (KeywordEntry *)S_malloc(sizeof(KeywordEntry));

  result->keyword = keyword;
  result->type = tok_type;

  return result;  
}

static void init_keyword() {
  if (keyword_table != NULL) {
    return;
  }    
  
  keyword_table = gen_htable();  
  
  ht_insert(keyword_table, L"var", gen_keyword(L"var", TokVar));
  ht_insert(keyword_table, L"if", gen_keyword(L"if", TokIf));
  ht_insert(keyword_table, L"for", gen_keyword(L"for", TokFor));
  ht_insert(keyword_table, L"func", gen_keyword(L"func", TokFunc));
  ht_insert(keyword_table, L"return", gen_keyword(L"return", TokReturn));
  ht_insert(keyword_table, L"else", gen_keyword(L"else", TokElse));
  ht_insert(keyword_table, L"class", gen_keyword(L"class", TokClass));
  ht_insert(keyword_table, L"extends", gen_keyword(L"extends", TokExtends));
  ht_insert(keyword_table, L"private", gen_keyword(L"private", TokPrivate));
  ht_insert(keyword_table, L"public", gen_keyword(L"public", TokPublic));
  ht_insert(keyword_table, L"protected", gen_keyword(L"protected", TokProtected));
  ht_insert(keyword_table, L"constructor", gen_keyword(L"constructor", TokConstructor));
  ht_insert(keyword_table, L"new", gen_keyword(L"new", TokNew));
  ht_insert(keyword_table, L"true", gen_keyword(L"true", TokTrue));
  ht_insert(keyword_table, L"false", gen_keyword(L"false", TokFalse));
}  

void init_tc(TokenizerContext* tc){
  tc->cur_ch = (wchar_t*) tc->begin_ch;
  tc->token_cache = NULL;
  tc->line_num = 1;
}

static bool is_sc(const wchar_t wc) {
  if (wc == L'_') return false;

  if ((wc >= L'!' && wc <= L'/') ||
      (wc >= L':' && wc <= L'@') ||
      (wc >= L'[' && wc <= L'`') ||
      (wc >= L'{' && wc <= L'~')) {
    return true;
  }
  return false;
}

static Token* gen_num_token(TokenizerContext* tc) {
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

static TokenType check_ident_type(const wchar_t* str){
  KeywordEntry *ke = ht_find(keyword_table, str);

  if (ke == NULL) {
    return TokIdent;    
  }    

  return ke->type;
}

static Token* gen_ident_token(TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  while (iswalnum(*tc->cur_ch) || *tc->cur_ch == L'_') {
    str[str_len++] = *tc->cur_ch;

    if(str_len >= MAX_TOKEN_STR - 1) panic(L"token string buffer overflow", tc);
	
    tc->cur_ch++;
  }
  str[str_len] = L'\0';

  TokenType type = check_ident_type(str);
    
  Token* tok = (Token*)S_malloc(sizeof(Token));
  tok->str = str;
  tok->type = type;

  return tok;
}

static void get_str_literal(TokenizerContext* tc, wchar_t* str, unsigned *str_len){
    
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

static Token* gen_sc_token(TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  TokenType type;

  wchar_t ch = *tc->cur_ch;
  str[str_len++] = ch;
  tc->cur_ch++;

  switch (ch) {
  case L'\"': 
    type = TokStringLiteral;
    get_str_literal(tc, str, &str_len);
	
    break;

  case L'=': 
    type = TokAssign;
	
    if (*(tc->cur_ch) == L'=') {
      type = TokEqual;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;

  case L'!': 
    type = TokNot;
	
    if (*(tc->cur_ch) == L'=') {
      type = TokNotEqual;
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;


  case L'#': 
    type = TokSharp;
    break;

  case L'<': 
    type = TokLesser;

    if (*(tc->cur_ch) == L'=') {
      type = TokEqualLesser;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;

  case L'>': 
    type = TokGreater;
	
    if (*(tc->cur_ch) == L'=') {
      type = TokEqualGreater;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;

  case L'(': 
    type = TokLParen;
    break;

  case L')': 
    type = TokRParen;
    break;


  case L'{': 
    type = TokLBracket;
    break;

  case L'}': 
    type = TokRBracket;
    break;

  case L'[': 
    type = TokLSquareBracket;
    break;

  case L']': 
    type = TokRSquareBracket;
    break;

  case L':': 
    type = TokColon;
    break;

  case L';': 
    type = TokSemiColon;
    break;

  case L'.': 
    type = TokDot;
    break;

  case L',': 
    type = TokComma;
    break;

  case L'|': 
    type = TokBitOr;

    if (*(tc->cur_ch) == L'|') {
      type = TokOr;

      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;

  case L'&':
    type = TokBitAnd;

    if (*(tc->cur_ch) == L'&') {
      type = TokAnd;
	    
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;

  case L'+': 
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

  case L'-': 
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

  case L'*': 
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

  case L'/': 
    type = TokDiv;
    if (*(tc->cur_ch) == L'=') {
      type = TokDivAssign;
      
      str[str_len++] = *tc->cur_ch;
      tc->cur_ch++;
    }
    break;
    
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

Token* peek(TokenizerContext* tc) {
  if(tc->token_cache)
    return tc->token_cache;
  
  return tc->token_cache = pull(tc);
}

Token* pull(TokenizerContext *tc) {
  Token* tok_cache_backup = tc->token_cache;
  tc->token_cache = NULL;
  
  if(tok_cache_backup){
    return tok_cache_backup;
  }

  while (iswspace(*tc->cur_ch)) { // skip white space 
    if (*tc->cur_ch == L'\n') {
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

  Token* eof_token = (Token*)S_malloc(sizeof(Token));
  eof_token->str = L"EOF";
  eof_token->type = TokEOF;

  return eof_token;
}

Token* consume(TokenizerContext *tc, TokenType tt) {
  Token *tok = pull(tc);

  if (tok->type != tt) {
    panic(L"Wrong token type consumed.\n", tc);
  }

  return tok;  
}  
