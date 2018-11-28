# 从零开始的 JSON 库教程（一）：启程解答篇

* Milo Yip
* 2016/9/17

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第一个单元解答篇。解答代码位于 [json-tutorial/tutorial01_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial01_answer/)。

## 1. 修正 LEPT_PARSE_ROOT_NOT_SINGULAR

单元测试失败的是这一行：

~~~c
EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
~~~

我们从 JSON 语法发现，JSON 文本应该有 3 部分：

~~~
JSON-text = ws value ws
~~~

但原来的 `lept_parse()` 只处理了前两部分。我们只需要加入第三部分，解析空白，然后检查 JSON 文本是否完结：

~~~c
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
~~~

有一些 JSON 解析器完整解析一个值之后就会顺利返回，这是不符合标准的。但有时候也有另一种需求，文本中含多个 JSON 或其他文本串接在一起，希望当完整解析一个值之后就停下来。因此，有一些 JSON 解析器会提供这种选项，例如 RapidJSON 的 `kParseStopWhenDoneFlag`。

## 2. true/false 单元测试

此问题很简单，只需参考 `test_parse_null()` 加入两个测试函数：

~~~c
static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}
~~~

但要记得在上一级的测试函数 `test_parse()` 调用这函数，否则会不起作用。还好如果我们记得用 `static` 修饰这两个函数，编译器会发出警告：

~~~
test.c:30:13: warning: unused function 'test_parse_true' [-Wunused-function]
static void test_parse_true() {
            ^
~~~

因为 static 函数的意思是指，该函数只作用于编译单元中，那么没有被调用时，编译器是能发现的。

### 3. true/false 解析

这部分很简单，只要参考 `lept_parse_null()`，再写两个函数，然后在 `lept_parse_value` 按首字符分派。

~~~c
static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);
        case 'n':  return lept_parse_null(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}
~~~

其实这 3 种类型都是解析字面量，可以使用单一个函数实现，例如用这种方式调用：

~~~c
        case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
~~~

这样可以减少一些重复代码，不过可能有少许额外性能开销。

## 4. 总结

如果你能完成这个练习，恭喜你！我想你通过亲自动手，会对教程里所说的有更深入的理解。如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。

下一单元是和数字类型相关，敬请期待。
