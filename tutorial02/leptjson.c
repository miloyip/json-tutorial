#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <math.h>
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0'&& (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type){
	size_t i;
	EXPECT(c, literal[0]);
	for(i=0; literal[i+1]!='\0' && c->json[i] == literal[i + 1]; i++);
	if(literal[i + 1] == '\0'){
		v->type = type;
		c->json +=i;
		return LEPT_PARSE_OK;
	}
	return LEPT_PARSE_INVALID_VALUE;
	
}
/*validate number and return where the number end*/
static int valid_number(const char* c){
	int i = 0;
	if(c[i] == '-'){
		i++;	
	}
	if(ISDIGIT(c[i])){
		if(ISDIGIT1TO9(c[i])){
			i++;
			while(ISDIGIT(c[i]))i++;
		}else i++;
		if(c[i] == '.'){
			i++;
			if(ISDIGIT(c[i]))
				while(ISDIGIT(c[i]))i++;
			else
				return 0;
		}
		if(c[i] == 'e' || c[i] == 'E'){
			i++;
			if(c[i] == '+' || c[i] == '-')i++;
			if(ISDIGIT(c[i]))
				while(ISDIGIT(c[i]))i++;
			else
				return 0;

		}
		return i;
	}
	return 0;
}
static int lept_parse_number(lept_context* c, lept_value* v) {
    int end;
    if((end = valid_number(c->json)) == 0)
	 return LEPT_PARSE_INVALID_VALUE;
    errno = 0;
    v->n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->n == HUGE_VAL|| v->n == -HUGE_VAL)){
	 v->type = LEPT_NULL;
	 return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json += end;
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
    } }

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
