#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "regex.h"

void test(char* pattern, char* src[], size_t counts) {
  char* dst = parse_src(pattern);
  printf("============\n%s: \n", dst);
  NFA nfa = compile_nfa(dst);
  for (size_t i = 0; i < counts; i++) {
    printf("%s\n", nfa_match(nfa, src[i]) ? "true" : "false");
  }
  dfs_free_nfa(nfa.start);
  free(dst);
}

int main() {
  char* dst = parse_src("[1234567890]+");
  printf("%s\n", dst);
  NFA nfa = compile_nfa(dst);
  printf(nfa_match(nfa, "123") ? "true\n" : "false\n");
  dfs_free_nfa(nfa.start);
  free(dst);
  
  /* test("[(12)345]+", ((char*[]){"123", "124", "13", "12"}), 4); */
  
  assert(regex_match("abc", "abc"));
  assert(regex_match("ab|c", "ab"));
  assert(regex_match("ab|c", "c"));
  assert(!regex_match("ab|c", "abc"));
  assert(regex_match(".", "a"));
  assert(regex_match(".bc", "abc"));
  assert(regex_match(".*bc", "aaabc"));
  assert(regex_match("a*bc", "aaabc"));
  assert(regex_match("a*bc", "bc"));
  assert(regex_match(".*", "aaa"));
  assert(regex_match(".*", "abc"));
  assert(regex_match("a+bc", "aaabc"));
  assert(!regex_match("a+bc", "bc"));
  assert(regex_match("abc+", "abccc"));
  assert(!regex_match("abc+", "ab"));
  assert(regex_match("(ab)*c", "c"));
  assert(regex_match("(a|b)c", "bc"));
  assert(regex_match("a(bc)+", "abcbcbc"));
  assert(regex_match("(ab)+c", "abababc"));
  assert(regex_match("a(bc|d)+", "abcbcbcddd"));
  assert(!regex_match("a(bc|d*)", "abcddd"));
  assert(regex_match("(1|2|3|4|5|6|7|8|9|0)+", "123"));
  assert(regex_match(".*abc.*", "123123123abc123123123"));
  assert(!regex_match(".*abc.*", "123123123bc123123123"));
  assert(regex_match("a[bcd]", "ab"));
  assert(regex_match("[1234567890]+", "123"));
  assert(!regex_match("[(12)345]+", "134"));
  
  puts("Success.");
  return 0;
}
