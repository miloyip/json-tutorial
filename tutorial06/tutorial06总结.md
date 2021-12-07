### 解析JSON对象
json对象的完整语法：
```c++
member = string ws : ws value
object = { ws [ member *(ws %x2C ws member) ] ws }
```

json对象示例：
```c++
{age: 10, name: xxx}
```
于是我们可以使用以下结构存储JSON对象的key和value，使用 `char *`作为key是为了节约空间
```c++
lept_member{
    char * k; size_t len;
    lept_value  v;
};
```
重构`lept_parse_string`为`lept_parse_string_raw`以及`lept_parse_string`分别处理字符串分离以及解析出来，这里原作者写了一个不容易察觉的bug，
为了性能使用了指针，但是在内部如果分配空间不足的时候会重新拷贝并生成新的对象，而旧指针仍指向将被销毁的对象。

`lept_object`的结构比`lept_value`更复杂些，是嵌套式的，`lept_object`又可以嵌套在`lept_value`中。

鉴于`lept_object`这种嵌套的性质， 与`lept_array`很像，所以解析的思路可以很类似。

