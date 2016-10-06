# 从零开始的 JSON 库教程（四）：Unicode 解答篇

* Milo Yip
* 2016/10/6

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第四个单元解答篇。解答代码位于 [json-tutorial/tutorial04_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial04_answer)。

## 1. 实现 `lept_parse_hex4()`

这个函数只是读 4 位 16 进制数字，可以简单地自行实现：

~~~c
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}
~~~

可能有同学想到用标准库的 [`strtol()`](http://en.cppreference.com/w/c/string/byte/strtol)，因为它也能解析 16 进制数字，那么可以简短的写成：

~~~c
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    char* end;
    *u = (unsigned)strtol(p, &end, 16);
    return end == p + 4 ? end : NULL;
}
~~~

但这个实现会错误地接受 `"\u 123"` 这种不合法的 JSON，因为 `strtol()` 会跳过开始的空白。要解决的话，还需要检测第一个字符是否 `[0-9A-Fa-f]`，或者 `!isspace(*p)`。但为了 `strtol()` 做多余的检测，而且自行实现也很简单，我个人会选择首个方案。（前两个单元用 `strtod()` 就没辨法，因为它的实现要复杂得多。）

## 2. 实现 `lept_encode_utf8()`

这个函数只需要根据那个 UTF-8 编码表就可以实现：

~~~c
static void lept_encode_utf8(lept_context* c, unsigned u) {
    if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}
~~~

有同学可能觉得奇怪，最终也是写进一个 `char`，为什么要做 `x & 0xFF` 这种操作呢？这是因为 `u` 是 `unsigned` 类型，一些编译器可能会警告这个转型可能会截断数据。但实际上，配合了范围的检测然后右移之后，可以保证写入的是 0~255 内的值。为了避免一些编译器的警告误判，我们加上 `x & 0xFF`。一般来说，编译器在优化之后，这与操作是会被消去的，不会影响性能。

其实超过 1 个字符输出时，可以只调用 1 次 `lept_context_push()`。这里全用 `PUTC()` 只是为了代码看上去简单一点。

## 3. 代理对的处理

遇到高代理项，就需要把低代理项 `\uxxxx` 也解析进来，然后用这两个项去计算出码点：

~~~c
case 'u':
    if (!(p = lept_parse_hex4(p, &u)))
        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
    if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
        if (*p++ != '\\')
            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
        if (*p++ != 'u')
            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
        if (!(p = lept_parse_hex4(p, &u2)))
            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
        if (u2 < 0xDC00 || u2 > 0xDFFF)
            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
        u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
    }
    lept_encode_utf8(c, u);
    break;
~~~

## 4. 总结

JSON 的字符串解析终于完成了。但更重要的是，同学通过教程和练习后，应该对于 Unicode 和 UTF-8 编码有基本了解。使用 Unicode 标准去处理文本数据已是世界潮流。虽然 C11/C++11 引入了 Unicode 字符串字面量及少量函数，但仍然有很多不足，一般需要借助第三方库。

我们在稍后的单元还要处理生成时的 Unicode 问题，接下来我们要继续讨论数组和对象的解析。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
