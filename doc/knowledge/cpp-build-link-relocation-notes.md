# C/C++ 编译、链接、静态库与重定位速记

## 1. 构建流程

```text
.cpp/.c
  ↓ 预处理：展开 include、宏、条件编译
.i/.ii
  ↓ 编译：语法/类型检查、优化、生成汇编
.s/.asm
  ↓ 汇编：汇编转机器码，生成目标文件
.o/.obj
  ↓ 链接：合并目标文件、解析符号、修补地址
可执行文件 / 静态库 / 动态库
```

CMake 不是编译器，它生成构建规则，真正执行的是 gcc/clang/msvc。

| CMake 指令 | 主要作用 |
|---|---|
| `target_include_directories` | 头文件搜索路径，影响预处理 |
| `target_compile_definitions` | 宏定义，影响预处理 |
| `target_compile_options` | 编译选项，如 `-O2`、`-Wall` |
| `add_executable` | 生成可执行文件目标 |
| `add_library(... STATIC ...)` | 生成静态库 `.a` |
| `add_library(... SHARED ...)` | 生成动态库 `.so` |
| `target_link_libraries` | 指定链接哪些库 |

---

## 2. `.o` 目标文件

`.o` 不是最终程序，而是可重定位目标文件，通常包含：

```text
机器码
符号表
重定位信息
未解析外部符号
调试信息，可选
```

原因：单个 `.o` 还不知道最终会和哪些 `.o`、哪些库链接，也不知道最终地址。

---

## 3. 符号表、未解析符号、重定位

### 符号表

记录 `.o` 里定义和引用的名字：

```text
函数名
全局变量名
静态变量名
未定义引用
```

例如：

```cpp
int add(int a, int b) {
    return a + b;
}
```

`add.o` 的符号表里会有：

```text
add：我定义了这个函数
```

### 未解析外部符号

当前 `.o` 用到了，但自己没定义的符号。

```cpp
int add(int, int);

int main() {
    return add(1, 2);
}
```

`main.o` 的符号表里更准确地说是：

```text
main：已定义符号，通常位于 .text 段
add：未定义符号，表示当前目标文件引用了它，但定义要等链接阶段从别的 .o/.a/.so 里找
```

真实工具里查看 `main.o`，常见类似：

```text
T main
U add        // C 情况下可能这样显示
U _Z3addii   // C++ 情况下常见为 name mangling 后的名字
```

换一个视角看，C/C++ 运行时启动文件，例如 `crt1.o` / `Scrt1.o`，会引用用户写的 `main`：

```text
crt1.o：引用 main
main.o：定义 main，引用 add
add.o：定义 add
```

所以“谁定义、谁引用”要看当前讨论的是哪个目标文件。

如果链接时没提供 `add.o`，会报：

```text
undefined reference to `add(int, int)'
```

### 重定位信息

重定位信息就是地址修补清单。

例如 `main.o` 里有：

```text
call add
```

但生成 `main.o` 时还不知道 `add` 最终地址，所以先留占位值，并记录：

```text
.text 段某个位置要调用 add，链接时请修补这里。
```

---

## 4. `.a` 静态库

`.a` 不是可执行文件，本质是多个 `.o` 的归档：

```text
libmylib.a
├── foo.o
├── bar.o
└── baz.o
```

所以：

```text
.a 本身不是已经定好地址的成品。
.a 里的 .o 仍然保留符号表和重定位信息。
```

链接可执行文件时：

```bash
g++ main.o libmylib.a -o app
```

链接器会：

```text
从 .a 里取出需要的 .o
解析符号
根据重定位信息修补地址
生成最终 app
```

注意：链接器通常不是把 `.a` 里所有 `.o` 都取出来，而是只取当前需要的 `.o`。

---

## 5. 绝对重定位和相对重定位

重定位信息不是简单的“绝对值”或“相对值”，而是一条规则：

```text
在某个位置，按照某种重定位类型，修补成某个符号相关的值。
```

常见有：

```text
绝对地址重定位：填符号最终地址
PC 相对重定位：填目标地址相对于当前位置的偏移
GOT/PLT 重定位：用于动态链接
```

---

## 6. `R_X86_64_PC32`

`R_X86_64_PC32` 拆开看：

```text
R       relocation，重定位
X86_64  x86-64 架构
PC      相对于程序计数器
32      32 位偏移
```

它常见于 `call` / `jmp`。

x86-64 上普通 `call` 指令形如：

```text
E8 xx xx xx xx
```

后面 4 字节不是目标函数绝对地址，而是：

```text
目标地址 - 下一条指令地址
```

例如：

```text
call 指令地址：       0x401000
下一条指令地址：      0x401005
add 函数地址：        0x401030
```

那么要填：

```text
0x401030 - 0x401005 = 0x2B
```

机器码类似：

```text
E8 2B 00 00 00
```

意思是：从下一条指令地址往后跳 `0x2B`，到达 `add`。

---

## 7. `addend` 是什么

常见重定位公式：

```text
结果 = S + A - P
```

含义：

```text
S = symbol address，符号最终地址
A = addend，附加值
P = place，被修补位置的地址
```

`addend` 就是额外加上的常量偏移。

### 例子 1：数组

```cpp
extern int arr[10];

