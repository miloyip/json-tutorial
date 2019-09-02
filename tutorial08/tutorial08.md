# 从零开始的 JSON 库教程（八）：访问与其他功能

* Milo Yip
* 2018/6/2

本文是[《从零开始的 JSON 库教程》](https://zhuanlan.zhihu.com/json-tutorial)的第八个单元。代码位于 [json-tutorial/tutorial08](https://github.com/miloyip/json-tutorial/blob/master/tutorial08)。

本单元内容：

1. [对象键值查询](#1-对象键值查询)
2. [相等比较](#2-相等比较)
3. [复制、移动与交换](#3-复制移动与交换)
4. [动态数组](#4-动态数组)
5. [动态对象](#5-动态对象)
6. [总结与练习](#6-总结与练习)

## 1. 对象键值查询

我们在第六个单元实现了 JSON 对象的数据结构，它仅为一个 `lept_value` 的数组：

~~~c
struct lept_value {
    union {
        struct { lept_member* m; size_t size; }o;
        /* ... */
    }u;
    lept_type type;
};

struct lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
};
~~~

为了做相应的解析测试，我们实现了最基本的查询功能：

~~~c
size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(lept_value* v, size_t index);
~~~

在实际使用时，我们许多时候需要查询一个键值是否存在，如存在，要获得其相应的值。我们可以提供一个函数，简单地用线性搜寻实现这个查询功能（时间复杂度 $\mathrm{O}(n)$）：

~~~c
#define LEPT_KEY_NOT_EXIST ((size_t)-1)

size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen) {
    size_t i;
    assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
    for (i = 0; i < v->u.o.size; i++)
        if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
            return i;
    return LEPT_KEY_NOT_EXIST;
}}
~~~

若对象内没有所需的键，此函数返回 `LEPT_KEY_NOT_EXIST`。使用时：

~~~c
lept_value o;
size_t index;
lept_init(&o);
lept_parse(&o, "{\"name\":\"Milo\", \"gender\":\"M\"}");
index = lept_find_object_index(&o, "name", 4);
if (index != LEPT_KEY_NOT_EXIST) {
    lept_value* v = lept_get_object_value(&o, index);
    printf("%s\n", lept_get_string(v));
}
lept_free(&o);
~~~

由于一般也是希望获取键对应的值，而不需要索引，我们再加入一个辅助函数，返回类型改为 `lept_value*`：

~~~c
lept_value* lept_find_object_value(lept_value* v, const char* key, size_t klen) {
    size_t index = lept_find_object_index(v, key, klen);
    return index != LEPT_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}
~~~

上述例子便可简化为：

~~~c
lept_value o, *v;
/* ... */
if ((v = lept_find_object_value(&o, "name", 4)) != NULL)
    printf("%s\n", lept_get_string(v));
~~~

## 2. 相等比较

在实现数组和对象的修改之前，为了测试结果的正确性，我们先实现 `lept_value` 的[相等比较](https://zh.wikipedia.org/zh-cn/%E9%97%9C%E4%BF%82%E9%81%8B%E7%AE%97%E5%AD%90)（equality comparison）。首先，两个值的类型必须相同，对于 true、false、null 这三种类型，比较类型后便完成比较。而对于数字和字符串，需进一步检查是否相等：

~~~c
int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if (lhs->type != rhs->type)
        return 0;
    switch (lhs->type) {
        case LEPT_STRING:
            return lhs->u.s.len == rhs->u.s.len && 
                memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
        case LEPT_NUMBER:
            return lhs->u.n == rhs->u.n;
        /* ... */
        default:
            return 1;
    }
}
~~~

由于值可能复合类型（数组和对象），也就是一个树形结构。当我们要比较两个树是否相等，可通过递归实现。例如，对于数组，我们先比较元素数目是否相等，然后递归检查对应的元素是否相等：

~~~c
int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
    size_t i;
    /* ... */
    switch (lhs->type) {
        /* ... */
        case LEPT_ARRAY:
            if (lhs->u.a.size != rhs->u.a.size)
                return 0;
            for (i = 0; i < lhs->u.a.size; i++)
                if (!lept_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
                    return 0;
            return 1;
        /* ... */
    }
}
~~~

而对象与数组的不同之处，在于概念上对象的键值对是无序的。例如，`{"a":1,"b":2}` 和 `{"b":2,"a":1}` 虽然键值的次序不同，但这两个 JSON 对象是相等的。我们可以简单地利用 `lept_find_object_index()` 去找出对应的值，然后递归作比较。这部分留给读者作为练习。

## 3. 复制、移动与交换

本单元的重点，在于修改数组和对象的内容。我们将会实现一些接口做修改的操作，例如，为对象设置一个键值，我们可能会这么设计：

~~~c
void lept_set_object_value(lept_value* v, const char* key, size_t klen, const lept_value* value);

void f() {
    lept_value v, s;
    lept_init(&v);
    lept_parse(&v, "{}");
    lept_init(&s);
    lept_set_string(&s, "Hello", 5);
    lept_set_object_keyvalue(&v, "s", &s); /* {"s":"Hello"} */
    lept_free(&v)
    lept_free(&s);  /* 第二次释放！*/
}
~~~

凡涉及赋值，都可能会引起资源拥有权（resource ownership）的问题。值 `s` 并不能以指针方式简单地写入对象 `v`，因为这样便会有两个地方都拥有 `s`，会做成重复释放的 bug。我们有两个选择：

1. 在 `lept_set_object_value()` 中，把参数 `value` [深度复制](https://en.wikipedia.org/wiki/Object_copying#Deep_copy)（deep copy）一个值，即把整个树复制一份，写入其新增的键值对中。
2. 在 `lept_set_object_value()` 中，把参数 `value` 拥有权转移至新增的键值对，再把 `value` 设置成 null 值。这就是所谓的移动语意（move semantics）。

深度复制是一个常用功能，使用者也可能会用到，例如把一个 JSON 复制一个版本出来修改，保持原来的不变。所以，我们实现一个公开的深度复制函数：

~~~c
void lept_copy(lept_value* dst, const lept_value* src) {
    size_t i;
    assert(src != NULL && dst != NULL && src != dst);
    switch (src->type) {
        case LEPT_STRING:
            lept_set_string(dst, src->u.s.s, src->u.s.len);
            break;
        case LEPT_ARRAY:
            /* \todo */
            break;
        case LEPT_OBJECT:
            /* \todo */
            break;
        default:
            lept_free(dst);
            memcpy(dst, src, sizeof(lept_value));
            break;
    }
}
~~~

C++11 加入了右值引用的功能，可以从语言层面区分复制和移动语意。而在 C 语言中，我们也可以通过实现不同版本的接口（不同名字的函数），实现这两种语意。但为了令接口更简单和正交（orthgonal），我们修改了 `lept_set_object_value()` 的设计，让它返回新增键值对的值指针，所以我们可以用 `lept_copy()` 去复制赋值，也可以简单地改变新增的键值：

~~~c
/* 返回新增键值对的指针 */
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen);

void f() {
    lept_value v;
    lept_init(&v);
    lept_parse(&v, "{}");
    lept_set_string(lept_set_object_value(&v, "s"), "Hello", 5);
    /* {"s":"Hello"} */
    lept_copy(
        lept_add_object_keyvalue(&v, "t"),
        lept_get_object_keyvalue(&v, "s", 1));
    /* {"s":"Hello","t":"Hello"} */
    lept_free(&v);
}
~~~

我们还提供了 `lept_move()`，它的实现也非常简单：

~~~c
void lept_move(lept_value* dst, lept_value* src) {
    assert(dst != NULL && src != NULL && src != dst);
    lept_free(dst);
    memcpy(dst, src, sizeof(lept_value));
    lept_init(src);
}
~~~

类似地，我们也实现了一个交换值的接口：

~~~c
void lept_swap(lept_value* lhs, lept_value* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if (lhs != rhs) {
        lept_value temp;
        memcpy(&temp, lhs, sizeof(lept_value));
        memcpy(lhs,   rhs, sizeof(lept_value));
        memcpy(rhs, &temp, sizeof(lept_value));
    }
}
~~~

当我们要修改对象或数组里的值时，我们可以利用这 3 个函数。例如：

~~~c
const char* json = "{\"a\":[1,2],\"b\":3}";
char *out;
lept_value v;
lept_init(&v);
lept_parse(&v, json);
lept_copy(
    lept_find_object_value(&v, "b", 1),
    lept_find_object_value(&v, "a", 1));
printf("%s\n", out = lept_stringify(&v, NULL)); /* {"a":[1,2],"b":[1,2]} */
free(out);

lept_parse(&v, json);
lept_move(
    lept_find_object_value(&v, "b", 1),
    lept_find_object_value(&v, "a", 1));
printf("%s\n", out = lept_stringify(&v, NULL)); /* {"a":null,"b":[1,2]} */
free(out);

lept_parse(&v, json);
lept_swap(
    lept_find_object_value(&v, "b", 1),
    lept_find_object_value(&v, "a", 1));
printf("%s\n", out = lept_stringify(&v, NULL)); /* {"a":3,"b":[1,2]} */
free(out);

lept_free(&v);
~~~

在使用时，可尽量避免 `lept_copy()`，而改用 `lept_move()` 或 `lept_swap()`，因为后者不需要分配内存。当中 `lept_swap()` 更是无须做释放的工作，令它达到 $\mathrm{O}(1)$ 时间复杂度，其性能与值的内容无关。

## 4. 动态数组

在此单元之前的实现里，每个数组的元素数目在解析后是固定不变的，其数据结构是：

~~~c
struct lept_value {
    union {
        /* ... */
        struct { lept_value* e; size_t size; }a; /* array:  elements, element count*/
        /* ... */
    }u;
    lept_type type;
};
~~~

用这种数据结构增删元素时，我们需要重新分配一个数组，把适当的旧数据拷贝过去。但这种做法是非常低效的。例如我们想要从一个空的数组加入 $n$ 个元素，便要做 $n(n - 1)/2$ 次元素复制，即 $\mathrm{O}(n^2)$ 的时间复杂度。

其中一个改进方法，是使用动态数组（dynamic array，或称可增长数组／growable array）的数据结构。C++ STL 标准库中最常用的 `std::vector` 也是使用这种数据结构的容器。

改动也很简单，只需要在数组中加入容量 `capacity` 字段，表示当前已分配的元素数目，而 `size` 则表示现时的有效元素数目：

~~~c
        /* ... */
        struct { lept_value* e; size_t size, capacity; }a; /* array:  elements, element count, capacity */
        /* ... */
~~~

我们终于提供设置数组的函数，而且它可提供初始的容量：

~~~c
void lept_set_array(lept_value* v, size_t capacity) {
    assert(v != NULL);
    lept_free(v);
    v->type = LEPT_ARRAY;
    v->u.a.size = 0;
    v->u.a.capacity = capacity;
    v->u.a.e = capacity > 0 ? (lept_value*)malloc(capacity * sizeof(lept_value)) : NULL;
}
~~~

我们需要稍修改 `lept_parse_array()`，调用 `lept_set_array()` 去设置类型和分配空间。

另外，类似于 `lept_get_array_size()`，也加入获取当前容量的函数：

~~~c
size_t lept_get_array_capacity(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.capacity;
}
~~~

如果当前的容量不足，我们需要扩大容量，标准库的 `realloc()` 可以分配新的内存并把旧的数据拷背过去：

~~~c
void lept_reserve_array(lept_value* v, size_t capacity) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.capacity < capacity) {
        v->u.a.capacity = capacity;
        v->u.a.e = (lept_value*)realloc(v->u.a.e, capacity * sizeof(lept_value));
    }
}
~~~

当数组不需要再修改，可以使用以下的函数，把容量缩小至刚好能放置现有元素：

~~~c
void lept_shrink_array(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.capacity > v->u.a.size) {
        v->u.a.capacity = v->u.a.size;
        v->u.a.e = (lept_value*)realloc(v->u.a.e, v->u.a.capacity * sizeof(lept_value));
    }
}
~~~

我们不逐一检视每个数组修改函数，仅介绍一下两个例子：

~~~c
lept_value* lept_pushback_array_element(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.size == v->u.a.capacity)
        lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);
    lept_init(&v->u.a.e[v->u.a.size]);
    return &v->u.a.e[v->u.a.size++];
}

