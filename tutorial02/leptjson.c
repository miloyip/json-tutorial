#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <string.h>
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT0(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context * c, lept_value * v, char prefix){
    c->json++;
    switch(prefix){
        case 'f': if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
                return LEPT_PARSE_INVALID_VALUE; break;
        case 't': if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
                return LEPT_PARSE_INVALID_VALUE; break;
        case 'n': if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
                return LEPT_PARSE_INVALID_VALUE; break;
    }
    c->json += 3;
    if(prefix == 'f') c->json++;
    switch (prefix) {
        case 'f': v->type = LEPT_FALSE; break;
        case 't': v->type = LEPT_TRUE; break;
        case 'n': v->type = LEPT_NULL; lept_parse_whitespace(c);
            if(*c->json != '\0') return LEPT_PARSE_ROOT_NOT_SINGULAR;break;
    }
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
    //errno是记录系统中存在错误的变量，只有当库函数出错的时候才会出现，设置为其他值代表不同的错误
    errno = 0;
    v->n = strtod(c->json, &end);
    //范围过大并且INF, inf这些宏在C语言里头是合法的，但是在JSON里头是INVALID的，所以解析结果会不一样
    if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) return LEPT_PARSE_NUMBER_TOO_BIG;
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    //开头的非法字符
    //the grammer is number = [ "-" ] int [ frac ] [ exp ]
    if(*c->json == '-') c->json++;
    if(!ISDIGIT0(*c->json)) return LEPT_PARSE_INVALID_VALUE;
    //0 end with other numbers
    if(*c->json == '0' && (c->json[1] != '\0' && c->json[1] != 'e' && c->json[1]
    != 'E' && c->json[1] != '.')) return LEPT_PARSE_ROOT_NOT_SINGULAR;
    while(ISDIGIT1(*c->json)) c->json++;
    //1.234E+10 case
    if(*c->json == '.') {
        c->json++;
        if(*c->json == '\0') return LEPT_PARSE_INVALID_VALUE;
    }
    while(ISDIGIT0(*c->json)) c->json++;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, 't');
        case 'f':  return lept_parse_literal(c, v, 'f');
        case 'n':  return lept_parse_literal(c, v, 'n');
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);

    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
