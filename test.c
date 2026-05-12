#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "regex.h"

void test(char* pattern, char* src[], size_t counts) {
  printf("============\n%s: \n", pattern);
  NFA nfa = compile_regex(pattern);
  for (size_t i = 0; i < counts; i++) {
    printf("%s: %s\n", src[i], regex_match(nfa, src[i]) ? "true" : "false");
  }
  free_regex(nfa);
}

int main() {
  char* dst = parse_src("ab?c");
  printf("%s\n", dst);
  NFA nfa = compile_nfa(dst);
  printf(nfa_match(nfa, "ac") ? "true\n" : "false\n");
  dfs_free_nfa(nfa.start);
  free(dst);
  
  test("ab?c", ((char*[]){"ac", "abc", "c", "ab"}), 4);
  
  assert(re_match("abc", "abc"));
  assert(re_match("ab|c", "ab"));
  assert(re_match("ab|c", "c"));
  assert(!re_match("ab|c", "abc"));
  assert(re_match(".", "a"));
  assert(re_match(".bc", "abc"));
  assert(re_match(".*bc", "aaabc"));
  assert(re_match("a*bc", "aaabc"));
  assert(re_match("a*bc", "bc"));
  assert(re_match(".*", "aaa"));
  assert(re_match(".*", "abc"));
  assert(re_match("a+bc", "aaabc"));
  assert(!re_match("a+bc", "bc"));
  assert(re_match("abc+", "abccc"));
  assert(!re_match("abc+", "ab"));
  assert(re_match("(ab)*c", "c"));
  assert(re_match("(a|b)c", "bc"));
  assert(re_match("a(bc)+", "abcbcbc"));
  assert(re_match("(ab)+c", "abababc"));
  assert(re_match("a(bc|d)+", "abcbcbcddd"));
  assert(!re_match("a(bc|d*)", "abcddd"));
  assert(re_match("(1|2|3|4|5|6|7|8|9|0)+", "123"));
  assert(re_match(".*abc.*", "123123123abc123123123"));
  assert(!re_match(".*abc.*", "123123123bc123123123"));
  assert(re_match("a[bcd]", "ab"));
  assert(re_match("[1234567890]+", "123"));
  assert(!re_match("[(12)345]+", "134"));
  assert(re_match("[0-9]+", "123"));
  assert(re_match("[a-z]+", "abc"));
  assert(re_match("[a-z0-9]+", "abc123"));
  assert(re_match("[a-zA-Z0-9]+", "abcABC123"));
  assert(re_match("[abc0-9]+", "aabc123"));
  assert(!re_match("[abc0-9]+", "d123"));
  assert(re_match("ab?c", "ac"));
  assert(re_match("ab?c", "abc"));
  assert(re_match("a(bc)?d", "ad"));
  assert(re_match("a(bc)?d", "abcd"));
  assert(re_match("a(b|c)?d", "abd"));
  assert(!re_match("a(b|c)?d", "abcd"));
  
  puts("Success.");
  return 0;
}
