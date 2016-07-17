#include "qobject.h"

enum symbol_lexer_state {
  IN_ADDR,
  IN_KIND,
  IN_SYM,
  IN_PROP,
};

enum kmodi_lexer_state {
  IN_NAME,
  IN_SIZE,
  IN_INST,
  IN_DEP,
  IN_STAT,
  IN_KADDR,
};

typedef struct {
  int state;
  int last;
}SymLexer;

typedef struct QKmod {
  QObject_HEAD;
  int64_t addr;
  int64_t size;
} QKmod;

QKmod *qkmod(int64_t addr, int64_t size);

QKmod *qkmod(int64_t addr, int64_t size) {
  QKmod *qk;
  qk = g_malloc(sizeof(*qk));
  qk->addr = addr;
  qk->size = size;
  return qk;
}
