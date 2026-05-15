#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hashtable.c"
#include "array.c"
#include "regex.h"

#define bool _Bool
#define true 1
#define false 0

#define epsilon '\0'

/* NOTE: not all symbols are here */
char* Left_Sym = "\\([";
char* Right_Sym = "*+)]?";
char* Mid_Sym = "&|";
char* Sym = "&|*+()[]?";

enum Op {NUL, CONCAT, UNION, CLOSURE, PLUS, LP, RP, OPTIONAL};

int Op_Priory[] = {-1, 2, 1, 3, 3, 0, 9, 3};

typedef struct {size_t len; size_t cap; char* str;} Pattern;

static inline bool is_trans(Pattern* dst) {
  if (dst->len > 1 && dst->str[dst->len - 2] == '\\') {
    return true;
  } else {
    return false;
  }
}

static char* ch_range(char left, char right) {
  size_t range_len = right - left + 1;
  struct {size_t len; size_t cap; char* str;} str = {0, 32, NULL};
  str.str = calloc(str.cap, sizeof(char));
  for (size_t i = 0; i < range_len; i++) {
    ARRAY_PUSH(str.str, str.len, str.cap, left + i);
  }
  ARRAY_PUSH(str.str, str.len, str.cap, '\0');
  return str.str;
}

static char* pre_parse_src(char* src) {
  struct {size_t len; size_t cap; char* str;} str = {0, 32, NULL};
  str.str = calloc(str.cap, sizeof(char));
  size_t src_len = strlen(src);
  bool trans_mode = false;
  for (size_t i = 0; i < src_len; i++) {
    if (src[i] == '[') trans_mode = true;
    if (strchr("]()", src[i])) trans_mode = false;
    if (trans_mode && src[i] == '-' && src[i-1] != '[') {
      char* ch_class_range = ch_range(src[i-1], src[i+1]);
      size_t range_len = strlen(ch_class_range);
      ARRAY_DROP(str.str, str.len, str.cap);
      for (size_t j = 0; j < range_len; j++) {
	ARRAY_PUSH(str.str, str.len, str.cap, ch_class_range[j]);
      }
      free(ch_class_range);
      i += 2;
    }
    ARRAY_PUSH(str.str, str.len, str.cap, src[i]);
  }
  ARRAY_PUSH(str.str, str.len, str.cap, '\0');
  return str.str;
}

static inline bool can_cat(char left, char right, bool is_trans) {
  if (!is_trans && (strchr(Mid_Sym, left) != NULL || strchr(Mid_Sym, right) != NULL)) return false;
  if (!is_trans && strchr(Left_Sym, left) != NULL) return false;
  if (strchr(Right_Sym, right) != NULL) return false;
  return true;
}

static char* _parse_src(Pattern *dst, char* src, char cat_sym, bool is_root) {
  for (; src[0] != '\0'; src++) {
    if (can_cat(dst->str[dst->len - 1], src[0], is_trans(dst))) {
      ARRAY_PUSH(dst->str, dst->len, dst->cap, cat_sym);
      ARRAY_PUSH(dst->str, dst->len, dst->cap, src[0]);
    } else {
      ARRAY_PUSH(dst->str, dst->len, dst->cap, src[0]);
    }
    if (src[0] == ')' && !is_trans(dst)) {
      if (!is_root) return src;
    };
    if (src[0] == '(' && !is_trans(dst)) src = _parse_src(dst, src+1, '&', false);
    if (src[0] == ']' && !is_trans(dst)) {
      if (!is_root) return src;
    }
    if (src[0] == '[' && !is_trans(dst)) src = _parse_src(dst, src+1, '|', false);
  }
  if (is_root) ARRAY_PUSH(dst->str, dst->len, dst->cap, '\0');
  return src;
}

char* parse_src(char* origin_src) {
  char* src = pre_parse_src(origin_src);
  Pattern dst = {0, 32, NULL};
  dst.str = calloc(dst.cap, sizeof(char));
  ARRAY_PUSH(dst.str, dst.len, dst.cap, src[0]);
  if (dst.str[0] == '[') _parse_src(&dst, src+1, '|', true);
  else _parse_src(&dst, src+1, '&', true);
  free(src);
  return dst.str;
}

