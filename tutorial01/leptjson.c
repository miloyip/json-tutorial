#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

//static int lept_parse_null(lept_context* c, lept_value* v) {
//    EXPECT(c, 'n');
//    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    lept_parse_whitespace(c); //跳过空格，然后看是否不为结束位置
//    if(c->json[0] != '\0'){
//        return LEPT_PARSE_ROOT_NOT_SINGULAR;
//    }
//    v->type = LEPT_NULL;
//    return LEPT_PARSE_OK;
//}
//static int lept_parse_true(lept_context * c, lept_value * v){
//    EXPECT(c, 't');
//    if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    v->type = LEPT_TRUE;
//    return LEPT_PARSE_OK;
//}
//
//static int lept_parse_false(lept_context * c, lept_value * v){
//    EXPECT(c, 'f');
//    if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 4;
//    v->type = LEPT_FALSE;
//    return LEPT_PARSE_OK;
//}

static int lept_parse_literal(lept_context * c, lept_value * v, char prefix){
    c->json++;
    switch(prefix){
        case 'f': if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
                return LEPT_PARSE_INVALID_VALUE; break;
        case 't': if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
                return LEPT_PARSE_INVALID_VALUE; break;
        case 'n': if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
                return LEPT_PARSE_INVALID_VALUE;
             break;
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

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, 'n');
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        case 't': return lept_parse_literal(c, v, 't');
        case 'f': return lept_parse_literal(c, v, 'f');
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}



int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    return lept_parse_value(&c, v);
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
