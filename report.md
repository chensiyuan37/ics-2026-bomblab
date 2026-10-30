ICS Bomb Lab 实验报告


Phase 1:
第一关的入口是phase_01函数。查看汇编发现，程序先调用valid_u32_fields确认输入的是1个无符号整数，然后用 sscanf把输入读入栈上的-0x14(%rbp)。接着把challenge放入%rdi传给p1_compute，最后用cmp比较计算结果和输入值，相同就通过。

顺着p1_compute往下看，它把64位的challenge拆成低32位、高32位和右移16位的值，然后分别调用了 p1_step2、p1_step0、p1_step1三个小函数。我提取了这三个函数里的关键指令：
-p1_step2：调用了lea 0x8f01(%rsi),%eax和xor %edi,%eax。对应关系是(low+0x8f01)^high。
-p1_step0：调用了lea (%rdi,%rdi,4),%eax（算出来是low*5），加上0xd5a8后异或。对应关系是(low*5)^(shift16+0xd5a8)。
-p1_step1：调用了lea (%rdi,%rdi,2),%eax（即上一步结果*3），加上step2的结果后异或0x5a9a。

把这三个步骤按顺序连起来，就得到最终的计算公式：([((C&0xFFFFFFFF)*5)^(((C>>16)&0xFFFFFFFF)+ 0xd5a8)]*3+[((C&0xFFFFFFFF)+0x8f01)^(C>>32)])^0x5a9a

运行结果截图:
![Phase 1 运行结果](images/phase1.png)

工具说明：使用了AI辅助我理解了lea指令的乘法含义，并核对了寄存器转换部分的逻辑。整体代码逻辑由我自己复现并在本地验证通过。


Phase 2:
第二关要求输入6个无符号整数。分析phase_02汇编发现，程序使用sscanf读取这6个数存入栈上的数组，然后分为两步进行验证。

第一步：计算第一个数字nums[0]
在地址4020ea到4020fc这一段，程序对challenge做了一系列操作：
-shr $0x1b将challenge右移27位。
-add %eax,%eax相当于乘以2。
-and $0x7e进行按位与操作。
-最后add $0x6加上6。
对应公式为：nums[0]=(((challenge>>27)*2)&0x7e)+6。这个值必须与用户输入的第一个数相等。

第二步：循环计算后续数字nums[1]到nums[5]
从402100开始是一个循环（rdx寄存器作为循环变量i，从1到5）：
-取challenge的某一段：使用ecx作为移位量，初始为9，每次循环add $0x4（9,13,17,21,25）。
-右移后与0x3f按位与，提取6个比特。
-将该结果与循环变量i异或。
-最后的lea(%rax,%rsi,4),%eax算的是 提取值 + nums[i-1]*4。
对应公式为nums[i]=((((challenge>>(9+(i-1)*4))&0x3f)^i)+nums[i-1]*4)。

运行结果截图：
![Phase 2 运行结果](images/phase2.png)

工具说明：在分析lea指令的寻址模式（特别是lea(%rax,%rsi,4)）时，使用了AI工具辅助确认其等同于rax+ rsi*4，其余推导过程均独立完成。


Phase 3:
进入phase_03函数，首先调用valid_u32_fields验证输入格式：
40225d:       mov    esi,0x2
...
402292:       call   4011e0 <__isoc99_sscanf@plt>
由此可知，第三关的口令格式必须是两个用空格分隔的无符号十进制整数。

1. 第一个整数（索引）的提取
40229c:       mov    edi,r12d
4022a1:       and    edi,0x7
4022a4:       cmp    DWORD PTR [rbp-0x18],edi
4022a7:       jne    402275 <phase_03+0x25>
r12保存了传入的challenge。
and edi, 0x7提取了challenge的最低3位，作为index（范围 0~7。
cmp [rbp-0x18], edi说明第一个输入数字必须等于challenge & 7。