void lept_popback_array_element(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY && v->u.a.size > 0);
    lept_free(&v->u.a.e[--v->u.a.size]);
}
~~~

`lept_pushback_array_element()` 在数组末端压入一个元素，返回新的元素指针。如果现有的容量不足，就需要调用 `lept_reserve_array()` 扩容。我们现在用了一个最简单的扩容公式：若容量为 0，则分配 1 个元素；其他情况倍增容量。

`lept_popback_array_element()` 则做相反的工作，记得删去的元素需要调用 `lept_free()`。

下面这 3 个函数留给读者练习：

1. `lept_insert_array_element()` 在 `index` 位置插入一个元素；
2. `lept_erase_array_element()` 删去在 `index` 位置开始共 `count` 个元素（不改容量）；
3. `lept_clear_array()` 清除所有元素（不改容量）。

~~~c
lept_value* lept_insert_array_element(lept_value* v, size_t index);
void lept_erase_array_element(lept_value* v, size_t index, size_t count);
void lept_clear_array(lept_value* v);
~~~

## 5. 动态对象

动态对象也是采用上述相同的结构，所以直接留给读者修改结构体，并实现以下函数：

~~~c
void lept_set_object(lept_value* v, size_t capacity);
size_t lept_get_object_capacity(const lept_value* v);
void lept_reserve_object(lept_value* v, size_t capacity);
void lept_shrink_object(lept_value* v);
void lept_clear_object(lept_value* v);
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen);
void lept_remove_object_value(lept_value* v, size_t index);
~~~

