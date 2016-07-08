enum symbol_lexer_state {
  IN_ADDR,
  IN_KIND,
  IN_SYM,
  IN_PROP,  
};

typedef struct {
  int state;
  int last;
  int64_t addr;
}SymLexer;