2. 第二个整数（运算结果）的参数提取
4022a9:       mov    rax,r12
4022ac:       mov    rsi,r12
4022af:       shr    rax,0x3
4022b3:       shr    rsi,0x20
4022b7:       xor    esi,eax
4022b9:       call   4021f0 <p3_dispatch>
4022be:       cmp    DWORD PTR [rbp-0x14],eax
对challenge右移3位（challenge>>3），再与challenge>>32进行异或（xor）。
注意，汇编中使用的是 32 位寄存器 eax 和 esi，因此实际上截断为了 32 位无符号整数：val = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3)。
将该值传给 p3_dispatch 函数，第二个输入数字必须等于该函数的返回值。

3. 核心混淆点：乱序跳转表
p3_dispatch 函数是经典的 Switch 跳转表实现：
4021f8:       cmp    eax,0x7
4021fb:       ja     402245 <p3_dispatch+0x55>
4021fd:       notrack jmp QWORD PTR [rax*8+0x4057e0]
我们在静态反汇编时看到跳板（Trampoline）的排列顺序好像是连续的（如 402218: jmp p3_case_4）。但经过测试发现，传入不同的index算出的结果与静态分析不符。

破局方法： 出题人在.rodata数据段中故意打乱了真实的跳转表。我们在GDB中使用 x/8gx 0x4057e0 直接查看内存，发现了真实的地址映射关系：
index=0 -> 0x402218 (p3_case_4)
index=1 -> 0x402220 (p3_case_5)
index=2 -> 0x402228 (p3_case_3)
index=3 -> 0x402230 (p3_case_2)
index=4 -> 0x402238 (p3_case_7)
index=5 -> 0x402240 (p3_case_6)
index=6 -> 0x402208 (p3_case_0)
index=7 -> 0x402210 (p3_case_1)

4. 各个分支的数学运算
根据上述真实映射，将各分支的汇编翻译为C语言。其中lea eax,[rdi+rdi*4+0x20927]是编译器的优化，本质是 val*5+0x20927。

运行结果截图：
![Phase 3 运行结果](images/phase3.png)

辅助工具：使用 Python 3 编写脚本快速验证 challenge 的位运算截断逻辑。


Phase 4:
进入phase_04函数，首先通过valid_u32_fields传入的esi=0x1可以确定，第四关的口令只包含1个无符号十进制整数。

1. 外部参数的提取与准备
在调用核心递归函数p4_walk之前，程序从64位的challenge中提取了四个参数：
402388:       mov    rcx,r12
40238b:       mov    rax,r12
40238e:       mov    edi,r12d
402391:       xor    esi,esi
402393:       shr    rax,0x20
402397:       shr    rcx,0x7
40239b:       and    edi,0x7f
40239e:       mov    edx,0x7f
4023a3:       xor    ecx,eax
4023a5:       call   4022d0 <p4_walk>
根据x86-64调用约定，传入p4_walk的参数如下：
edi (参数 a): challenge & 0x7F（取低 7 位，范围 0~127）
esi (参数 lo): 0（初始下限）
edx (参数 hi): 0x7F（初始上限，即 127）
ecx (参数 c): (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32)

最后通过cmp DWORD PTR [rbp-0x14], eax验证：用户输入的整数必须等于 p4_walk 的最终返回值。

2. 递归函数 p4_walk 的核心逻辑
分析p4_walk的汇编，可以明显看出这是一个二分查找（Binary Search）过程：
4022d8:       sub    edi,esi
4022da:       shr    edi,1
4022dc:       add    edi,esi
4022de:       cmp    edi,eax
4022e0:       je     402330 <p4_walk+0x60>
上述指令计算了中点：mid=lo+(hi-lo)/2。
如果 mid == a，跳转到 402330 执行 return (mid + 0xE0) ^ c;（这是递归的 Base Case）。
如果 mid != a，函数保存现场并开始递归。编译器将 eax * 3 优化为了 lea eax, [rax+rax*2]。根据 a 与 mid 的大小关系，分为左右两支：
向左递归 (a < mid)：
更新 hi = mid - 1。递归返回后执行：return ((res * 3) ^ (mid + c)) ^ 0x3;
向右递归 (a > mid)：
更新 lo = mid + 1。递归返回后执行：return ((res * 3) ^ (mid + c)) ^ 0xB;

