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


static int lept_parse_literal(lept_context * c, lept_value * v, char prefix){
    c->json++;
    switch(prefix){
        case 'f': if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
                return LEPT_PARSE_INVALID_VALUE;
        case 't': if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
                return LEPT_PARSE_INVALID_VALUE;
        case 'n': if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
                return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 3;
    if(prefix == 'f') c->json++;
    switch (prefix) {
        case 'f': v->type = LEPT_FALSE;
        case 't': v->type = LEPT_TRUE;
        case 'n': v->type = LEPT_NULL;
    }
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context * c, lept_value * v){
    if(*c->json == '+') {
        return LEPT_PARSE_INVALID_VALUE;
    }
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, 't');
        case 'f':  return lept_parse_literal(c, v, 'f');
        case 'n':  return lept_parse_literal(c, v, 'n');
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v); //for numbers
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
        if (*c.json != '\0')
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