int get() {
    return arr[3];
}
```

如果 `arr` 地址是 `0x600000`，`arr[3]` 偏移是 `12`，那么访问的是：

```text
arr + 12
```

这里：

```text
S = arr
A = 12
```

### 例子 2：结构体字段

```cpp
struct User {
    int id;
    int age;
};

extern User user;

int get_age() {
    return user.age;
}
```

`age` 字段偏移是 `4`，所以访问的是：

```text
user + 4
```

这里：

```text
A = 4
```

### 例子 3：call 指令

`call rel32` 的 CPU 语义是：

```text
下一条指令地址 + rel32
```

但重定位公式里的 `P` 常指被修补的 4 字节字段位置，不一定是下一条指令地址。

例如：

```text
call 起始地址：       0x401000
被修补字段 P：        0x401001
下一条指令地址：      0x401005
add 地址 S：          0x401030
```

希望填：

```text
0x401030 - 0x401005 = 0x2B
```

用公式：

```text
S + A - P
```

则：

```text
A = -4
```

所以：

```text
0x401030 - 4 - 0x401001 = 0x2B
```

---

## 8. 为什么动态库需要 `-fPIC`

动态库 `.so` 的加载地址不固定：

```text
这次加载到 0x7f1000000000
下次加载到 0x7f2000000000
```

如果代码里写死绝对地址，就会有问题：

```text
加载时要修改 .text 代码段
代码页不能被多个进程共享
某些 32 位重定位装不下 64 位高地址
```

`-fPIC` 表示 position independent code，位置无关代码。

它让编译器尽量生成：

```text
不依赖固定加载地址的代码
用相对地址
通过 GOT/PLT 间接访问外部变量/函数
```

---

## 9. GOT / PLT

### GOT

GOT = Global Offset Table，全局偏移表。

可以理解成动态库里的地址表：

```text
代码不直接写 global_value 的绝对地址，
而是去 GOT 表里取 global_value 的真实地址。
```

动态库加载时主要修改 GOT，而不是修改 `.text` 代码段。

### PLT

PLT = Procedure Linkage Table，过程链接表。

主要用于外部函数调用，例如：

```cpp
printf("hello\n");
```

PIC 代码通常不是直接写死 `printf` 地址，而是：

```text
call printf@PLT
printf@PLT 再通过 GOT 找到真正的 printf
```

---

## 10. 静态库链接进动态库的问题

`.a` 只是 `.o` 的集合。

如果 `.a` 里的 `.o` 不是用 `-fPIC` 编的，再拿它去生成 `.so`：

```bash
g++ -shared bar.o libfoo.a -o libbar.so
```

可能报：

```text
relocation R_X86_64_32 against symbol ... can not be used when making a shared object; recompile with -fPIC
```

意思是：

```text
这个静态库里的目标文件包含不适合动态库的重定位。
请用 -fPIC 重新编译。
```

CMake 写法：

```cmake
add_library(foo STATIC foo.cpp)

set_target_properties(foo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
```

或者全局：

```cmake
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
```

---

## 11. 最短记忆版

```text
.o 是半成品，里面有机器码、符号表、重定位信息。
.a 是很多 .o 的包，也还是半成品仓库。

符号表：记录名字。
未解析外部符号：我用到了但我没定义。
重定位信息：链接器之后要修补哪些位置。

R_X86_64_PC32：填相对偏移，不是填绝对地址。
addend：符号地址基础上额外加的常量。
-fPIC：生成位置无关代码，适合做动态库。
GOT/PLT：动态链接时用的间接地址表/跳转表。
```