注意 `lept_set_object_value()` 会先搜寻是否存在现有的键，若存在则直接返回该值的指针，不存在时才新增。

## 6. 总结与练习

本单元主要加入了数组和对象的访问、修改方法。当中的赋值又引申了三种赋值的方式（复制、移动、交换）。这些问题是各种编程语言中都需要考虑的事情，为了减少深度复制的成本，有些程序库或运行时还会采用[写入时复制](https://zh.wikipedia.org/zh-cn/%E5%AF%AB%E5%85%A5%E6%99%82%E8%A4%87%E8%A3%BD)（copy-on-write, COW）。而浅复制（shallow copy）则需要 [引用计数](https://zh.wikipedia.org/wiki/%E5%BC%95%E7%94%A8%E8%AE%A1%E6%95%B0)（reference count）或 [垃圾回收](https://zh.wikipedia.org/zh-cn/%E5%9E%83%E5%9C%BE%E5%9B%9E%E6%94%B6_(%E8%A8%88%E7%AE%97%E6%A9%9F%E7%A7%91%E5%AD%B8))（garbage collection, GC）等技术。

另外，我们实现了以动态数组的数据结构，能较高效地对数组和对象进行增删操作。至此，我们已经完成本教程的所有核心功能。做完下面的练习后，我们还会作简单讲解，然后将迎来本教程的最后一个单元。

本单元练习内容：

1. 完成 `lept_is_equal()` 里的对象比较部分。不需要考虑对象内有重复键的情况。
2. 打开 `test_access_array()` 里的 `#if 0`，实现 `lept_insert_array_element()`、`lept_erase_array_element()` 和 `lept_clear_array()`。
3. 打开 `test_access_object()` 里的 `#if 0`，参考动态数组，实现第 5 部分列出的所有函数。
4. 完成 `lept_copy()` 里的数组和对象的复制部分。

如果你遇到问题，有不理解的地方，或是有建议，都欢迎在评论或 [issue](https://github.com/miloyip/json-tutorial/issues) 中提出，让所有人一起讨论。
