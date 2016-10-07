# 从零开始的 JSON 库教程（三）：解析字符串解答篇

* Milo Yip
* 2016/9/27

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第三个单元解答编。解答代码位于 [json-tutorial/tutorial03_answer](https://github.com/miloyip/json-tutorial/blob/master/tutorial03_answer)。

## 1. 访问的单元测试

在编写单元测试时，我们故意先把值设为字符串，那么做可以测试设置其他类型时，有没有调用 `lept_free()` 去释放内存。

~~~c
static void test_access_boolean() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_boolean(&v, 1);
    EXPECT_TRUE(lept_get_boolean(&v));
    lept_set_boolean(&v, 0);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_number() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
    lept_free(&v);
}
~~~

以下是访问函数的实现：

~~~c
int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}
~~~

那问题是，如果我们没有调用 `lept_free()`，怎样能发现这些内存泄漏？

## 1A. Windows 下的内存泄漏检测方法

在 Windows 下，可使用 Visual C++ 的 [C Runtime Library（CRT） 检测内存泄漏](https://msdn.microsoft.com/zh-cn/library/x98tx3cf.aspx)。

首先，我们在两个 .c 文件首行插入这一段代码：

~~~c
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
~~~

并在 `main()` 开始位置插入：

~~~c
int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
~~~

在 Debug 配置下按 F5 生成、开始调试程序，没有任何异样。

然后，我们删去 `lept_set_boolean()` 中的 `lept_free(v)`：

~~~c
void lept_set_boolean(lept_value* v, int b) {
    /* lept_free(v); */
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
~~~

再次按 F5 生成、开始调试程序，在输出会看到内存泄漏信息：

~~~
Detected memory leaks!
Dumping objects ->
C:\GitHub\json-tutorial\tutorial03_answer\leptjson.c(212) : {79} normal block at 0x013D9868, 2 bytes long.
 Data: <a > 61 00 
Object dump complete.
~~~

这正是我们在单元测试中，先设置字符串，然后设布尔值时没释放字符串所分配的内存。比较麻烦的是，它没有显示调用堆栈。从输出信息中 `... {79} ...` 我们知道是第 79 次分配的内存做成问题，我们可以加上 `_CrtSetBreakAlloc(79);` 来调试，那么它便会在第 79 次时中断于分配调用的位置，那时候就能从调用堆栈去找出来龙去脉。

## 1B. Linux/OSX 下的内存泄漏检测方法

在 Linux、OS X 下，我们可以使用 [valgrind](http://valgrind.org/) 工具（用 `apt-get install valgrind`、 `brew install valgrind`）。我们完全不用修改代码，只要在命令行执行：

~~~
$ valgrind --leak-check=full  ./leptjson_test
==22078== Memcheck, a memory error detector
==22078== Copyright (C) 2002-2015, and GNU GPL'd, by Julian Seward et al.
==22078== Using Valgrind-3.11.0 and LibVEX; rerun with -h for copyright info
==22078== Command: ./leptjson_test
==22078== 
--22078-- run: /usr/bin/dsymutil "./leptjson_test"
160/160 (100.00%) passed
==22078== 
==22078== HEAP SUMMARY:
==22078==     in use at exit: 27,728 bytes in 209 blocks
==22078==   total heap usage: 301 allocs, 92 frees, 34,966 bytes allocated
==22078== 
==22078== 2 bytes in 1 blocks are definitely lost in loss record 1 of 79
==22078==    at 0x100012EBB: malloc (in /usr/local/Cellar/valgrind/3.11.0/lib/valgrind/vgpreload_memcheck-amd64-darwin.so)
==22078==    by 0x100008F36: lept_set_string (leptjson.c:208)
==22078==    by 0x100008415: test_access_boolean (test.c:187)
==22078==    by 0x100001849: test_parse (test.c:229)
==22078==    by 0x1000017A3: main (test.c:235)
==22078== 
...
~~~

它发现了在 `test_access_boolean()` 中，由 `lept_set_string()` 分配的 2 个字节（`"a"`）泄漏了。

Valgrind 还有很多功能，例如可以发现未初始化变量。我们若在应用程序或测试程序中，忘了调用 `lept_init(&v)`，那么 `v.type` 的值没被初始化，其值是不确定的（indeterministic），一些函数如果读取那个值就会出现问题：

~~~c
static void test_access_boolean() {
    lept_value v;
    /* lept_init(&v); */
    lept_set_string(&v, "a", 1);
    ...
}
~~~

这种错误有时候测试时能正确运行（刚好 `v.type` 被设为 `0`），使我们误以为程序正确，而在发布后一些机器上却可能崩溃。这种误以为正确的假像是很危险的，我们可利用 valgrind 能自动测出来：

~~~
$ valgrind --leak-check=full  ./leptjson_test
...
==22174== Conditional jump or move depends on uninitialised value(s)
==22174==    at 0x100008B5D: lept_free (leptjson.c:164)
==22174==    by 0x100008F26: lept_set_string (leptjson.c:207)
==22174==    by 0x1000083FE: test_access_boolean (test.c:187)
==22174==    by 0x100001839: test_parse (test.c:229)
==22174==    by 0x100001793: main (test.c:235)
==22174== 
~~~

它发现 `lept_free()` 中依靠了一个未初始化的值来跳转，就是 `v.type`，而错误是沿自 `test_access_boolean()`。

编写单元测试时，应考虑哪些执行次序会有机会出错，例如内存相关的错误。然后我们可以利用 TDD 的步骤，先令测试失败（以内存工具检测），修正代码，再确认测试是否成功。

## 2. 转义序列的解析

转义序列的解析很直观，对其他不合法的字符返回 `LEPT_PARSE_INVALID_STRING_ESCAPE`：

~~~c
static int lept_parse_string(lept_context* c, lept_value* v) {
    /* ... */
    for (;;) {
        char ch = *p++;
        switch (ch) {
            /* ... */
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    default:
                        c->top = head;
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
           /* ... */
       }
   }
 }
~~~

## 3. 不合法的字符串

上面已解决不合法转义，余下部分的唯一难度，是要从语法中知道哪些是不合法字符：

~~~
unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
~~~

当中空缺的 %x22 是双引号，%x5C 是反斜线，都已经处理。所以不合法的字符是 %x00 至 %x1F。我们简单地在 default 里处理：

~~~c
        /* ... */
            default:
                if ((unsigned char)ch < 0x20) { 
                    c->top = head;
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch);
        /* ... */
~~~

注意到 `char` 带不带符号，是实现定义的。如果编译器定义 `char` 为带符号的话，`(unsigned char)ch >= 0x80` 的字符，都会变成负数，并产生 `LEPT_PARSE_INVALID_STRING_CHAR` 错误。我们现时还没有测试 ASCII 以外的字符，所以有没有转型至不带符号都不影响，但下一单元开始处理 Unicode 的时候就要考虑了。

## 4. 性能优化的思考

这是本教程第一次的开放式问题，没有标准答案。以下列出一些我想到的。

1. 如果整个字符串都没有转义符，我们不就是把字符复制了两次？第一次是从 `json` 到 `stack`，第二次是从 `stack` 到 `v->u.s.s`。我们可以在 `json` 扫描 `'\0'`、`'\"'` 和 `'\\'` 3 个字符（ `ch < 0x20` 还是要检查），直至它们其中一个出现，才开始用现在的解析方法。这样做的话，前半没转义的部分可以只复制一次。缺点是，代码变得复杂一些，我们也不能使用 `lept_set_string()`。
2. 对于扫描没转义部分，我们可考虑用 SIMD 加速，如 [RapidJSON 代码剖析（二）：使用 SSE4.2 优化字符串扫描](https://zhuanlan.zhihu.com/p/20037058) 的做法。这类底层优化的缺点是不跨平台，需要设置编译选项等。
3. 在 gcc/clang 上使用 `__builtin_expect()` 指令来处理低概率事件，例如需要对每个字符做 `LEPT_PARSE_INVALID_STRING_CHAR` 检测，我们可以假设出现不合法字符是低概率事件，然后用这个指令告之编译器，那么编译器可能可生成较快的代码。然而，这类做法明显是不跨编译器，甚至是某个版本后的 gcc 才支持。

## 5. 总结

本解答篇除了给出一些建议方案，也介绍了内存泄漏的检测方法。JSON 字符串本身的语法并不复杂，但它需要相关的内存分配与数据结构的设计，还好这些设计都能用于之后的数组和对象类型。下一单元专门针对 Unicode，这部分也是许多 JSON 库没有妥善处理的地方。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
