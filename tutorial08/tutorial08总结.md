### tutorial08总结

终于到了最后的篇章qwq

这一章要实现的就是动态数组，动态对象，以及`lept_is_euqal`等函数，与之前的篇章基本一致。

这一章多了一些东西，值语义，引用语义等，需要好好理解，可以配合Milo的一篇回答再
更多了解一下
[如何理解 C++ 中的深拷贝和浅拷贝？ - Milo Yip的回答 - 知乎](
https://www.zhihu.com/question/36370072/answer/68086634)

本单元练习内容：

1. 完成 lept_is_equal() 里的对象比较部分。不需要考虑对象内有重复键的情况。
2. 打开 test_access_array() 里的 #if 0，实现 lept_insert_array_element()、lept_erase_array_element() 和 lept_clear_array()。
3. 打开 test_access_object() 里的 #if 0，参考动态数组，实现第 5 部分列出的所有函数。
4. 完成 lept_copy() 里的数组和对象的复制部分。

### lept_copy(lept_value* dst, const lept_value* src)
对于`object`类型的每个对象，需要比较`klen`字段以及`k`字段，于是使用 `lept_find_object_index`来查找是否存在；
接下来的`v`字段可以采用`lept_value`递归解析。
```c++
        case LEPT_OBJECT:
            /* \todo */
            /* size compare */
            if(lhs->u.o.size != rhs->u.o.size)
                return 0;
            /* key-value comp */
            for(i = 0; i < lhs->u.o.size; i++){
                index = lept_find_object_index(rhs, lhs->u.o.m[i].k, lhs->u.o.m[i].klen);
                if(index == LEPT_KEY_NOT_EXIST) return 0; //key not exist
                if(!lept_is_equal(&lhs->u.o.m[i].v, &rhs->u.o.m[index].v)) return 0; //value not match
            }
            return 1;

```

### lept_insert_array_element
需要先考虑是否需要扩容，扩容系数为2。将`[index, end)`的内容右移，然后将`index`初始化，最后调整长度，返回`[index]`指针
```c++
lept_value* lept_insert_array_element(lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY && index <= v->u.a.size);
    /* \todo */
    if(v->u.a.size == v->u.a.capacity) lept_reserve_array(v, v->u.a.capacity == 0? 1: (v->u.a.size << 1)); //扩容为原来一倍
    memcpy(&v->u.a.e[index + 1], &v->u.a.e[index], (v->u.a.size - index) * sizeof(lept_value));
    lept_init(&v->u.a.e[index]);
    v->u.a.size++;
    return &v->u.a.e[index];
}
```

### lept_erase_array_element
回收完空间，然后将index 后面count个元素移到index处，最后将空闲的count个元素重新初始化
```c++
void lept_erase_array_element(lept_value* v, size_t index, size_t count) {
    assert(v != NULL && v->type == LEPT_ARRAY && index + count <= v->u.a.size);
    /* \todo */
    size_t i;
    for(i = index; i < index + count; i++){
        lept_free(&v->u.a.e[i]);
    }
    memcpy(v->u.a.e + index, v->u.a.e + index + count, (v->u.a.size - index - count) * sizeof(lept_value));
    for(i = v->u.a.size - count; i < v->u.a.size; i++)
        lept_init(&v->u.a.e[i]);
    v->u.a.size -= count;
}
```

### lept_object部分函数

#### lept_reserve_object
与`lept_reserve_array`差不多
```c++
void lept_reserve_object(lept_value* v, size_t capacity) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    /* \todo */
    if(v->u.o.capacity < capacity){
        v->u.o.capacity = capacity;
        v->u.o.m = (lept_member *) realloc(v->u.o.m, capacity * sizeof(lept_member));
    }
}
```

#### lept_shrink_object
与`lept_shrink_object`差不多
```c++
void lept_shrink_object(lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    /* \todo */
    if(v->u.o.capacity > v->u.o.size) {
        v->u.o.capacity = v->u.o.size;
        v->u.o.m = (lept_member *)realloc(v->u.o.m, v->u.o.capacity * sizeof(lept_value));
    }
}
```

#### lept_clear_object
`object`类型包含`k`, `k`是普通字符串可以`free`释放，要避免空悬指针问题；`v`使用 `lept_free`释放
```c++
void lept_clear_object(lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    /* \todo */
    size_t i;
    for(i = 0; i < v->u.o.size; i++){
        //回收k和v空间
        free(v->u.o.m[i].k);
        v->u.o.m[i].k = NULL;
        v->u.o.m[i].klen = 0;
        lept_free(&v->u.o.m[i].v);
    }
    v->u.o.size = 0;
}
```

#### lept_set_object_value
设置`k`字段为`key`的对象的值，如果在查找过程中找到了已经存在`key`，则返回；否则新申请一块空间并初始化，然后返回
```c++
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen) {
    assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
    /* \todo */
    size_t i, index;
    index = lept_find_object_index(v, key, klen);
    if(index != LEPT_KEY_NOT_EXIST)
        return &v->u.o.m[index].v;
    //key not exist, then we make room and init
    if(v->u.o.size == v->u.o.capacity){
        lept_reserve_object(v, v->u.o.capacity == 0? 1: (v->u.o.capacity << 1));
    }
    i = v->u.o.size;
    v->u.o.m[i].k = (char *)malloc((klen + 1));
    memcpy(v->u.o.m[i].k, key, klen);
    v->u.o.m[i].k[klen] = '\0';
    v->u.o.m[i].klen = klen;
    lept_init(&v->u.o.m[i].v);
    v->u.o.size++;
    return &v->u.o.m[i].v;
}
```

#### lept_remove_object_value
类似于链表去除节点，该节点后面的节点接上
```c++
void lept_remove_object_value(lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT && index < v->u.o.size);
    /* \todo */
    free(v->u.o.m[index].k);
    lept_free(&v->u.o.m[index].v);
    //think like a list
    memcpy(v->u.o.m + index, v->u.o.m + index + 1, (v->u.o.size - index - 1) * sizeof(lept_member));
    v->u.o.m[--v->u.o.size].k = NULL;
    v->u.o.m[v->u.o.size].klen = 0;
    lept_init(&v->u.o.m[v->u.o.size].v);
}
```

#### lept_copy
```c++
void lept_copy(lept_value* dst, const lept_value* src) {
    assert(src != NULL && dst != NULL && src != dst);
    size_t i;
    switch (src->type) {
        case LEPT_STRING:
            lept_set_string(dst, src->u.s.s, src->u.s.len);
            break;
        case LEPT_ARRAY:
            /* \todo */
            //数组
            lept_set_array(dst, src->u.a.size);
            for(i = 0; i < src->u.a.size; i++){
                lept_copy(&dst->u.a.e[i], &src->u.a.e[i]);
            }
            dst->u.a.size = src->u.a.size;
            break;
        case LEPT_OBJECT:
            /* \todo */
            //对象
            lept_set_object(dst, src->u.o.size);

            for(i = 0; i < src->u.o.size; i++){
                //k
                lept_value * val = lept_set_object_value(dst, src->u.o.m[i].k, src->u.o.m[i].klen);
                //v
                lept_copy(val, &src->u.o.m[i].v);
            }
            dst->u.o.size = src->u.o.size;
            break;
        default:
            lept_free(dst);
            memcpy(dst, src, sizeof(lept_value));
            break;
    }
}
```