#define VALUE_NOT_FOUND NULL

static void add_line(NfaState* from, char ch, NfaState* to) {
  Set* temp_value = VALUE_NOT_FOUND;
  HT_GET(from->next_state, from->len, from->cap, ((char[]){ch, '\0'}), temp_value);
  if (temp_value == VALUE_NOT_FOUND) {
    temp_value = calloc(1, sizeof(Set));
    HT_PUT(from->next_state, from->len, from->cap, ((char[]){ch,'\0'}), temp_value);
  }
  ARRAY_PUSH(temp_value->set, temp_value->len, temp_value->cap, to);
}

static NfaState* create_nfastate(bool is_end) {
  NfaState* a = malloc(sizeof(NfaState));
  a->is_end = is_end;
  a->freed = false;
  a->len = 0;
  a->cap = 4;
  a->next_state = calloc(a->cap, sizeof(KV*));
  return a;
}

static NFA create_nfa(NfaState* start, NfaState* end) {
  return (NFA){start, end};
}

static NFA create_nfa_single(char ch) {
  NfaState* a = create_nfastate(false);
  NfaState* b = create_nfastate(true);
  add_line(a, ch, b);
  return create_nfa(a, b);
}

static NFA nfa_concat(NFA start, NFA end) {
  add_line(start.end, epsilon, end.start);
  start.end->is_end = false;
  return create_nfa(start.start, end.end);
}

static NFA nfa_union(NFA left, NFA right) {
  NfaState* start = create_nfastate(false);
  NfaState* end = create_nfastate(true);
  add_line(start, epsilon, left.start);
  add_line(start, epsilon, right.start);
  add_line(left.end, epsilon, end);
  left.end->is_end = false;
  add_line(right.end, epsilon, end);
  right.end->is_end = false;
  return create_nfa(start, end);
}

static NFA nfa_closure(NFA a) {
  NfaState* start = create_nfastate(false);
  NfaState* end = create_nfastate(true);
  add_line(start, epsilon, a.start);
  add_line(a.end, epsilon, end);
  a.end->is_end = false;
  add_line(start, epsilon, end);
  add_line(a.end, epsilon, a.start);
  return create_nfa(start, end);
}

static NFA nfa_plus(NFA a) {
  add_line(a.end, epsilon, a.start);
  return a;
}

static NFA nfa_optional(NFA a) {
  NfaState* start = create_nfastate(false);
  NfaState* end = create_nfastate(true);
  add_line(start, epsilon, a.start);
  add_line(a.end, epsilon, end);
  a.end->is_end = false;
  add_line(start, epsilon, end);
  return create_nfa(start, end);
}

static inline enum Op trans_op(char sym) {
  switch (sym) {
  case '&':
    return CONCAT;
  case '|':
    return UNION;
  case '*':
    return CLOSURE;
  case '+':
    return PLUS;
  case '?':
    return OPTIONAL;
  case '[':
  case '(':
    return LP;
  case ']':
  case ')':
    return RP;
  }
  assert(0);
  return NUL;
}

typedef struct {size_t len; size_t cap; enum Op* stack;} Sym_Stack;
typedef struct {size_t len; size_t cap; NFA* stack;} Nfa_Stack;

static NFA nfa_compile(Sym_Stack *sym_stack, Nfa_Stack *nfa_stack, enum Op sym) {
  NFA a, b, c;
  switch (sym) {
  case NUL:
  case LP:
    assert(0);
    break;
  case RP:
    NOSHRINK_POP(sym_stack->stack, sym_stack->len, sym_stack->cap, sym);
    while (sym != LP) {
      c = nfa_compile(sym_stack, nfa_stack, sym);
      ARRAY_PUSH(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, c);
      NOSHRINK_POP(sym_stack->stack, sym_stack->len, sym_stack->cap, sym);
    }
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, c);
    break;
  case CONCAT:
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, b);
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, a);
    c = nfa_concat(a, b);
    break;
  case UNION:
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, b);
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, a);
    c = nfa_union(a, b);
    break;
  case CLOSURE:
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, a);
    c = nfa_closure(a);
    break;
  case PLUS:
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, a);
    c = nfa_plus(a);
    break;
  case OPTIONAL:
    NOSHRINK_POP(nfa_stack->stack, nfa_stack->len, nfa_stack->cap, a);
    c = nfa_optional(a);
    break;
  }
  return c;
};

