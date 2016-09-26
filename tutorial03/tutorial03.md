# 从零开始的 JSON 库教程（三）：解析字符串

* Milo Yip
* 2016/9/22

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第三个单元。本单元的练习源代码位于 [json-tutorial/tutorial03](https://github.com/miloyip/json-tutorial/blob/master/tutorial03)。

本单元内容：

1. [JSON 字符串语法](#json-字符串语法)
2. [字符串表示](#字符串表示)
3. [内存管理](#内存管理)
4. [缓冲区与堆栈](#缓冲区与堆栈)
5. [解析字符串](#解析字符串)
6. [总结和练习](#总结和练习)
7. [参考](#参考)
8. [常见问题](#常见问题)

## 1. JSON 字符串语法

JSON 的字符串语法和 C 语言很相似，都是以双引号把字符括起来，如 `"Hello"`。但字符串采用了双引号作分隔，那么怎样可以在字符串中插入一个双引号？ 把 `a"b` 写成 `"a"b"` 肯定不行，都不知道那里是字符串的结束了。因此，我们需要引入转义字符（escape character），C 语言和 JSON 都使用 `\`（反斜线）作为转义字符，那么 `"` 在字符串中就表示为 `\"`，`a"b` 的 JSON 字符串则写成 `"a\"b"`。如以下的字符串语法所示，JSON 共支持 9 种转义序列：

~~~
string = quotation-mark *char quotation-mark
char = unescaped /
   escape (
       %x22 /          ; "    quotation mark  U+0022
       %x5C /          ; \    reverse solidus U+005C
       %x2F /          ; /    solidus         U+002F
       %x62 /          ; b    backspace       U+0008
       %x66 /          ; f    form feed       U+000C
       %x6E /          ; n    line feed       U+000A
       %x72 /          ; r    carriage return U+000D
       %x74 /          ; t    tab             U+0009
       %x75 4HEXDIG )  ; uXXXX                U+XXXX
escape = %x5C          ; \
quotation-mark = %x22  ; "
unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
~~~

简单翻译一下，JSON 字符串是由前后两个双引号夹着零至多个字符。字符分开无转义字符或转义序列。转义序列有 9 种，都是以反斜线开始，如常见的 `\n` 代表换行符。比较特殊的是 `\uXXXX`，当中 XXXX 为 16 进位的 UTF-16 编码，本单元将不处理这种转义序列，留待下回分解。

无转义字符就是普通的字符，语法中列出了合法的码点范围（码点还是在下单元才介绍）。要注意的是，该范围不包括 0 至 31、双引号和反斜线，这些码点都必须要使用转义方式表示。

## 2. 字符串表示

在 C 语言中，字符串一般表示为空结尾字符串（null-terminated string），即以空字符（`'\0'`）代表字符串的结束。然而，JSON 字符串是允许含有空字符的，例如这个 JSON `"Hello\u0000World"` 就是单个字符串，解析后为11个字符。如果纯粹使用空结尾字符来表示 JSON 解析后的结果，就没法处理空字符。

因此，我们可以分配内存来储存解析后的字符，以及记录字符的数目（即字符串长度）。由于大部分 C 程序都假设字符串是空结尾字符串，我们还是在最后加上一个空字符，那么不需处理 `\u0000` 这种字符的应用可以简单地把它当作是空结尾字符串。

了解需求后，我们考虑实现。`lept_value` 事实上是一种变体类型（variant type），我们通过 `type` 来决定它现时是哪种类型，而这也决定了哪些成员是有效的。首先我们简单地在这个结构中加入两个成员：

~~~c
typedef struct {
    char* s;
    size_t len;
    double n;
    lept_type type;
}lept_value;
~~~

然而我们知道，一个值不可能同时为数字和字符串，因此我们可使用 C 语言的 `union` 来节省内存：

~~~c
typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;
~~~

这两种设计在 32 位平台时的内存布局如下，可看出右方使用 `union` 的能省下内存。

![union_layout](images/union_layout.png)

我们要把之前的 `v->n` 改成 `v->u.n`。而要访问字符串的数据，则要使用 `v->u.s.s` 和 `v->u.s.len`。这种写法比较麻烦吧，其实 C11 新增了匿名 struct/union 语法，就可以采用 `v->n`、`v->s`、`v->len` 来作访问。

## 3. 内存管理

由于字符串的长度不是固定的，我们要动态分配内存。为简单起见，我们使用标准库 `<stdlib.h>` 中的 `malloc()`、`realloc()` 和 `free()` 来分配／释放内存。

当设置一个值为字符串时，我们需要把参数中的字符串复制一份：

~~~c
void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
~~~

断言中的条件是，非空指针（有具体的字符串）或是零长度的字符串都是合法的。

注意，在设置这个 `v` 之前，我们需要先调用 `lept_free(v)` 去清空 `v` 可能分配到的内存。例如原来已有一字符串，我们要先把它释放。然后就是简单地用 `malloc()` 分配及用 `memcpy()` 复制，并补上结尾空字符。`malloc(len + 1)` 中的 1 是因为结尾空字符。

那么，再看看 `lept_free()`：

~~~c
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}
~~~

现时仅当值是字符串类型，我们才要处理，之后我们还要加上对数组及对象的释放。`lept_free(v)` 之后，会把它的类型变成 null。这个设计能避免重复释放。

但也由于我们会检查 `v` 的类型，在调用所有访问函数之前，我们必须初始化该类型。所以我们加入 `lept_init(v)`，因非常简单我们用宏实现：

~~~c
#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
~~~

用上 `do { ... } while(0)` 是为了把表达式转为语句，模仿无返回值的函数。

其实在前两个单元中，我们只提供读取值的 API，没有写入的 API，就是因为写入时我们还要考虑释放内存。我们在本单元中把它们补全：

~~~c
#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);
~~~

由于 `lept_free()` 实际上也会把 `v` 变成 null 值，我们只用一个宏来提供 `lept_set_null()` 这个 API。

应用方的代码在调用 `lept_parse()` 之后，最终也应该调用 `lept_free()` 去释放内存。我们把之前的单元测试也加入此调用。

如果不使用 `lept_parse()`，我们需要初始化值，那么就像以下的单元测试，先 `lept_init()`，最后 `lept_free()`。

~~~c
static void test_access_string() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}
~~~

## 4. 缓冲区与堆栈

我们解析字符串（以及之后的数组、对象）时，需要把解析的结果先储存在一个临时的缓冲区，最后再用 `lept_set_string()` 把缓冲区的结果设进值之中。在完成解析一个字符串之前，这个缓冲区的大小是不能预知的。因此，我们可以采用动态数组（dynamic array）这种数据结构，即数组空间不足时，能自动扩展。C++ 标准库的 `std::vector` 也是一种动态数组。

如果每次解析字符串时，都重新建一个动态数组，那么是比较耗时的。我们可以重用这个动态数组，每次解析 JSON 时就只需要创建一个。而且我们将会发现，无论是解析字符串、数组或对象，我们也只需要以先进后出的方式访问这个动态数组。换句话说，我们需要一个动态的堆栈数据结构。

我们把一个动态堆栈的数据放进 `lept_context` 里：

~~~c
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;
~~~

当中 `size` 是当前的堆栈容量，`top` 是栈顶的位置（由于我们会扩展 `stack`，所以不要把 `top` 用指针形式存储）。

然后，我们在创建 `lept_context` 的时候初始化 `stack` 并最终释放内存：

~~~c
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;        /* <- */
    c.size = c.top = 0;    /* <- */
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        /* ... */
    }
    assert(c.top == 0);    /* <- */
    free(c.stack);         /* <- */
    return ret;
}
~~~

在释放时，加入了断言确保所有数据都被弹出。

然后，我们实现堆栈的压入及弹出操作。和普通的堆栈不一样，我们这个堆栈是以字节储存的。每次可要求压入任意大小的数据，它会返回数据起始的指针（会 C++ 的同学可再参考[1]）：

~~~
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}
~~~

压入时若空间不足，便回以 1.5 倍大小扩展。为什么是 1.5 倍而不是两倍？可参考我在 [STL 的 vector 有哪些封装上的技巧？](https://www.zhihu.com/question/25079705/answer/30030883) 的答案。

注意到这里使用了 [`realloc()`](http://en.cppreference.com/w/c/memory/realloc) 来重新分配内存，`c->stack` 在初始化时为 `NULL`，`realloc(NULL, size)` 的行为是等价于 `malloc(size)` 的，所以我们不需要为第一次分配内存作特别处理。

另外，我们把初始大小以宏 `LEPT_PARSE_STACK_INIT_SIZE` 的形式定义，使用 `#ifndef X #define X ... #endif` 方式的好处是，使用者可在编译选项中自行设置宏，没设置的话就用缺省值。

## 5. 解析字符串

有了以上的工具，解析字符串的任务就变得很简单。我们只需要先备份栈顶，然后把解析到的字符压栈，最后计算出长度并一次性把所有字符弹出，再设置至值里便可以。以下是部分实现，没有处理转义和一些不合法字符的校验。

~~~c
#define PUTC(c, ch) do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                lept_set_string(v, lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                c->top = head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            default:
                PUTC(c, ch);
        }
    }
}
~~~

## 6. 总结和练习

之前的单元都是固定长度的数据类型（fixed length data type），而字符串类型是可变长度的数据类型（variable length data type），因此本单元花了较多篇幅讲述内存管理和数据结构的设计和实现。字符串的解析相对数字简单，以下的习题难度不高，同学们应该可轻松完成。

1. 编写 `lept_get_boolean()` 等访问函数的单元测试，然后实现。
2. 实现除了 `\u` 以外的转义序列解析，令 `test_parse_string()` 中所有测试通过。
3. 解决 `test_parse_invalid_string_escape()` 和 `test_parse_invalid_string_char()` 中的失败测试。
4. 思考如何优化 `test_parse_string()` 的性能，那些优化方法有没有缺点。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。

## 7. 参考

[1] [RapidJSON 代码剖析（一）：混合任意类型的堆栈](https://zhuanlan.zhihu.com/p/20029820)

# 8. 常见问题

其他常见问答将会从评论中整理。
