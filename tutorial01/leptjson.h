#ifndef LEPTJSON_H_
#define LEPTJSON_H_

typedef enum {
  LEPT_NULL,
  LEPT_FALSE,
  LEPT_TRUE,
  LEPT_NUMBER,
  LEPT_STRING,
  LEPT_ARRAY,
  LEPT_OBJECT
} lept_type;

typedef struct {
  lept_type type;
} lept_value;

typedef struct {
  const char *json; /* data */
} lept_context;

enum {
  LEPT_PARSE_OK = 0,
  LEPT_PARSE_EXPECT_VALUE,
  LEPT_PARSE_INVALID_VALUE,
  LEPT_PARSE_ROOT_NOT_SINGULAR
};

int lept_parse(lept_value *v, const char *json);

lept_type lept_get_type(const lept_value *v);

#endif /* LEPTJSON_H_ */