NFA compile_nfa(char* dst) {
  Sym_Stack sym_stack = {0, 8, NULL};
  sym_stack.stack = calloc(8, sizeof(enum Op));
  ARRAY_PUSH(sym_stack.stack, sym_stack.len, sym_stack.cap, NUL);
  Nfa_Stack nfa_stack = {0, 8, NULL};
  nfa_stack.stack = calloc(8, sizeof(NFA));
  for (size_t i = 0; i < strlen(dst); i++) {
    if (strchr(Sym, dst[i]) != NULL) {
      enum Op prev_sym, sym = trans_op(dst[i]);
      NOSHRINK_POP(sym_stack.stack, sym_stack.len, sym_stack.cap, prev_sym);
      while (Op_Priory[prev_sym] >= Op_Priory[sym] && sym != LP) {
	NFA nfa = nfa_compile(&sym_stack, &nfa_stack, prev_sym);
	ARRAY_PUSH(nfa_stack.stack, nfa_stack.len, nfa_stack.cap, nfa);
	NOSHRINK_POP(sym_stack.stack, sym_stack.len, sym_stack.cap, prev_sym);
      }
      ARRAY_PUSH(sym_stack.stack, sym_stack.len, sym_stack.cap, prev_sym);
      ARRAY_PUSH(sym_stack.stack, sym_stack.len, sym_stack.cap, sym);
    }
    else if (dst[i] == '\\') {
      i++;
      ARRAY_PUSH(nfa_stack.stack, nfa_stack.len, nfa_stack.cap, create_nfa_single(dst[i]));
    }
    else {
      ARRAY_PUSH(nfa_stack.stack, nfa_stack.len, nfa_stack.cap, create_nfa_single(dst[i]));
    }
  }
  enum Op sym;
  NOSHRINK_POP(sym_stack.stack, sym_stack.len, sym_stack.cap, sym);
  while (sym != NUL) {
    NFA nfa = nfa_compile(&sym_stack, &nfa_stack, sym);
    ARRAY_PUSH(nfa_stack.stack, nfa_stack.len, nfa_stack.cap, nfa);
    NOSHRINK_POP(sym_stack.stack, sym_stack.len, sym_stack.cap, sym);
  }
  NFA res;
  NOSHRINK_POP(nfa_stack.stack, nfa_stack.len, nfa_stack.cap, res);
  free(sym_stack.stack);
  free(nfa_stack.stack);
  return res;
}

static Set epsilon_closure(NfaState* state) {
  Set stack = {0, 8, NULL}; stack.set = calloc(stack.cap, sizeof(typeof(stack.set[0])));
  ARRAY_PUSH(stack.set, stack.len, stack.cap, state);
  Set closure = {0, 8, NULL}; closure.set = calloc(closure.cap, sizeof(typeof(closure.set[0])));
  ARRAY_PUSH(closure.set, closure.len, closure.cap, state);
  while (stack.len > 0) {
    NfaState* current_state;
    NOSHRINK_POP(stack.set, stack.len, stack.cap, current_state);
    Set* next_state = &(Set){0, 0, NULL};
    HT_GET(current_state->next_state, current_state->len, current_state->cap, ((char[]){epsilon, '\0'}), next_state);
    for (size_t i = 0; i < next_state->len; i++) {
      for (size_t j = 0; j < closure.len; j++) {
	if (closure.set[j] == next_state->set[i]) goto bk;
      }
      ARRAY_PUSH(closure.set, closure.len, closure.cap, next_state->set[i]);
      ARRAY_PUSH(stack.set, stack.len, stack.cap, next_state->set[i]);
    bk:
      ;;
    }
  }
  free(stack.set);
  return closure;
}

