#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct
{
	const char* json;
} lept_context;

static void lept_parse_whitespace(lept_context* c)
{
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_helper_check_value(lept_context *c,lept_value* v, const char * expected,lept_type type)
{
	assert(expected != NULL);
	EXPECT(c, expected[0]);
	int i = 1;
	while (expected[i] != '\0')
	{
		if (c->json[i-1] != expected[i])
		{
			return LEPT_PARSE_INVALID_VALUE;
		}
		i++;
	}
	c->json += (i-1);
	v->type = type;
	return LEPT_PARSE_OK;
}



static int lept_parse_null(lept_context* c, lept_value* v)
{
	return lept_helper_check_value(c,v, "null",LEPT_NULL);
}

static int lept_parse_true(lept_context* c, lept_value* v)
{
	
	return lept_helper_check_value(c,v,"true",LEPT_TRUE);

	
}

static int lept_parse_false(lept_context*c, lept_value* v)
{
	return lept_helper_check_value(c, v, "false", LEPT_FALSE);
}




static int lept_parse_value(lept_context* c, lept_value* v)
{
	switch (*c->json)
	{
	case 'n': return lept_parse_null(c, v);
	case 't': return lept_parse_true(c, v);
	case 'f':return lept_parse_false(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	default: return LEPT_PARSE_INVALID_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json)
{
	lept_context c;

	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	int ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK)
	{
		lept_parse_whitespace(&c);
		if (c.json[0] != '\0')
		{
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v)
{
	assert(v != NULL);
	return v->type;
}
