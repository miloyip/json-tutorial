# 从零开始的 JSON 库教程（二）：解析数字解答篇

* Milo Yip
* 2016/9/20

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第二个单元解答篇。解答代码位于 [json-tutorial/tutorial02_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial02_answer/)。

## 1. 重构合并

由于 true / false / null 的字符数量不一样，这个答案以 for 循环作比较，直至 `'\0'`。

~~~c
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        /* ... */
    }
}
~~~

注意在 C 语言中，数组长度、索引值最好使用 `size_t` 类型，而不是 `int` 或 `unsigned`。

你也可以直接传送长度参数 4、5、4，只要能通过测试就行了。

## 2. 边界值测试

这问题其实涉及一些浮点数类型的细节，例如 IEEE-754 浮点数中，有所谓的 normal 和 subnormal 值，这里暂时不展开讨论了。以下是我加入的一些边界值，可能和同学的不完全一样。

~~~
TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
~~~

另外，这些加入的测试用例，正常的 `strtod()` 都能通过。所以不能做到测试失败、修改实现、测试成功的 TDD 步骤。

有一些 JSON 解析器不使用 `strtod()` 而自行转换，例如在校验的同时，记录负号、尾数（整数和小数）和指数，然后 naive 地计算：

~~~
int negative = 0;
int64_t mantissa = 0;
int exp = 0;

/* 解析... 并存储 negative, mantissa, exp */
v->n = (negative ? -mantissa : mantissa) * pow(10.0, exp);
~~~

这种做法会有精度问题。实现正确的答案是很复杂的，RapidJSON 的初期版本也是 naive 的，后来 RapidJSON 就内部实现了三种算法（使用 `kParseFullPrecision` 选项开启），最后一种算法用到了大整数（高精度计算）。有兴趣的同学也可以先尝试做一个 naive 版本，不使用 `strtod()`。之后可再参考 Google 的 [double-conversion](https://github.com/google/double-conversion) 开源项目及相关论文。

## 3. 校验数字

这条题目是本单元的重点，考验同学是否能把语法手写为校验规则。我详细说明。

首先，如同 `lept_parse_whitespace()`，我们使用一个指针 `p` 来表示当前的解析字符位置。这样做有两个好处，一是代码更简单，二是在某些编译器下性能更好（因为不能确定 `c` 会否被改变，从而每次更改 `c->json` 都要做一次间接访问）。如果校验成功，才把 `p` 赋值至 `c->json`。

~~~c
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    /* 负号 ... */
    /* 整数 ... */
    /* 小数 ... */
    /* 指数 ... */
    v->n = strtod(c->json, NULL);
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
~~~

我们把语法再看一遍：

~~~
number = [ "-" ] int [ frac ] [ exp ]
int = "0" / digit1-9 *digit
frac = "." 1*digit
exp = ("e" / "E") ["-" / "+"] 1*digit
~~~

负号最简单，有的话跳过便行：

~~~c
    if (*p == '-') p++;
~~~

整数部分有两种合法情况，一是单个 `0`，否则是一个 1-9 再加上任意数量的 digit。对于第一种情况，我们像负数般跳过便行。对于第二种情况，第一个字符必须为 1-9，如果否定的就是不合法的，可立即返回错误码。然后，有多少个 digit 就跳过多少个。

~~~c
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
~~~

如果出现小数点，我们跳过该小数点，然后检查它至少应有一个 digit，不是 digit 就返回错误码。跳过首个 digit，我们再检查有没有 digit，有多少个跳过多少个。这里用了 for 循环技巧来做这件事。

~~~c
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
~~~

最后，如果出现大小写 `e`，就表示有指数部分。跳过那个 `e` 之后，可以有一个正或负号，有的话就跳过。然后和小数的逻辑是一样的。

~~~c
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
~~~

这里用了 18 行代码去做这个校验。当中把一些 if 用一行来排版，而没用采用传统两行缩进风格，我个人认为在不影响阅读时可以这样弹性处理。当然那些 for 也可分拆成三行：

~~~c
        p++;
        while (ISDIGIT(*p))
            p++;
~~~

## 4. 数字过大的处理

最后这题纯粹是阅读理解题。

~~~c
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

static int lept_parse_number(lept_context* c, lept_value* v) {
    /* ... */
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && v->n == HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    /* ... */
}
~~~

许多时候课本／书籍也不会把每个标准库功能说得很仔细，我想藉此提醒同学要好好看参考文档，学会读文档编程就简单得多！[cppreference.com](http://cppreference.com) 是 C/C++ 程序员的宝库。

## 5. 总结

本单元的习题比上个单元较有挑战性一些，所以我花了多一些篇幅在解答篇。纯以语法来说，数字类型已经是 JSON 中最复杂的类型。如果同学能完成本单元的练习（特别是第 3 条），之后的字符串、数组和对象的语法一定难不到你。然而，接下来也会有一些新挑战，例如内存管理、数据结构、编码等，希望你能满载而归。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
