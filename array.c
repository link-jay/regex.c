#include <stdlib.h>

#define ARRAY_PUSH(arr, len, cap, val)				\
  do {								\
    if ((cap) == 0) {						\
      (arr) = malloc(sizeof(*arr) * ((cap) = 4));		\
    } else if ((len) >= (cap)) {				\
      (arr) = realloc((arr), sizeof(*arr) * ((cap) *= 2));	\
    }								\
    (arr)[(len)++] = (val);					\
  } while(0)
  
#define ARRAY_POP(arr, len, cap, var)				\
  do {							        \
    if ((cap) == 0) {						\
      break;							\
    } else if ((len) <= (cap) / 4) {				\
      (arr) = realloc((arr), sizeof(*arr) * ((cap) /= 2));	\
    }							        \
    (var) = (arr)[--(len)];				        \
  } while(0)

#define ARRAY_DROP(arr, len, cap)				\
  do {							        \
    if ((cap) == 0) {						\
      break;							\
    } else if ((len) <= (cap) / 4) {				\
      (arr) = realloc((arr), sizeof(*arr) * ((cap) /= 2));	\
    }							        \
    (len)--;							\
  } while(0)
