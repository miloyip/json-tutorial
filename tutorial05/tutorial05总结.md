1. 编写测试检验结果，由于给出的测试数据都是合法的，所以，可以放心检测。与前面的不同的地方在于，数组是复合类型，因此，对于每个数组元素，需要单独进行解析。已有的API `lept_get_array_element`可以实现剥离每个元素，然后进行检查。

2.在合理的位置加上`lept_parse_whitespace`即可

3.使用检测工具发现
```c++
# valgrind --tool=memcheck --leak-check=full ./test
==207== Memcheck, a memory error detector
==207== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==207== Using Valgrind-3.13.0 and LibVEX; rerun with -h for copyright info
==207== Command: ./test
==207==
227/227 (100.00%) passed
==207==
==207== HEAP SUMMARY:
==207==     in use at exit: 364 bytes in 6 blocks
==207==   total heap usage: 34 allocs, 28 frees, 4,539 bytes allocated
==207==
==207== 124 (120 direct, 4 indirect) bytes in 1 blocks are definitely lost in loss record 2 of 4
==207==    at 0x4C31B0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==207==    by 0x111936: lept_parse_array (in /json-tutorial/tutorial05/test)
==207==    by 0x111A66: lept_parse_value (in /json-tutorial/tutorial05/test)
==207==    by 0x111AFE: lept_parse (in /json-tutorial/tutorial05/test)
==207==    by 0x10CEE0: test_parse_array (in /json-tutorial/tutorial05/test)
==207==    by 0x1106F3: test_parse (in /json-tutorial/tutorial05/test)
==207==    by 0x110CA3: main (in /json-tutorial/tutorial05/test)
==207==
==207== 240 (96 direct, 144 indirect) bytes in 1 blocks are definitely lost in loss record 4 of 4
==207==    at 0x4C31B0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==207==    by 0x111936: lept_parse_array (in /json-tutorial/tutorial05/test)
==207==    by 0x111A66: lept_parse_value (in /json-tutorial/tutorial05/test)
==207==    by 0x111AFE: lept_parse (in /json-tutorial/tutorial05/test)
==207==    by 0x10D436: test_parse_array (in /json-tutorial/tutorial05/test)
==207==    by 0x1106F3: test_parse (in /json-tutorial/tutorial05/test)
==207==    by 0x110CA3: main (in /json-tutorial/tutorial05/test)
==207==
==207== LEAK SUMMARY:
==207==    definitely lost: 216 bytes in 2 blocks
==207==    indirectly lost: 148 bytes in 4 blocks
==207==      possibly lost: 0 bytes in 0 blocks
==207==    still reachable: 0 bytes in 0 blocks
==207==         suppressed: 0 bytes in 0 blocks
==207==
==207== For counts of detected and suppressed errors, rerun with: -v
==207== ERROR SUMMARY: 2 errors from 2 contexts (suppressed: 0 from 0)
```
就是说，`lept_parse_array`的某个位置malloc没有释放，实际上测试用例在`test.c`生成。我们在`lept_free`中合理安排，将所有申请的堆内存释放即可

4. 异常处理：因为得到的`json`字符串可能是不合法的，然而已有的代码框架不会将已经存储的值弹出，因此我们需要特判出异常情况，先弹出再返回

5.空悬指针问题：
这里返回指针会出现一个生命周期问题，当realloc分配的空间不足以在原位置连续时，则会拷贝到另一处，而之前的指针e就会变为空悬指针。