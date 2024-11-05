[![Open in Codeanywhere](https://img.shields.io/badge/Open%20in-Codeanywhere-7f3f97)](https://app.codeanywhere.com/#https://github.com/miloyip/json-tutorial)

# 从零开始的 JSON 库教程

* Milo Yip
* 2016/9/15

也许有很多同学上过 C/C++ 的课后，可以完成一些简单的编程练习，又能在一些网站刷题，但对于如何开发有实际用途的程序可能感到束手无策。本教程希望能以一个简单的项目开发形式，让同学能逐步理解如何从无到有去开发软件。

为什么选择 JSON？因为它足够简单，除基本编程外不需大量技术背景知识。JSON 有标准，可按照标准逐步实现。JSON 也是实际在许多应用上会使用的格式，所以才会有大量的开源库。

这是一个免费、开源的教程，如果你喜欢，也可以打赏鼓励。因为工作及家庭因素，不能保证每篇文章的首发时间，请各位见谅。

## 对象与目标

教程对象：学习过基本 C/C++ 编程的同学。

通过这个教程，同学可以了解如何从零开始写一个 JSON 库，其特性如下：

* 符合标准的 JSON 解析器和生成器
* 手写的递归下降解析器（recursive descent parser）
* 使用标准 C 语言（C89）
* 跨平台／编译器（如 Windows／Linux／OS X，vc／gcc／clang）
* 仅支持 UTF-8 JSON 文本
* 仅支持以 `double` 存储 JSON number 类型
* 解析器和生成器的代码合共少于 500 行

除了围绕 JSON 作为例子，希望能在教程中讲述一些课题：

* 测试驱动开发（test driven development, TDD）
* C 语言编程风格
* 数据结构
* API 设计
* 断言
* Unicode
* 浮点数
* Github、CMake、valgrind、Doxygen 等工具

## 教程大纲

本教程预计分为 9 个单元，第 1-8 个单元附带练习和解答。

1. [启程](tutorial01/tutorial01.md)（2016/9/15 完成）：编译环境、JSON 简介、测试驱动、解析器主要函数及各数据结构。练习 JSON 布尔类型的解析。[启程解答篇](tutorial01_answer/tutorial01_answer.md)（2016/9/17 完成）。
2. [解析数字](tutorial02/tutorial02.md)（2016/9/18 完成）：JSON number 的语法。练习 JSON number 类型的校验。[解析数字解答篇](tutorial02_answer/tutorial02_answer.md)（2016/9/20 完成）。
3. [解析字符串](tutorial03/tutorial03.md)（2016/9/22 完成）：使用 union 存储 variant、自动扩展的堆栈、JSON string 的语法、valgrind。练习最基本的 JSON string 类型的解析、内存释放。[解析字符串解答篇](tutorial03_answer/tutorial03_answer.md)（2016/9/27 完成）。
4. [Unicode](tutorial04/tutorial04.md)（2016/10/2 完成）：Unicode 和 UTF-8 的基本知识、JSON string 的 unicode 处理。练习完成 JSON string 类型的解析。[Unicode 解答篇](tutorial04_answer/tutorial04_answer.md)（2016/10/6 完成）。
5. [解析数组](tutorial05/tutorial05.md)（2016/10/7 完成）：JSON array 的语法。练习完成 JSON array 类型的解析、相关内存释放。[解析数组解答篇](tutorial05_answer/tutorial05_answer.md)（2016/10/13 完成）。
6. [解析对象](tutorial06/tutorial06.md)（2016/10/29 完成）：JSON object 的语法、重构 string 解析函数。练习完成 JSON object 的解析、相关内存释放。[解析对象解答篇](tutorial06_answer/tutorial06_answer.md)（2016/11/15 完成）。
7. [生成器](tutorial07/tutorial07.md)（2016/12/20 完成）：JSON 生成过程、注意事项。练习完成 JSON 生成器。[生成器解答篇](tutorial07_answer/tutorial07_answer.md)（2017/1/5 完成）。
8. [访问与其他功能](tutorial08/tutorial08.md)（2018/6/2 完成）：JSON array／object 的访问及修改。练习完成相关功能。
9. 终点及新开始：加入 nativejson-benchmark 测试，与 RapidJSON 对比及展望。

## 关于作者

叶劲峰（Milo Yip）现任腾讯 T4 专家、互动娱乐事业群魔方工作室群游戏客户端技术总监。他获得香港大学认知科学学士（BCogSc）、香港中文大学系统工程及工程管理哲学硕士（MPhil）。他是《游戏引擎架构》译者、《C++ Primer 中文版（第五版）》审校。他曾参与《天涯明月刀》、《斗战神》、《爱丽丝：疯狂回归》、《美食从天降》、《王子传奇》等游戏项目，以及多个游戏引擎及中间件的研发。他是开源项目 [RapidJSON](https://github.com/miloyip/rapidjson) 的作者，开发 [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark) 比较 41 个开源原生 JSON 库的标准符合程度及性能。他在 1990 年学习 C 语言，1995 年开始使用 C++ 于各种项目。
