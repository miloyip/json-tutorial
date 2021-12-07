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
重构`lept_parse_string`为`lept_parse_string_raw`以及`lept_parse_string`分别处理字符串分离以及解析出来，