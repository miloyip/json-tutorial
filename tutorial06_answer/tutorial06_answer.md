# 从零开始的 JSON 库教程（六）：解析对象解答篇

* Milo Yip
* 2016/11/15

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第六个单元解答篇。解答代码位于 [json-tutorial/tutorial06_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial06_answer)。

## 1. 重构 `lept_parse_string()`

这个「提取方法」重构练习很简单，只需要把原来调用 `lept_set_string` 的地方，改为写入参数变量。因此，原来的 `lept_parse_string()` 和 答案中的 `lept_parse_string_raw()` 的 diff 仅是两处：

~~~
130,131c130,131
< static int lept_parse_string(lept_context* c, lept_value* v) {
<     size_t head = c->top, len;
---
> static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
>     size_t head = c->top;
140,141c140,141
<                 len = c->top - head;
<                 lept_set_string(v, (const char*)lept_context_pop(c, len), len);
---
>                 *len = c->top - head;
>                 *str = lept_context_pop(c, *len);
~~~

以 TDD 方式开发软件，因为有单元测试确保软件的正确性，面对新需求可以安心重构，改善软件架构。

## 2. 实现 `lept_parse_object()`

有了 `lept_parse_array()` 的经验，实现 `lept_parse_object()` 几乎是一样的，分别只是每个迭代要多处理一个键和冒号。我们把这个实现过程分为 5 步曲。

第 1 步是利用刚才重构出来的 `lept_parse_string_raw()` 去解析键的字符串。由于 `lept_parse_string_raw()` 假设第一个字符为 `"`，我们要先作校检，失败则要返回 `LEPT_PARSE_MISS_KEY` 错误。若字符串解析成功，它会把结果存储在我们的栈之中，需要把结果写入临时 `lept_member` 的 `k` 和 `klen` 字段中：

~~~c
static int lept_parse_object(lept_context* c, lept_value* v) {
    size_t i, size;
    lept_member m;
    int ret;
    /* ... */
    m.k = NULL;
    size = 0;
    for (;;) {
        char* str;
        lept_init(&m.v);
        /* 1. parse key */
        if (*c->json != '"') {
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen + 1);
        /* 2. parse ws colon ws */
        /* ... */
    }
    /* 5. Pop and free members on the stack */
    /* ... */
}
~~~

第 2 步是解析冒号，冒号前后可有空白字符：

~~~c
        /* 2. parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') {
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
~~~

第 3 步是解析任意的 JSON 值。这部分与解析数组一样，递归调用 `lept_parse_value()`，把结果写入临时 `lept_member` 的 `v` 字段，然后把整个 `lept_member` 压入栈：

~~~c
        /* 3. parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */
~~~

但有一点要注意，如果之前缺乏冒号，或是这里解析值失败，在函数返回前我们要释放 `m.k`。如果我们成功地解析整个成员，那么就要把 `m.k` 设为空指针，其意义是说明该键的字符串的拥有权已转移至栈，之后如遇到错误，我们不会重覆释放栈里成员的键和这个临时成员的键。

第 4 步，解析逗号或右花括号。遇上右花括号的话，当前的 JSON 对象就解析完结了，我们把栈上的成员复制至结果，并直接返回：

~~~c
        /* 4. parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(lept_member) * size;
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
~~~

最后，当 `for (;;)` 中遇到任何错误便会到达这第 5 步，要释放临时的 key 字符串及栈上的成员：

~~~c
    /* 5. Pop and free members on the stack */
    free(m.k);
    for (i = 0; i < size; i++) {
        lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;
    return ret;
~~~

注意我们不需要先检查 `m.k != NULL`，因为 `free(NULL)` 是完全合法的。

## 3. 释放内存

使用工具检测内存泄漏时，我们会发现以下这行代码造成内存泄漏：

~~~c
memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
~~~

类似数组，我们也需要在 `lept_free()` 释放 JSON 对象的成员（包括键及值）：

~~~c
void lept_free(lept_value* v) {
    size_t i;
    assert(v != NULL);
    switch (v->type) {
        /* ... */
        case LEPT_OBJECT:
            for (i = 0; i < v->u.o.size; i++) {
                free(v->u.o.m[i].k);
                lept_free(&v->u.o.m[i].v);
            }
            free(v->u.o.m);
            break;
        default: break;
    }
    v->type = LEPT_NULL;
}
~~~

## 4. 总结

至此，你已实现一个完整的 JSON 解析器，可解析任何合法的 JSON。统计一下，不计算空行及注释，现时 `leptjson.c` 只有 405 行代码，`lept_json.h` 54 行，`test.c` 309 行。

另一方面，一些程序也需要生成 JSON。也许最初读者会以为生成 JSON 只需直接 `sprintf()/fprintf()` 就可以，但深入了解 JSON 的语法之后，我们应该知道 JSON 语法还是需做一些处理，例如字符串的转义、数字的格式等。然而，实现生成器还是要比解析器容易得多。而且，假设我们有一个正确的解析器，可以简单使用 roundtrip 方式实现测试。请期待下回分解。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
