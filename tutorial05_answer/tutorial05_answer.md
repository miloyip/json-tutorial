# 从零开始的 JSON 库教程（五）：解析数组解答篇

* Milo Yip
* 2016/10/13

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第五个单元解答篇。解答代码位于 [json-tutorial/tutorial05_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial05_answer)。

## 1. 编写 `test_parse_array()` 单元测试

这个练习纯粹为了熟习数组的访问 API。新增的第一个 JSON 只需平凡的检测。第二个 JSON 有特定模式，第 i 个子数组的长度为 i，每个子数组的第 j 个元素是数字值 j，所以可用两层 for 循环测试。

~~~c
static void test_parse_array() {
    size_t i, j;
    lept_value v;

    /* ... */

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    for (i = 0; i < 4; i++) {
        lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
        for (j = 0; j < i; j++) {
            lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));
        }
    }
    lept_free(&v);
}
~~~

## 2. 解析空白字符

按现时的 `lept_parse_array()` 的编写方式，需要加入 3 个 `lept_parse_whitespace()` 调用，分别是解析 `[` 之后，元素之后，以及 `,` 之后：

~~~c
static int lept_parse_array(lept_context* c, lept_value* v) {
    /* ... */
    EXPECT(c, '[');
    lept_parse_whitespace(c);
    /* ... */
    for (;;) {
        /* ... */
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            return ret;
        /* ... */
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }
        /* ... */
    }
}
~~~

## 3. 内存泄漏

成功测试那 3 个 JSON 后，使用内存泄漏检测工具会发现 `lept_parse_array()` 用 `malloc()`分配的内存没有被释放：

~~~
==154== 124 (120 direct, 4 indirect) bytes in 1 blocks are definitely lost in loss record 2 of 4
==154==    at 0x4C28C20: malloc (vg_replace_malloc.c:296)
==154==    by 0x409D82: lept_parse_array (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x409E91: lept_parse_value (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x409F14: lept_parse (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x405261: test_parse_array (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x408C72: test_parse (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x40916A: main (in /json-tutorial/tutorial05/build/leptjson_test)
==154== 
==154== 240 (96 direct, 144 indirect) bytes in 1 blocks are definitely lost in loss record 4 of 4
==154==    at 0x4C28C20: malloc (vg_replace_malloc.c:296)
==154==    by 0x409D82: lept_parse_array (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x409E91: lept_parse_value (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x409F14: lept_parse (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x40582C: test_parse_array (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x408C72: test_parse (in /json-tutorial/tutorial05/build/leptjson_test)
==154==    by 0x40916A: main (in /json-tutorial/tutorial05/build/leptjson_test)
~~~

很明显，有 `malloc()` 就要有对应的 `free()`。正确的释放位置应该放置在 `lept_free()`，当值被释放时，该值拥有的内存也在那里释放。之前字符串的释放也是放在这里：

~~~c
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}
~~~

但对于数组，我们应该先把数组内的元素通过递归调用 `lept_free()` 释放，然后才释放本身的 `v->u.a.e`：

~~~c
void lept_free(lept_value* v) {
    size_t i;
    assert(v != NULL);
    switch (v->type) {
        case LEPT_STRING:
            free(v->u.s.s);
            break;
        case LEPT_ARRAY:
            for (i = 0; i < v->u.a.size; i++)
                lept_free(&v->u.a.e[i]);
            free(v->u.a.e);
            break;
        default: break;
    }
    v->type = LEPT_NULL;
}
~~~

修改之后，再运行内存泄漏检测工具，确保问题已被修正。

## 4. 解析错误时的内存处理

遇到解析错误时，我们可能在之前已压入了一些值在自定议堆栈上。如果没有处理，最后会在 `lept_parse()` 中发现堆栈上还有一些值，做成断言失败。所以，遇到解析错误时，我们必须弹出并释放那些值。

在 `lept_parse_array` 中，原本遇到解析失败时，会直接返回错误码。我们把它改为 `break` 离开循环，在循环结束后的地方用 `lept_free()` 释放从堆栈弹出的值，然后才返回错误码：

~~~c
static int lept_parse_array(lept_context* c, lept_value* v) {
    /* ... */
    for (;;) {
        /* ... */
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            break;
        /* ... */
        if (*c->json == ',') {
            /* ... */
        }
        else if (*c->json == ']') {
            /* ... */
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /* Pop and free values on the stack */
    for (i = 0; i < size; i++)
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
    return ret;
}
~~~

## 5. bug 的解释

这个 bug 源于压栈时，会获得一个指针 `e`，指向从堆栈分配到的空间：

~~~c
    for (;;) {
        /* bug! */
        lept_value* e = lept_context_push(c, sizeof(lept_value));
        lept_init(e);
        size++;
        if ((ret = lept_parse_value(c, e)) != LEPT_PARSE_OK)
            return ret;
        /* ... */
    }
~~~

然后，我们把这个指针调用 `lept_parse_value(c, e)`，这里会出现问题，因为 `lept_parse_value()` 及之下的函数都需要调用 `lept_context_push()`，而 `lept_context_push()` 在发现栈满了的时候会用 `realloc()` 扩容。这时候，我们上层的 `e` 就会失效，变成一个悬挂指针（dangling pointer），而且 `lept_parse_value(c, e)` 会通过这个指针写入解析结果，造成非法访问。

在使用 C++ 容器时，也会遇到类似的问题。从容器中取得的迭代器（iterator）后，如果改动容器内容，之前的迭代器会失效。这里的悬挂指针问题也是相同的。

但这种 bug 有时可能在简单测试中不能自动发现，因为问题只有堆栈满了才会出现。从测试的角度看，我们需要一些压力测试（stress test），测试更大更复杂的数据。但从编程的角度看，我们要谨慎考虑变量的生命周期，尽量从编程阶段避免出现问题。例如把 `lept_context_push()` 的 API 改为：

~~~
static void lept_context_push(lept_context* c, const void* data, size_t size);
~~~

这样就确把数据压入栈内，避免了返回指针的生命周期问题。但我们之后会发现，原来的 API 设计在一些情况会更方便一些，例如在把字符串值转化（stringify）为 JSON 时，我们可以预先在堆栈分配字符串所需的最大空间，而当时是未有数据填充进去的。

无论如何，我们编程时都要考虑清楚变量的生命周期，特别是指针的生命周期。

## 6. 总结

经过对数组的解析，我们也了解到如何利用递归处理复合型的数据类型解析。与一些用链表或自动扩展的动态数组的实现比较，我们利用了自定义堆栈作为缓冲区，能分配最紧凑的数组作存储之用，会比其他实现更省内存。我们完成了数组类型后，只余下对象类型了。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