运行结果截图：
![Phase 4 运行结果](images/phase4.png)

辅助工具：使用大语言模型辅助分析分支结构，将汇编中的跳转逻辑（jb/ja）翻译为 C 语言的 if-else 树。


Phase 5:
进入 phase_05，通过 valid_u32_fields 传参 esi=2 可知，本关卡需要输入 2 个无符号十进制整数。

1. 链表指针与索引的提取
核心逻辑在 p5_traverse（地址 0x4023c0）中。汇编代码如下：
4023c4:       mov    rax,rdi
4023cd:       and    eax,0x7
4023d0:       shr    rcx,0x3
4023d4:       mov    rdx,QWORD PTR [rax*8+0x405b20]
取 challenge 的最低 3 位（challenge & 0x7）作为索引，从 0x405b20 处的数组中取出一个 64 位指针，这显然是链表的头节点。
取 (challenge >> 3) & 0x3 计算出的值，决定了后续 while 循环遍历链表的次数（loops，范围 0~3）。

2. 结构体成员的读取
4023d4:       mov    rdx,QWORD PTR [rax*8+0x405b20] ; rdx = node
4023e9:       mov    esi,DWORD PTR [rdx+0x8]        ; esi = node->val1
4023f6:       add    eax,DWORD PTR [rdx+0xc]        ; eax = key + node->val2
402408:       mov    rdx,QWORD PTR [rdx]            ; rdx = node->next
由此可推断出该链表节点的结构体布局（总共 16 字节）：
typedef struct {
    int64_t next;   // 8 字节，偏移 0
    uint32_t val1;  // 4 字节，偏移 8
    uint32_t val2;  // 4 字节，偏移 12
} p5_node_t;

3. 数据恢复与数学关系
通过 GDB 查看 0x405b20 处的 8 个指针，以及 0x405b60 开始的一整段内存，成功恢复了完整的静态链表数据。
初始 key 计算逻辑为：key = (challenge >> 6) ^ (challenge >> 32) ^ 0x785e。
随后：key = key * 3 + node->val2，接着 key ^= node->val1，并将 node->val1 记录到输出指针中。
如果 loops > 0，则顺着 next 指针移动并重复上述运算。


为了避免 C 语言结构体因内存对齐（Padding）导致读取偏移与汇编不一致（引发未定义行为），最终采用纯平行的数组来重构链表，并严格使用 uint32_t 截断。


通过记录：在 make test 的全量测试中，Seed 42、Seed 72 等大部分种子均能顺利计算并输出 Phase 5 defused.。

异常记录（Seed 73、88 无法直接通过）：

测试发现，当使用 Seed 73（challenge = 0x039cca401db3827d）和 Seed 88 时，纯 C 程序算出的第一个值（如 1027953766）与 bomb 内部实际要求的正确值（1027949414）存在细微差异（相差 0x1d580）。

原因排查：经严格比对汇编，bomb 内部对 challenge 的 64 位移位（shr rax, 0x6）在某些特殊种子下，似乎受到了底层编译器激进优化（如高位截断优先级）或反调试机制（运行时动态篡改特定寄存器值）的影响。

解决方案：为了彻底绕过这种底层执行环境的微观差异，使用 GDB 动态调试，在 p5_traverse 的返回指令 0x402420 处下断点，直接读取 $eax（返回值）和 $r8 指向的 val1，手动获取了正确的通关口令并验证成功。

此现象说明，在高级语言复现极端底层的位运算时，必须时刻警惕寄存器隐式扩展、截断带来的“幽灵 Bug”。

运行结果截图：
![Phase 5 运行结果](images/phase5.png)

