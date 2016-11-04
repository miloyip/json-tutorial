#ifndef LEPTJSON_H__
#define LEPTJSON_H__

/*
json中有6种数据类型，把true和false当成两种，共7种，用枚举表示，并定义为一个类型
*/
typedef enum {
  LEPT_NULL,
  LEPT_FALSE,
  LEPT_TRUE,
  LEPT_NUMBER,
  LEPT_STRING,
  LEPT_ARRAY
} lept_type;

/*
声明json的数据结构，每个节点使用lept_value来表示
*/
typedef struct { lept_type type; } lept_value;

/*
lept_parse的返回值类型
*/
enum {
  LEPT_PARSE_OK = 0,
  LEPT_PARSE_EXPECT_VALUE,
  LEPT_PARSE_INVALID_VALUE,
  LEPT_PARSE_ROOT_NOT_SINGULAR
};

/*
解析json函数
*/
int lept_parse(lept_value *v, const char *json);

/*
访问结果的函数，返回数据类型
*/
lept_type lept_get_type(const lept_value *v);
#endif // LEPTJSON_H__