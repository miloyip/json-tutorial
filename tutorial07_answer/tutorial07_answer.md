# 从零开始的 JSON 库教程（七）：生成器解答篇

* Milo Yip
* 2017/1/5

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第七个单元解答篇。解答代码位于 [json-tutorial/tutorial07_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial07_answer)。

## 1. 生成字符串

我们需要对一些字符进行转义，最简单的实现如下：

~~~c
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
    size_t i;
    assert(s != NULL);
    PUTC(c, '"');
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': PUTS(c, "\\\"", 2); break;
            case '\\': PUTS(c, "\\\\", 2); break;
            case '\b': PUTS(c, "\\b",  2); break;
            case '\f': PUTS(c, "\\f",  2); break;
            case '\n': PUTS(c, "\\n",  2); break;
            case '\r': PUTS(c, "\\r",  2); break;
            case '\t': PUTS(c, "\\t",  2); break;
            default:
                if (ch < 0x20) {
                    char buffer[7];
                    sprintf(buffer, "\\u%04X", ch);
                    PUTS(c, buffer, 6);
                }
                else
                    PUTC(c, s[i]);
        }
    }
    PUTC(c, '"');
}

static void lept_stringify_value(lept_context* c, const lept_value* v) {
    switch (v->type) {
        /* ... */
        case LEPT_STRING: lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
        /* ... */
    }
}
~~~

注意到，十六进位输出的字母可以用大写或小写，我们这里选择了大写，所以 roundstrip 测试时也用大写。但这个并不是必然的，输出小写（用 `"\\u%04x"`）也可以。

## 2. 生成数组和对象

生成数组也是非常简单，只要输出 `[` 和 `]`，中间对逐个子值递归调用 `lept_stringify_value()`。只要注意在第一个元素后才加入 `,`。而对象也仅是多了一个键和 `:`。

~~~cs
static void lept_stringify_value(lept_context* c, const lept_value* v) {
    size_t i;
    switch (v->type) {
        /* ... */
        case LEPT_ARRAY:
            PUTC(c, '[');
            for (i = 0; i < v->u.a.size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                lept_stringify_value(c, &v->u.a.e[i]);
            }
            PUTC(c, ']');
            break;
        case LEPT_OBJECT:
            PUTC(c, '{');
            for (i = 0; i < v->u.o.size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
                PUTC(c, ':');
                lept_stringify_value(c, &v->u.o.m[i].v);
            }
            PUTC(c, '}');
            break;
        /* ... */
    }
}
~~~

## 3. 优化 `lept_stringify_string()`

最后，我们讨论一下优化。上面的 `lept_stringify_string()` 实现中，每次输出一个字符／字符串，都要调用 `lept_context_push()`。如果我们使用一些性能剖测工具，也可能会发现这个函数消耗较多 CPU。

~~~c
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) { // (1)
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;       // (2)
    c->top += size;                // (3)
    return ret;                    // (4)
}
~~~

中间最花费时间的，应该会是 (1)，需要计算而且作分支检查。即使使用 C99 的 `inline` 关键字（或使用宏）去减少函数调用的开销，这个分支也无法避免。

所以，一个优化的点子是，预先分配足够的内存，每次加入字符就不用做这个检查了。但多大的内存才足够呢？我们可以看到，每个字符可生成最长的形式是 `\u00XX`，占 6 个字符，再加上前后两个双引号，也就是共 `len * 6 + 2` 个输出字符。那么，使用 `char* p = lept_context_push()` 作一次分配后，便可以用 `*p++ = c` 去输出字符了。最后，再按实际输出量调整堆栈指针。

另一个小优化点，是自行编写十六进位输出，避免了 `printf()` 内解析格式的开销。

~~~c
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
    static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t i, size;
    char* head, *p;
    assert(s != NULL);
    p = head = lept_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
    *p++ = '"';
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': *p++ = '\\'; *p++ = '\"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b';  break;
            case '\f': *p++ = '\\'; *p++ = 'f';  break;
            case '\n': *p++ = '\\'; *p++ = 'n';  break;
            case '\r': *p++ = '\\'; *p++ = 'r';  break;
            case '\t': *p++ = '\\'; *p++ = 't';  break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    c->top -= size - (p - head);
}
~~~

要注意的是，很多优化都是有代价的。第一个优化采取空间换时间的策略，对于只含一个字符串的JSON，很可能会分配多 6 倍内存；但对于正常含多个值的 JSON，多分配的内存可在之后的值所利用，不会造成太多浪费。

而第二个优化的缺点，就是有稍增加了一点程序体积。也许有人会问，为什么 `hex_digits` 不用字符串字面量 `"0123456789ABCDEF"`？其实是可以的，但这会多浪费 1 个字节（实际因数据对齐可能会浪费 4 个或更多）。

## 4. 总结

我们用 80 行左右的代码就实现了 JSON 生成器，并尝试了做一些简单的优化。除了这种最简单的功能，有一些 JSON 库还会提供一些美化功能，即加入缩进及换行。另外，有一些应用可能需要大量输出数字，那么就可能需要优化数字的输出。这方面可考虑 C++ 开源库 [double-conversion](https://github.com/google/double-conversion)，以及参考本人另一篇文章《[RapidJSON 代码剖析（四）：优化 Grisu](https://zhuanlan.zhihu.com/p/20092285)》。

现时数组和对象类型只有最基本的访问、修改函数，我们会在下一篇补完。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
