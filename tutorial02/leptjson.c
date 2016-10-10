#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* l, lept_type type) {
    int i;
    EXPECT(c, *l);
    for(i=0; i<strlen(l)-1; i++) {
        if(c->json[i] != l[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += strlen(l)-1;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    int i;
    if(c->json[0] == '+' || c->json[0] == '.') 
        return LEPT_PARSE_INVALID_VALUE;
    if(strcmp(c->json, "INF")==0 || strcmp(c->json, "inf")==0 || strcmp(c->json, "NAN")==0 || strcmp(c->json, "nan")==0)
        return LEPT_PARSE_INVALID_VALUE;
    if(c->json[0] == '0' && c->json[1] != '.' && c->json[1] != '\0')
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    for(i=0; c->json[i]; i++) {
        if(c->json[i] == '.' && c->json[i+1] == '\0')
            return LEPT_PARSE_INVALID_VALUE;
    }
    v->n = strtod(c->json, &end);
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    if(v->n == HUGE_VAL || -v->n == HUGE_VAL)
       return LEPT_PARSE_NUMBER_TOO_BIG; 
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
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
    /*assert(v != NULL && v->type == LEPT_NUMBER);*/
    assert(v != NULL);
    assert(v->type == LEPT_NUMBER);
    return v->n;
}
