/* TDD 先写测试，再实现功能 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                \
  do {                                                                  \
    test_count++;                                                       \
    if (equality)                                                       \
      test_pass++;                                                      \
    else {                                                              \
      fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
              __FILE__, __LINE__, expect, actual);                      \
      main_ret = 1;                                                     \
    }                                                                   \
  } while (0)

#define EXPECT_EQ_INT(expect, actual) \
  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null() {
  lept_value v;
  v.type = LEPT_TRUE;
  EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
  EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
  lept_value v;
  v.type = LEPT_NULL;
  EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
  EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
  lept_value v;
  v.type = LEPT_NULL;
  EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
  EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

#define TEST_ERROR(error, json)                  \
  do {                                           \
    lept_value v;                                \
    v.type = LEPT_FALSE;                         \
    EXPECT_EQ_INT(error, lept_parse(&v, json));  \
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v)); \
  } while (0)

static void test_parse_expect_value() {
  TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
  TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
  TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
  TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
}

static void test_parse_root_not_singular() {
  lept_value v;
  v.type = LEPT_FALSE;
  EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
  EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse() {
  test_parse_null();
  test_parse_true();
  test_parse_false();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
}

int main() {
  test_parse();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
         test_pass * 100.0 / test_count);
  return main_ret;
}
