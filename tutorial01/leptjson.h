#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {//树节点的值
    lept_type type;
}lept_value;

enum {//lept_parse的枚举返回值，无误返回第一个
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

int lept_parse(lept_value* v, const char* json);//解析json的API

lept_type lept_get_type(const lept_value* v);//访问结果的API，就是获取类型

#endif /* LEPTJSON_H__ */
