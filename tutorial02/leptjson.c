#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <math.h>

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

static int lept_parse_literal(lept_context* c, lept_value* v, const char* dest, lept_type type) {
	EXPECT(c, *(dest++));
	int ii = 0;
	while (dest[ii] != '\0')
	{
		if (c->json[ii] != dest[ii])
			return LEPT_PARSE_INVALID_VALUE;
		++ii;
	}
	c->json += ii;
	v->type = type;
	return LEPT_PARSE_OK;
}

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

static int lept_parse_number(lept_context* c, lept_value* v) {
    /* \TODO validate number */
	const char* p = c->json;
	char* des;
	//["-"]
	if (*p == '-')
		++p;
	//"0" / digit1-9 *digit
	if (*p == '0')
		++p;
	else if (ISDIGIT1TO9(*p))
	{
		while (ISDIGIT(*(++p)));
	}
	else
		return LEPT_PARSE_INVALID_VALUE;
	//["." 1*digit]
	if (*p == '.')
	{
		++p;
		if (!ISDIGIT(*p))
			return LEPT_PARSE_INVALID_VALUE;
		else
		{
			while (ISDIGIT(*(++p)));
		}
	}
	//[("e" / "E") ["+" \ "-"] 1*digit]
	if (*p == 'e' || *p == 'E')
	{
		++p;
		if (*p == '+' || *p == '-')
			++p;
		if (!ISDIGIT(*p))
			return LEPT_PARSE_INVALID_VALUE;
		else
		{
			while (ISDIGIT(*(++p)));
		}
	}
	des = (char*)malloc((p - c->json + 1) * sizeof(char));
	strncpy(des, c->json, p - c->json);
	des[p - c->json] = '\0';
    v->n = strtod(des, NULL);
	free(des);
	if (v->n == HUGE_VAL || v->n == -HUGE_VAL)
		return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
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
