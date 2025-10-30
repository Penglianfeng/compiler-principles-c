# 编译原理实验 - 词法分析与自动机构造（仅使用 GCC）

## 实验环境
- 操作系统：Windows 11（PowerShell）
- 编译器：GCC（MinGW-w64/WinLibs）
- 语言：C

---


## 一、实验一：词法分析器（Untitled-1.c）

### 目标
将源程序字符流切分为 Token 序列，识别关键字、标识符、数字/字符串/字符常量、运算符、标点、注释，并能报告常见词法错误。

### 操作步骤（一步步做）
1) 编译词法分析器
```powershell
gcc Untitled-1.c -o lexer.exe
```

2) 运行并观察各测试集（可逐个运行，也可将输出重定向到文件）
```powershell
# 综合测试
.\lexer.exe test_sample.c

# 错误检测
.\lexer.exe test_errors.c

# 数字常量
.\lexer.exe test_numbers.c

# 标识符
.\lexer.exe test_identifiers.c

# 注释
.\lexer.exe test_comments.c

# 可选：保存输出
.\lexer.exe test_sample.c > out_sample.txt
```

### 词法分析器如何工作（规则与优先级）
- 关键字表匹配：识别到形如字母/下划线开头且由字母数字下划线组成的串后，先查关键字表，命中则记为 KEYWORD，否则为 IDENT。
- 数字常量：
	- 十进制整数：连续数字。
	- 十六进制：以 0x/0X 开头，后接十六进制数字（0-9a-fA-F）。
	- 浮点数：含小数点或指数（e/E，可带正负号），如 1.23、1e5、1.2E-3。
	- 注意：当前实现“识别数字”的入口是“首字符为数字”，因此形如“.456”会被切分为 PUNC(".") 与 INT("456")，这是预期设计选择。
- 字符串/字符常量：成对引号包围，支持转义（如 \n、\t、\\、\"、\' 等）；未闭合将报错。
- 注释：
	- 单行注释 // 直到行尾。
	- 块注释 /* ... */，未闭合将报错。
- 运算符：采用最长匹配（如先尝试“==”“++”“--”，再退化到“=”“+”“-”）。
- 标点：如 (){}[];,.:?&|^~# 等按单字符输出。
- 非法字符：如 @、$、` 等直接产生 ERROR。

### 测试集解析：为什么会得到这些结果

下列说明帮助你对照输出理解“为什么会这样”。括号内为典型 Token 形式：(种类, "词素", 行, 列)。

1) 综合测试：`test_sample.c`
- 关键字与标识符：`int num` → (KEYWORD,"int"), (IDENT,"num")。
- 16 进制：`0x1A2B` → (HEX,"0x1A2B")。
- 浮点与科学计数法：`3.14159` → (FLOAT,"3.14159")；`1.5e-10` → (FLOAT,"1.5e-10")。
- 字符与转义：`'\n'` → (CHAR,"\n")。
- 字符串：`"Hello, World!"` → (STRING,"...")。
- 运算符最长匹配：`== ++ -- <= >=` 分别识别为双字符 OP；单目/双目 `+ - * / % = < >` 识别为单字符 OP。
- 标点与分隔：`()[]{},;.` 等输出为 PUNC。
- 注释：`// ...` 与 `/* ... */` 输出为 COMMENT（或被忽略，取决于实现；本实现会产出 COMMENT）。

2) 错误检测：`test_errors.c`
- 未闭合字符串：遇到起始 `"`，未在行内或文件结束前找到闭合 `"` → (ERROR,"Unterminated string literal")。
- 未闭合字符常量：起始 `'` 未找到闭合 `'` → (ERROR,"Unterminated char literal")。
- 未闭合块注释：`/*` 未匹配到 `*/` → (ERROR,"Unterminated block comment")。
- 非法字符：`@ $ `` ` → 直接产出 (ERROR,"@/$/`")。因此该文件的输出会包含多处 ERROR。

3) 数字常量：`test_numbers.c`
- 整数：`0 12345` → (INT,"0"), (INT,"12345")。
- 十六进制：`0xabcdef 0XABCDEF` → (HEX,"...")。
- 浮点：`123. 0.0 123.456` → (FLOAT,"...")。
- 科学计数法：`1e5 1.23e-5 1.23E+10` → (FLOAT,"...")。
- 特别说明：`.456` 会被切分为 PUNC(".") 与 INT("456")（因为当前实现只有“数字开头”才进入数值扫描）。

4) 标识符：`test_identifiers.c`
- 规则 `[A-Za-z_][A-Za-z0-9_]*`：以字母或下划线开头，后续由字母/数字/下划线组成。
- 关键字判定：如出现 `int`、`for` 等，先匹配为 IDENT，再经关键字表提升为 KEYWORD。
- 典型：`my_variable_123` → (IDENT,"my_variable_123")；`_var`、`__var` 合法；单字母 `i,j,k` 也为 IDENT。

5) 注释：`test_comments.c`
- `//` 单行：产生一个 COMMENT，直到换行结束。
- `/* ... */` 多行：产生一个 COMMENT，包含块内文本。内部若出现 `//` 不会生效（在注释内按普通字符处理）。
- 代码中嵌入注释：`int /* 注释 */ x = 10;` 会将注释整体识别为 COMMENT，前后仍能正确解析为 `int`、`x`、`=`、`10`、`;`。

---

## 二、实验二：从 NFA 到最简 DFA（Untitled-2.c）

### 目标
基于标识符正规式 `letter(letter|digit)*`：
1) 构造 NFA；2) 子集构造法确定化为 DFA；3) Hopcroft 算法最小化 DFA；4) 打印转换表/矩阵。

### 操作步骤
1) 编译
```powershell
gcc Untitled-2.c -o nfa_to_dfa.exe
```

2) 运行（程序内置用例，无需参数）
```powershell
.\nfa_to_dfa.exe
```

### 解读
1) Identifier NFA：
	 - 状态少且可能含不确定性（本实现用“字母=0、数字=1”的抽象字母表，q0 在 letter→q1，q1 在 letter/digit 自环）。
2) DFA before minimization：
	 - 由子集构造得到；每个 DFA 状态代表一个 NFA 状态集合；
	 - 未定义迁移会被补到“陷阱态”（ensure_total_dfa 实现）。
3) Minimal DFA：
	 - 通过 Hopcroft 划分等价类并合并，得到状态最少的等价自动机；
	 - 打印新的起始态、接收态及转换矩阵。

直观理解：标识符的语言是“首字符必须是字母，其后可为字母或数字”。因此最小化 DFA 会保留：
- 起始非接收态（尚未读到合法首字母时的等价类）、
- 接收态（已读到 1+ 个合法字符的等价类，letter/digit 自环）、
- 陷阱态（读到非法首字符或之后出现非法续字符时的等价类）。

---

## 三、附：一键指令速查
```powershell
# 进入目录
cd "C:\\Users\\86187\\Desktop\\source\\C\\编译原理"

# 实验一：编译 + 运行
gcc Untitled-1.c -o lexer.exe
.\lexer.exe test_sample.c
.\lexer.exe test_errors.c
.\lexer.exe test_numbers.c
.\lexer.exe test_identifiers.c
.\lexer.exe test_comments.c

# 实验二：编译 + 运行
gcc Untitled-2.c -o nfa_to_dfa.exe
.\nfa_to_dfa.exe
```

---

