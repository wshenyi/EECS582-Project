#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <emmintrin.h>
#include <immintrin.h>

#define LAYOUT_NAME "intro_1"
#define MAX_BUF_LEN 20

struct my_root {
  size_t len;
  char buf[MAX_BUF_LEN];
};
