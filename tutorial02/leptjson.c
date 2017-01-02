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

static int lept_parse_literal(lept_context* c, lept_value* v, 
                              const char* json, int type) {
    int i, len = strlen(json);
    for(i = 0; i < len; ++i) {
        if(c->json[i] != json[i]) return LEPT_PARSE_INVALID_VALUE;
    }
    v->type = type;
    c->json += len;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    const char* ptr = c->json;
    if(*ptr == '-') ++ptr;
    if(!ISDIGIT(*ptr)) return LEPT_PARSE_INVALID_VALUE;
    if(*ptr == '0') {
        if(ptr[1] != '\0' && ptr[1] != '.') return LEPT_PARSE_ROOT_NOT_SINGULAR; /*after zero should be '.' or nothing*/
    }
    while(*ptr != '\0' && ISDIGIT(*ptr)) ptr++;
    if(*ptr != '\0') {
        if(*ptr == '.') {
            ++ptr;
            if(*ptr == '\0') return LEPT_PARSE_INVALID_VALUE; /*at least one digit after '.'*/
        }
        while(*ptr != '\0' && ISDIGIT(*ptr)) ++ptr;
        if(*ptr != '\0') {
            if(*ptr != 'E' && *ptr != 'e') return LEPT_PARSE_INVALID_VALUE;
            ++ptr;
            if(!ISDIGIT(*ptr)) {
                if(*ptr != '+' && *ptr != '-') return LEPT_PARSE_INVALID_VALUE;
                ++ptr;
            }
            while(*ptr != '\0' && ISDIGIT(*ptr)) ++ptr;
        }
    }
    v->n = strtod(c->json, &end);
    if(!isfinite(v->n)) {
        /* overflow */
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
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
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
