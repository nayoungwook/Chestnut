#include <token.h>
#include <error.h>

#include <stdlib.h>
#include <wctype.h>
#include <assert.h>

static void init_keyword();

struct TokenizerContext* gen_tc(wchar_t* file){
  struct TokenizerContext* tc = (struct TokenizerContext*)S_malloc(sizeof(struct TokenizerContext));

  tc->file = file;
  tc->cur_ch = file;
  tc->begin_ch = file;
  tc->token_cache = NULL;
  tc->line_num = 1;

  init_keyword();  
  
  return tc;
}

static struct HTable *keyword_table;

static struct KeywordEntry *gen_keyword(const wchar_t *keyword, enum TokenType tok_type) {
  struct KeywordEntry *result = (struct KeywordEntry *)S_malloc(sizeof(struct KeywordEntry));

  result->keyword = keyword;
  result->type = tok_type;

  return result;  
}

static void insert_keyword(const wchar_t *keyword, enum TokenType tok_type) {
  assert(keyword_table != NULL);

  ht_insert(keyword_table, keyword, gen_keyword(keyword, tok_type));
}  

static void init_keyword() {
  if (keyword_table != NULL) {
    return;
  }    
  
  keyword_table = gen_htable();

  insert_keyword(L"var", TokVar);   
  insert_keyword(L"if", TokIf);  
  insert_keyword(L"for", TokFor);  
  insert_keyword(L"func", TokFunc);  
  insert_keyword(L"return", TokReturn);  
  insert_keyword(L"else", TokElse);  
  insert_keyword(L"class", TokClass);  
  insert_keyword(L"extends", TokExtends);  
  insert_keyword(L"private", TokPrivate);  
  insert_keyword(L"public", TokPublic);  
  insert_keyword(L"protected", TokProtected);  
  insert_keyword(L"constructor", TokConstructor);  
  insert_keyword(L"new", TokNew);  
  insert_keyword(L"true", TokTrue);  
  insert_keyword(L"false", TokFalse);  
}  

void init_tc(struct TokenizerContext* tc){
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

static struct Token* gen_num_token(struct TokenizerContext* tc) {
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

  struct Token* tok = (struct Token*)S_malloc(sizeof(struct Token));
  tok->str = str;
  tok->type = TokNumberLiteral;
    
  return tok;
}

static enum TokenType check_ident_type(const wchar_t* str){
  struct KeywordEntry *ke = ht_find(keyword_table, str);

  if (ke == NULL) {
    return TokIdent;    
  }    

  return ke->type;
}

static struct Token* gen_ident_token(struct TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  while (iswalnum(*tc->cur_ch) || *tc->cur_ch == L'_') {
    str[str_len++] = *tc->cur_ch;

    if(str_len >= MAX_TOKEN_STR - 1) panic(L"token string buffer overflow", tc);
	
    tc->cur_ch++;
  }
  str[str_len] = L'\0';

  enum TokenType type = check_ident_type(str);

  struct Token *tok = (struct Token *)S_malloc(sizeof(struct Token));
  tok->str = str;
  tok->type = type;

  return tok;
}

static void get_str_literal(struct TokenizerContext* tc, wchar_t* str, unsigned *str_len){
    
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

static struct Token* gen_sc_token(struct TokenizerContext* tc) {
  wchar_t* str = (wchar_t*) S_malloc(MAX_TOKEN_STR * sizeof(wchar_t));
  unsigned str_len = 0;

  enum TokenType type;

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
    
    struct Token* tok = (struct Token*) S_malloc(sizeof(struct Token));
    tok->str = str;
    tok->type = type;

    return tok;
}

struct Token* peek(struct TokenizerContext* tc) {
  if(tc->token_cache)
    return tc->token_cache;
  
  return tc->token_cache = pull(tc);
}

struct Token* pull(struct TokenizerContext *tc) {
  struct Token* tok_cache_backup = tc->token_cache;
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

  struct Token* eof_token = (struct Token*)S_malloc(sizeof(struct Token));
  eof_token->str = L"EOF";
  eof_token->type = TokEOF;

  return eof_token;
}

struct Token* consume(struct TokenizerContext *tc, enum TokenType tt) {
  struct Token *tok = pull(tc);

  if (tok->type != tt) {
    panic(L"Wrong token type consumed.\n", tc);
  }

  return tok;  
}  