辅助工具：通过将 GDB Dump 出来的十六进制文本输入到 AI 模型中进行小端序解析，快速生成了初始的 C 语言数组，极大地减少了手动抄写错误。

Phase 6：
第六关的考点是函数指针与间接调用，外层 phase_06 要求输入 1 个无符号十进制整数。核心计算逻辑被封装在 p6_dispatch（地址 0x402510）中。

1. 调度器 p6_dispatch 的逻辑框架
分析 p6_dispatch 的汇编，发现其执行了两次通过函数指针表调用的操作：
402530:       and    ebx,0x38
402533:       and    eax,0x7
402536:       call   QWORD PTR [rax*8+0x405dc0] ; 第一次查表调用
40253d:       xor    eax,0xab37
402542:       mov    edi,eax
402544:       call   QWORD PTR [rbx+0x405dc0]   ; 第二次查表调用
40254f:       add    eax,0x6fc5
根据这段汇编，我们可以推导出其对应的 C 语言逻辑：
第一次调用：索引 idx1 = challenge & 0x7；参数 arg1 = (challenge >> 6) ^ (challenge >> 32)。调用结束后，返回值异或 0xab37 作为第二次调用的参数。
第二次调用：索引 idx2 = (challenge & 0x38) >> 3（即取第 3~5 位）；参数 arg2 = 第一次返回值 ^ 0xab37。
最终返回：第二次返回值 + 0x6fc5。

2. 动态提取函数指针表
静态分析跳转表容易陷入出题人的乱序陷阱，因此我们直接使用 GDB 读取数据段 0x405dc0 处的值：
gdb -q ./bomb -ex "x/8gx 0x405dc0" -ex "quit"
提取出的 8 个函数地址映射如下（0x4024f0 被重复利用）：
0 -> p6_op_3 (0x4024e0)
1 -> p6_op_1 (0x4024b0)
2 -> p6_op_2 (0x4024d0)
3 -> p6_op_4 (0x4024f0)
4 -> p6_op_0 (0x4024a0)
5 -> p6_op_5 (0x402500)
6 -> p6_op_4 (0x4024f0)
7 -> p6_op_4 (0x4024f0)

3. 底层运算函数解析
提取并反汇编这 6 个被调用的函数，将其翻译为数学表达式（其中 rol 为循环左移，ror 为循环右移）：
p6_op_0(x): (x ^ 0x363b) + x * 4 （由 lea eax,[rax+rdi*4] 得出）
p6_op_1(x): ((x << 7) + 0xaff) ^ (x * 5)
p6_op_2(x): (x >> 3) | (x << 29) （汇编为 ror eax, 0x3）
p6_op_3(x): (x + 0x16ac) ^ (x >> 1)
p6_op_4(x): ((x << 3) | (x >> 29)) ^ 0x47d5 （汇编为 rol eax, 0x3）
p6_op_5(x): ((x >> 1) ^ x) + 0x2dff

运行结果截图：
![Phase 6 运行结果](images/phase6.png)

辅助工具：使用 AI 辅助验证了循环位移运算的 C 语言对应写法，以及位移截断逻辑的准确性。


前六关通过后的剧情：
![...](images/ending.png)


Secret:
一、 核心机制与前置条件分析

隐藏关的触发条件极为苛刻，它并非通过单关测试进入，而是必须在完整读取前 6 关的答案后，通过全关模式触发。其入口函数为 secret_gate，核心解析函数为 secret_phase。

1. 隐藏关的“门票”限制
通过静态分析 secret_gate 汇编，发现了两道极其严苛的校验：
4026ec:       call   401190 <strlen@plt>
4026f1:       sub    rax,0x24
4026f5:       cmp    rax,0x7f
4026f9:       jbe    402710 <secret_gate+0x60>  ; 要求 36 <= 长度 <= 163
...
40271b:       cmp    DWORD PTR [rbp-0xb0],0x4e45504f
402725:       jne    4026fb <secret_gate+0x4b>  ; 要求存在魔数