static Set epsilon_closure_set(Set states) {
  Set closure = {0, 16, NULL};
  closure.set = calloc(closure.cap, sizeof(typeof(closure.set[0])));
  for (size_t i = 0; i < states.len; i++) {
    Set closure_singel = epsilon_closure(states.set[i]);
    for (size_t j = 0; j < closure_singel.len; j++) {
      ARRAY_PUSH(closure.set, closure.len, closure.cap, closure_singel.set[j]);
    }
    free(closure_singel.set);
  }
  return closure;
}

bool nfa_match(NFA nfa, char* src) {
  Set current_states = epsilon_closure(nfa.start);
  for (size_t i = 0; i < strlen(src); i++) {
    Set next_states = {0, 16, NULL};
    next_states.set = calloc(next_states.cap, sizeof(typeof(next_states.set[0])));
    for (size_t j = 0; j < current_states.len; j++) {
      Set* sts = NULL;
      HT_GET(current_states.set[j]->next_state,
	     current_states.set[j]->len,
	     current_states.set[j]->cap,
	     ((char[]){src[i], '\0'}),
	     sts);
      HT_GET(current_states.set[j]->next_state, /* NOTE: 不论输入什么符号, '.'都一定会被匹配，实现通配符 */
	     current_states.set[j]->len,
	     current_states.set[j]->cap,
	     ((char[]){'.', '\0'}),
	     sts);
      if (sts == NULL) continue;
      else {
	for (size_t k = 0; k < sts->len; k++) {
	  ARRAY_PUSH(next_states.set, next_states.len, next_states.cap, sts->set[k]);
	}
      }
    }
    free(current_states.set);
    current_states = epsilon_closure_set(next_states);
    if (current_states.len == 0) {
      free(current_states.set);
      free(next_states.set);
      return false;
    }
    free(next_states.set);
  }
  for (size_t i = 0; i < current_states.len; i++) {
    if (current_states.set[i]->is_end) {
      free(current_states.set);
      return true;
    }
  }
  free(current_states.set);
  return false;
}

/* NfaState -> KV -> Set -> NfaState*/
static void collect_nodes(NfaState* head, Set* nodes) {
  if (head == NULL || head->freed) return;
  head->freed = true;
  ARRAY_PUSH(nodes->set, nodes->len, nodes->cap, head);
  for (size_t i = 0; i < head->cap; i++) {
    KV* kv = head->next_state[i];
    while (kv) {
      if (kv->value) {
	for (size_t j = 0; j < kv->value->len; j++) {
	  collect_nodes(kv->value->set[j], nodes);
	}
      }
      kv = kv->next;
    }
  }
}

static void free_collect_nodes(Set* nodes) {
  for (size_t i = 0; i < nodes->len; i++) {
    NfaState* head = nodes->set[i];
    for (size_t j = 0; j < head->cap; j++) {
      KV *next_kv, *kv = head->next_state[j];
      while (kv) {
	next_kv = kv->next;
	if (kv->value) {
	  free(kv->value->set);
	  free(kv->value);
	}
	free(kv);
	kv = next_kv;
      }
    }
    free(head->next_state);
    free(head);
  }
}

void dfs_free_nfa(NfaState* head) {
  Set nodes = {0, 64, NULL};
  nodes.set = calloc(nodes.cap, sizeof(NfaState*));
  collect_nodes(head, &nodes);
  free_collect_nodes(&nodes);
  free(nodes.set);
}

bool re_match(char* pattern, char* src) {
  char* dst = parse_src(pattern);
  NFA nfa = compile_nfa(dst);
  bool res = nfa_match(nfa, src);
  dfs_free_nfa(nfa.start);
  free(dst);
  return res;
}

NFA compile_regex(char* src) {
  char* dst = parse_src(src);
  NFA nfa = compile_nfa(dst);
  free(dst);
  return nfa;
}

bool regex_match(NFA nfa, char* string) {
  return nfa_match(nfa, string);
}

void free_regex(NFA nfa) {
  dfs_free_nfa(nfa.start);
}
