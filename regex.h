typedef struct KV KV;
typedef struct Set Set;
typedef struct NfaState NfaState;
typedef struct NFA NFA;

struct KV {
  struct KV* next;
  Set* value;
  char key[];
};

struct Set {
  size_t len;
  size_t cap;
  NfaState** set;
};

struct NfaState {
  bool is_end;
  bool freed;
  /* dict[str, set[NfaState,]] */
  size_t len;
  size_t cap;
  KV** next_state;
};

struct NFA {
  NfaState* start;
  NfaState* end;
};

char* parse_src(char*);
NFA compile_nfa(char*);
bool nfa_match(NFA, char*);
void dfs_free_nfa(NfaState*);
bool regex_match(char*, char*);
