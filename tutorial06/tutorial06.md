# 从零开始的 JSON 库教程（六）：解析对象

* Milo Yip
* 2016/10/29

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第六个单元。代码位于 [json-tutorial/tutorial06](https://github.com/miloyip/json-tutorial/blob/master/tutorial06)。

本单元内容：

1. [JSON 对象](#1-JSON-对象)
2. [数据结构](#2-数据结构)
3. [重构字符串解析](#3-重构字符串解析)
4. [实现](#4-实现)
5. [总结与练习](#5-总结与练习)

## 1. JSON 对象

此单元是本教程最后一个关于 JSON 解析器的部分。JSON 对象和 JSON 数组非常相似，区别包括 JSON 对象以花括号 `{}`（`U+007B`、`U+007D`）包裹表示，另外 JSON 对象由对象成员（member）组成，而 JSON 数组由 JSON 值组成。所谓对象成员，就是键值对，键必须为 JSON 字符串，然后值是任何 JSON 值，中间以冒号 `:`（`U+003A`）分隔。完整语法如下：

~~~
member = string ws %x3A ws value
object = %x7B ws [ member *( ws %x2C ws member ) ] ws %x7D
~~~

## 2. 数据结构

要表示键值对的集合，有很多数据结构可供选择，例如：

* 动态数组（dynamic array）：可扩展容量的数组，如 C++ 的 [`std::vector`](http://en.cppreference.com/w/cpp/container/vector)。
* 有序动态数组（sorted dynamic array）：和动态数组相同，但保证元素已排序，可用二分搜寻查询成员。
* 平衡树（balanced tree）：平衡二叉树可有序地遍历成员，如红黑树和 C++ 的 [`std::map`](http://en.cppreference.com/w/cpp/container/map)（[`std::multi_map`](http://en.cppreference.com/w/cpp/container/multimap) 支持重复键）。
* 哈希表（hash table）：通过哈希函数能实现平均 O(1) 查询，如 C++11 的 [`std::unordered_map`](http://en.cppreference.com/w/cpp/container/unordered_map)（[`unordered_multimap`](http://en.cppreference.com/w/cpp/container/unordered_multimap) 支持重复键）。

设一个对象有 n 个成员，数据结构的容量是 m，n ⩽ m，那么一些常用操作的时间／空间复杂度如下：

|               |动态数组 |有序动态数组|平衡树    |哈希表                |  
|---------------|:-------:|:----------:|:--------:|:--------------------:|  
|有序           |否       |是          |是        |否                    |
|自定成员次序   |可       |否          |否        |否                    |
|初始化 n 个成员|O(n)     |O(n log n)  |O(n log n)|平均 O(n)、最坏 O(n^2)|
|加入成员       |分摊 O(1)|O(n)        |O(log n)  |平均 O(1)、最坏 O(n)  |  
|移除成员       |O(n)     |O(n)        |O(log n)  |平均 O(1)、最坏 O(n)  |
|查询成员       |O(n)     |O(log n)    |O(log n)  |平均 O(1)、最坏 O(n)  |
|遍历成员       |O(n)     |O(n)        |O(n)      |O(m)                  |
|检测对象相等   |O(n^2)   |O(n)        |O(n)      |平均 O(n)、最坏 O(n^2)|
|空间           |O(m)     |O(m)        |O(n)      |O(m)                  |

在 ECMA-404 标准中，并没有规定对象中每个成员的键一定要唯一的，也没有规定是否需要维持成员的次序。

为了简单起见，我们的 leptjson 选择用动态数组的方案。我们将在单元八才加入动态功能，所以这单元中，每个对象仅仅是成员的数组。那么它跟上一单元的数组非常接近：

~~~c
typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

struct lept_value {
    union {
        struct { lept_member* m; size_t size; }o;
        struct { lept_value* e; size_t size; }a;
        struct { char* s; size_t len; }s;
        double n;
    }u;
    lept_type type;
};

struct lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
};
~~~

成员结构 `lept_member` 是一个 `lept_value` 加上键的字符串。如同 JSON 字符串的值，我们也需要同时保留字符串的长度，因为字符串本身可能包含空字符 `\u0000`。

在这单元中，我们仅添加了最基本的访问函数，用于撰写单元测试：

~~~c
size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);
~~~

在软件开发过程中，许多时候，选择合适的数据结构后已等于完成一半工作。没有完美的数据结构，所以最好考虑多一些应用的场合，看看时间／空间复杂度以至相关系数是否合适。

接下来，我们就可以着手实现。

## 3. 重构字符串解析

在软件工程中，[代码重构](https://zh.wikipedia.org/wiki/%E4%BB%A3%E7%A0%81%E9%87%8D%E6%9E%84)（code refactoring）是指在不改变软件外在行为时，修改代码以改进结构。代码重构十分依赖于单元测试，因为我们是通过单元测试去维护代码的正确性。有了足够的单元测试，我们可以放胆去重构，尝试并评估不同的改进方式，找到合乎心意而且能通过单元测试的改动，我们才提交它。

我们知道，成员的键也是一个 JSON 字符串，然而，我们不使用 `lept_value` 存储键，因为这样会浪费了当中 `type` 这个无用的字段。由于 `lept_parse_string()` 是直接地把解析的结果写进一个 `lept_value`，所以我们先用「提取方法（extract method，见下注）」的重构方式，把解析 JSON 字符串及写入 `lept_value` 分拆成两部分：

~~~c
/* 解析 JSON 字符串，把结果写入 str 和 len */
/* str 指向 c->stack 中的元素，需要在 c->stack  */
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
    /* \todo */
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}
~~~

这样的话，我们实现对象的解析时，就可以使用 `lept_parse_string_raw()`　来解析 JSON 字符串，然后把结果复制至 `lept_member` 的 `k` 和 `klen` 字段。

注：在 Fowler 的经典著作 [1] 中，把各种重构方式分门别类，每个方式都有详细的步骤说明。由于书中以 Java 为例子，所以方式的名称使用了 Java 的述语，例如方法（method）。在 C 语言中，「提取方法」其实应该称为「提取函数」。

[1] Fowler, Martin. Refactoring: improving the design of existing code. Pearson Education India, 2009. 中译本：熊节译，《重构——改善既有代码的设计》，人民邮电出版社，2010年。

## 4. 实现

解析对象与解析数组非常相似，所以我留空了几段作为练习。在解析数组时，我们把当前的元素以 `lept_value` 压入栈中，而在这里，我们则是以 `lept_member` 压入：

~~~c
static int lept_parse_object(lept_context* c, lept_value* v) {
    size_t size;
    lept_member m;
    int ret;
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = LEPT_OBJECT;
        v->u.o.m = 0;
        v->u.o.size = 0;
        return LEPT_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    for (;;) {
        lept_init(&m.v);
        /* \todo parse key to m.k, m.klen */
        /* \todo parse ws colon ws */
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */
        /* \todo parse ws [comma | right-curly-brace] ws */
    }
    /* \todo Pop and free members on the stack */
    return ret;
}
~~~

要注意的是，我们要为 `m.k` 分配内存去存储键的字符串，若在整个对象解析时发生错误，也要记得释放栈中的 `lept_member` 的 `k`。

我们为解析对象定义了几个新的错误码：

~~~c
enum {
    /* ... */
    LEPT_PARSE_MISS_KEY,
    LEPT_PARSE_MISS_COLON,
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};
~~~

在此不再赘述它们的意义了，可从以下的单元测试看到例子：

~~~c
static void test_parse_miss_key() {
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}
~~~

## 5. 总结与练习

在本单元中，除了谈及 JSON 对象的语法、可选的数据结构、实现方式，我们也轻轻谈及了重构的概念。有赖于测试驱动开发（TDD），我们可以不断重塑软件的内部结构。

完成这次练习之后，恭喜你，你已经完整地实现了一个符合标准的 JSON 解析器了。之后我们会完成更简单的生成器及其他访问功能。

由于对象和数组的相似性，此单元留空了较多实现部分作为练习：

1. 依第 3 节所述，重构 `lept_parse_string()`。重构前运行单元测试，重构后确保单元测试仍保持通过。
2. 打开 `test.c` 中两个 `#if 0`，运行单元测试，证实单元测试不通过。然后实现 `lept_parse_object()` 中的 `\todo` 部分。验证实现能通过单元测试。
3. 使用工具检测内存泄漏，解决它们。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
