# zcc

zero c compiler，一个从零开始写的编译器，不仅仅为了学习编译原理

> 命名规范如下

- 类型大写首字母驼峰
- 变量和函数名小写下划线
- 常量大写下划线
- 驼峰和下划线不混用

## 优势

由于是用 c 写的编译 c 代码的编译器，项目中并没有采用任何第三方的 `.so` 动态链接库或者是 `.a` 静态链接库，有也只是用了标准库里面的函数，所以基本遵从 "从零开始写" 这一原则

## Features

- [x] 支持基本的汇编代码生成
- [x] 支持 全局/局部 变量解析
- [x] 支持 `if`/`while`/`for`/`switch` 语句的解析
- [x] 支持 `struct`/`union`/`sizeof`/`static`/`extern` 等关键字声明的语句的解析
- [x] 支持 全局/局部 的 数组/指针 变量的赋值与被赋值
- [x] 支持 函数声明/定义
- [x] 支持 函数参数的解析
- [x] 支持 部分不带括号的 `return` 语句
- [x] 支持 打印 AST
- [x] 支持 first time 自举编译

## 用法和调试

```bash
git clone git@github.com:strugglebak/zcc.git
cd zcc
# make install 需要权限
make clean && make install

./parser your_c_code.zc
```

## 测试

```bash
make clean && make test
```

## Roadmaps

- [ ] 完善 ARM 后端相关汇编代码生成
- [ ] 完善 `BNF` 语法（这里可能会使用一些 ``BNF` 工具来做）
- [ ] 支持 `...` token 的解析，用来检查函数的实参和形参个数
- [ ] 支持 `short` 类型解析
- [ ] 优化寄存器分配策略
- [ ] 优化 AST 结构
- [ ] 汇编代码生成优化
- [ ] 增加 Debugging 输出
- [ ] 支持在其他平台运行编译器

## 协议

[MIT](./LICENSE)