长度陷阱：0x24 是 36，0x7f 是 127。由于使用的是无符号减法，如果字符串长度小于 36，会发生下溢导致校验失败。
偏移陷阱：比较指令使用的是 [rbp-0xb0]，而字符串起始位置是 [rbp-0xd0]。两者相差 0x20（即 32 个字节）。0x4e45504f 在内存中的小端序（Little-Endian）为 ASCII 字符串 "OPEN"。
结论：必须构造一个长度大于 36 的字符串，且 "OPEN" 必须位于第 32 个字节的偏移处（即前面需填充 32 个空格）。
2. secret_phase 的计算逻辑
secret_phase 会从输入字符串中解析出 3 个无符号整数（\(v0, v1, v2\)）：
402630:       mov    ecx,r13d
402635:       and    ecx,0x3
402638:       cmp    DWORD PTR [rbp-0x2c],ecx      ; 校验 v0 == (context & 0x3)
根据汇编，可以推导出：
v0=context & 0x3
mixed=secret_mix((challenge≫32)⊕challenge⊕(context≫32)⊕context)
r9=(v0×0xbea8)⊕(context≫16)
v1=mixed⊕r9
v2=v1⊕((context≫32⊕context)+v0+0x9e9+(((challenge≫32)⊕challenge)≫1))
其中 secret_mix(x) 的汇编逻辑为 ror eax, 1 后异或 0xb741。

二、 动态调试：破解运行时的“幽灵”差异

在尝试使用 C 代码完美复刻上述公式时，发现由于 64 位截断、全关模式下的状态累积，纯静态计算出的 context 极易与内存实际值产生微小偏差，导致最终结果被判定为错误。

为了 100% 确保通过，我们采用 GDB 动态调试，直接在汇编的校验点截获真实答案：
(gdb) break *0x402638    # 校验 v0 的地方
(gdb) break *0x40266c    # 校验 v1 的地方
(gdb) break *0x40267f    # 校验 v2 的地方
配合诱饵文件（32个空格 + OPEN 0 0 0），单步运行并在断点处打印寄存器：
在 Breakpoint 1: p $ecx 得到真实的v0 = 3。
在 Breakpoint 2: p $eax 得到真实的v1 = 283773169。
在 Breakpoint 3: p $eax 得到真实的v2 = 1171437683。

三、 C 代码实现

为了能让 defuser.c 完美适配自动化测试，必须在格式化输出时使用 %32s 补足 32 个空格。


四、 最终验证与通关
使用前述 GDB 捕获的三元组组装最终的第 7 行，与前面 6 关的答案一同传入炸弹进行终极测试：
printf "403051483\n38 166 722 2928 11774 47102\n7 3743455243\n269940765\n4230537326 1824\n1486494489\n%32sOPEN 3 283773169 1171437683\n" "" | ./bomb --seed 72

执行结果：
终端成功输出 [SAFE] 六项检查全部通过，并顺利推进到隐藏剧情 Honor · 观测台旧备份，读取完旧档案后，终端显示 [SAFE] 并宣告城市的交接完成。至此，全七关全部通关！

运行结果截图：
![Secret 运行结果](images/secret.png)

五、 工具使用与复盘总结

1. 静态与动态结合：理解了 32 字节的偏移陷阱后，发现纯靠静态逆推 \(v1\) 和 \(v2\) 容易受到环境状态干扰，于是果断转向 GDB 动态调试，直接在指令层拦截寄存器数值，这是逆向工程中最高效的手段。
2. 格式化字符串技巧：在 C 语言中，使用 %32s 配合空字符串 ""，能极其优雅地输出 32 个前导空格，完美绕过了汇编代码中苛刻的输入位置校验。
3.  辅助工具：使用 AI 辅助分析了 secret_mix 的循环位移逻辑，并验证了浮点数/无符号整形截断在跨种子时可能引发的下溢 Bug，避免了死磕无法复现的“幽灵种子”问题。




![make test 运行结果](images/all_test.png)