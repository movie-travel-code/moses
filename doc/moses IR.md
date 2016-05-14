# moses IR

------

moses IR是对LLVM IR的简化，由于moses采用垃圾回收，所以需要在IR中记录关于垃圾回收的信息。由于现阶段moses只有两种内置类型 **int** 和 **bool** ，没有pointer，没有Array等复杂类型，所以IR设计只取LLVM IR的一个子集即可。由于取用LLVM IR的一个子集，所以moses IR自然也是SSA-based的中间表示形式，我们采取LLVM采取的方法，在中间代码生成的时候，没有直接生成标准SSA形式，而是后面通过一个 **mem2reg** 的过程将非SSA的代码promote成为SSA形式的代码。

> * 介绍
> * Identifiers
> * High Level Structure
> * Structure Types

------

## 介绍

moses IR打算像LLVM IR一样承担起moses中主要的角色，因为moses实现的主要目的就是熟悉编译中各种改变，所以moses会尽可能的应用各种编译优化概念。由于LLVM IR是给予寄存器形式的中间表示，可以方便在其上实现很多种优化概念。

我们首先会遍历AST生成近SSA形式的中间表示，然后将其规格化为SSA的moses IR。然后实现一个虚拟机来解释这些byte code，并通过虚拟机进行垃圾回收的操作。后面我们会尝试直接生成二进制，并将垃圾回收做到可执行文件中（但是对于垃圾回收，我还不是很精通）。所以先实现第一步，中间代码生成，然后构建一个虚拟机并实现简单的垃圾回收。

***Note:其实在moses中是没有必要生成AST的，moses太简单，可以使用SDT在不构建AST的情况下进行code generation.***


----------


## Identifiers
moses IR的identifier主要来自于两种情形：global和local。Gloabl identifiers(functions, global variables, class) 以字符'@' 开头. Local identifiers(register names, typs) 以字符'%'开头。另外，对于identifiers来说有三种不同的格式，分别用于不同的目的:

> * 具名值（named values）由带有特定前缀的字符串表示。例如：%foo, @DivisionByZero, %a.really.long.identifier. 
> * 匿名值（unamed values）由带有特定前缀的一组无符号数表示。例如：%12, @2, %44.
> * 常量

moses IR要求identifier特有前缀的原因类似于LLVM，一方面防止和保留字的名字冲突，如果保留字以后需要扩充的话会方便一些。另一方面，对于匿名值（unamed identifiers）来说，编译器在创建新的临时变量的时候，不需要考虑符号表中的冲突问题。

moses IR中的保留字（reserved words）参考LLVM IR，包括操作符（'add', 'ret'等），基本类型名（'void', 'i32'等）以及其他保留字。由于identifier以'@'和'%'开头，所以identifier不会和保留字冲突。

下面是moses IR关于变量 '%x' 乘以8的示例代码：

简单的方式：
```
%result = mul i32 %x, 8
```

强度消减的方式：
```
%result = shl i32 %x, 3
```

最复杂的方式：
```
%0 = add i32 %x, %x
%1 = add i32 %0, %0
%result = add i32 %1, %1
```

从最后一种方式可以看到：
> * 注释以';'开始直到行尾。
> * 当计算结果没有赋值给具名变量（named values）时，需要创建新的匿名临时变量。
> * 匿名临时变量通过数字增序来进行编号。注意，如果存在多个基本块的话，基本块也会参与编号。


----------
## High Level Structure
moses的设计还比较简陋，不支持分离编译，多疑不支持LLVM IR中的Module Type和Linkage Type。

moses IR采用两种调用惯例，一种是普通的，一种是快速的调用惯例。
调用惯例基本关于caller和callee如何协作，主要包括参数传递方式和顺序、返回值传递、栈惯例。

 1. 参数的传递方式
 大部分时候x86都是使用栈传递参数（调用前caler push实参，callee运行时，以帧指针为参考地址从栈中取出实参值），但有些情况编译优化使用寄存器传递参数，以提高性能。所以caller和callee关于参数的传递必须协商好。

 2. 参数的传递顺序
 caller将参数压入栈中，callee再从栈中取出参数。对于多参数的函数调用，参数是按照“从左至右”还是“从右至左”的顺序入栈，caller和callee必须协商好。

 3. 返回值的传递
 除了参数的传递之外，caller与callee的交互还有一个重要的方式就是返回值。callee的返回值怎么返回也是要协商好的，例如一般情况下x86下，简单的返回值一般是存储到 **eax** 寄存器中返回。但是毕竟 **eax** 容量有限，当返回值大于4字节（5 ~ 8字节）的时候，使用 **edx** 和**eax**联合返回，eax寄存器返回低4字节，edx返回高4字节。对于返回对象超过8字节的返回类型，将使用caller栈空间内存区域作为中转，在callee返回前将返回值的对象（callee栈上的临时变量，用完即销毁）拷贝到caller栈空间中。然后返回到caller中，再将临时值拷贝最终位置。

 4. 栈平衡
 callee返回时，一般会回收自己的栈空间，但是**传递的参数作为交互部分，该由caller还是callee负责修正栈指针到调用前的状态（回收参数空间，以维持栈平衡）**，双方也必须有一个约定。
关于这一方面的详细说明见[x86下的C函数调用惯例][1]，关于函数栈帧如下图所示：
![此处输入图片的描述][2]

对于moses来说，调用惯例的设计要考虑到上面4个方面。moses采用两种调用惯例，**ccc** 和 **fastcc**。

moses函数调用如下代码所示：
```
class base
{
    var start : int;
    var end : int;
};

func add( parm : base ) -> {int, int}
{
    return { parm.start, parm.end };
}

var arg : base;
arg.start = 0;
arg.start = 10;
// num 是一个解包声明，用于接收add的返回值
var num = add(arg);
```
**moses对于内置类型和匿名类型(anonymous type)采用值语义，对于用户自定义类型(class)采用引用语义**。所以对于参数传递来说只有两种类型， **内置类型(int, bool)** 以及 **匿名类型(anonymous type)** 的值以及自定义类型的地址，**内置类型(int, bool)**的值都可以使用32位值来表示，也就是说 **eax** 寄存器完全可以满足要求。对于 **匿名类型(anonymous type)** 的参数传递，可以采用**内置类型(int, bool)**的方式来实现，例如 **{int, int}**，可以看做两个int类型的参数。但是如果 **target architecture**寄存器数量不多的话，只能使用栈来进行参数的传递了。

但是对于返回值来说就比较特别了，对于**内置类型(int, bool)**和**用户自定义类型(class)**仍然可以使用 **eax** 来进行返回值的传递，但是对于**匿名类型(anonymous type)**来说就不能使用寄存器来进行返回了。对于**匿名类型(anonymous type)**的返回，可以参照 [Go][3] 的实现。

多值返回其实就是一个“语法糖衣”，C/C++中的多值返回，可以通过struct和class对象进行包装来返回。在C/C++中的实现，是在caller栈帧中创建一个临时对象作为“中转站”来实现。moses中返回值的实现参考Go的多值返回的实现。Go的做法是在传入的参数之上留了两个空位，被调者直接将返回值放在这两空位，函数f调用前其内存布局如下：
```
为ret2保留空位
为ret1保留空位
参数3
参数2
参数1 <- sp
```
如下图所示：
![此处输入图片的描述][4]

对于moses就是参照该方式实现。

**ccc** 调用惯例，就是传统的调用惯例，从右至左压参，使用栈来传递参数，使用栈来进行值的返回， caller来负责参数栈空间的回收。

**fastcc** 调用惯例，这个调用惯例尽可能地使这次调用变得更快。例如，参数和返回值都是通过寄存器来传递。


----------
## Structure Types
moses语言设计的时候，对于每种类型都有 **Type fingerprint** 的概念，moses通过type fingerprint来实现结构类型等价。
```
class type1
{
    var num : int;
    var flag : bool;
};

class type2
{
    var mem : int;
    var flag : bool;
};
// type1和type2的type fingerprint相同
// 两种类型的变量可以相互赋值
```
只要type fingerprint相同就可以相互赋值，其中有一个特例是anonymous type variable只能赋值给class type variable。Type Checking在语义分析的时候就已经完成。

因此，moses IR允许定义两种类型，"identified" 和 "literal"。其中"literal"类型可以在结构上唯一确定，也就是说在源码中出现的多处 **"{int, int}"** 都是等价的，我们也会给匿名类型命名，相同结构的匿名类型会统一指向其中一个匿名类型。虽然moses采用结构类型等价，但是"identified"类型只能通过name来唯一确定，两种class type可以等价但不同名。

匿名类型对应的"iteral"类型的声明如下：
```
%mytype = anonytype { int, int }
```

一种"identified"类型的声明如下：
```
%mytype = type { int, int }
```
**Note: 从moses设计至今，其实一直在回避一个问题，就是类型嵌套，例如：**
```
class type
{
    var num : int;
    var next : type;
};
```
**这方面涉及到后面的指针分析，我们暂时不涉及。**


----------
## Global Variables
moses中允许Global Variable无需初始化，所以moses IR沿用了这种做法。

LLVM IR中沿用了C/C++中const的概念，就是const必须初始化，然后后面不能再进行改动。如果中间对const变量进行初始化就不能算作const变量（**理由竟然是"as there is a store to the variable"**）。相反，moses IR允许const变量进行中间初始化，但是初始化后的const变量就不能在进行赋值了。这个语义限制是在语义分析的时候检查的。

但是moses IR沿用了LLVM IR中一个"trick"，就是可以将不是const的变量标成const变量，如果后面没有对其进行再赋值的话，我们就可以将该变量作为const变量进行优化。

作为SSA values, global variables会定义一个指针值（该值会dominate后面的所有scope，除非同名变量覆盖global variable），该指针值指向global variable content. 同时如果两个全局变量有相同的**"intializer"**的话，可以合并。

Syntax:

    @<GlobalVarName> = <global | constant> <Type> [<InitializerConstant>] [, align[Alignment]]

例如：
```
@G = constant int 1, align 4
```


----------
## Functions
moses IR对于函数定义需要使用 **"func"** 关键字，可选的**"calling convention"**， 返回类型（return type）， 函数名（function name），可选的"garbage collector name"（由于moses自己编写的垃圾回收模块，并无指定的必要），后面接一对花括号（{}中间包裹的是函数体）。

一个函数定义包括一组基本块，用来组成该函数的CFG(Control Flow Graph)，由于moses IR参考LLVM IR，所以也会在IR中隐含的涵盖CFG。每一个基本块以一个label开头（label可选），这个label给当前基本块定义了一个符号表（symbol table entry）项，一个基本块由一组指令组成，以一个terminator指令结尾（例如branch和function return）。如果没有给基本块定义显示的label，那么会隐式地提供一个数字label，和匿名临时变量使用的数字编号是一起的。例如，如果函数入口基本块（entry block）没有提供显示的label，那么会为其分配 **"%0" label**，那么入口 基本块中第一个匿名临时变量的编号就是 **%1**。

函数中的第一个基本块会在进入函数时立刻执行，并且也不可能有前驱基本块（predecessor basic blocks），同时由于没有前驱基本块，所以在入口基本块中肯定没有 **PHI nodes**。

Sytax:
```
func <ResultType> @<FunctionName> ([argument list]) { ... }
```


----------

## Parameter Attributes
LLVM IR比较有意思的一块是Parameter Attributes，moses IR也会抽取出共性有趣的部分。

paramter attributes是函数固有的一种属性（但是和函数类型无关），是在进行语法或者语义分析时获取到的额为信息，用来进行后续的优化。形参属性（**"parameter attributes"**）是一些关键字，如果一个形参有多个属性，中间以空格隔开。

例如：
```
func i32 @add(i32 returned, ) {}
```
当前在moses IR暂时只支持一种形参属性（后面会进行扩展）：
> * returned 
这个属性表示函数在任何情况下都会将其对应的实参作为返回。

***Note: 其实 function 也有其对应的 attribute ，例如函数是否inline，是否需要优化code size，LLVM IR作为一种通用的中间语言，也提供了code选项，表明该函数"rarely called"等等。只是moses暂时不需要这么多特性。***


----------
## Function Attributes
**"Function Attributes"**主要用于在函数上附加额外的信息。类似于"parameter attribute", "Function Attributes"属于函数的一部分，但是不属于"Function Type"的一部分。

函数属性是跟在函数形参列表后面一组关键字。如果指定多个关键字，中间以空格隔开。

例如：
```
func void @f() optsize cold {}
```
> * cold
这个属性表明该函数很少被调用。

**Note: When computing edge weights, basic blocks post-dominated by a cold function call are also cosiderd to be code; and, thus, given low weight.**
> * post-dominate
A node *z* is said to post-dominate a node *n* if all paths to the exit node of the graph starting at *n* must go through z.  -[Dominator (graph theory)][5]

上面的注意事项中是LLVM IR中一句话，就是说"cold function" post-dominate 的基本块会被赋予较低的权重。

> * minsize
这个属性建议优化 *pass* 和代码生成 *pass* 会尽力将 *code size* 降到最低，即使影响到程序运行性能。

> * norecurse
这个函数属性表明该函数在任何路径中都不会直接或间接的调用自身。

> * optsize
这个属性建议优化 *pass* 和代码生成 *pass* 尽量使该函数 *code size* 降到最低，但是在降低 *code size* 的同时不会影响程序性能。

> * readnone
这个属性表明函数只通过实参来计算返回结果，也就是或该函数在函数体中不会 *access* 到函数外的 *state*。同时不会通过指针实参（用户自定义类型实参）直写数据。

> * readonly
该属性表明该函数不会通过指针实参（用户自定义类型实参）直写数据，并且只读函数外部状态。

> * argmemonly
该属性表明函数只通过指针实参进行访存操作。


----------

## Type System
moses IR的类型系统参考LLVM IR，并结合了moses语言的特性。一个完整的类型系统允许在LLVM IR上实现很多优化，并且可读性更强。
### Void Type
void类型不表示任何值并且不占用空间。
Syntax:
```
void
```
### Function Type
函数类型相当于函数签名，但是moses不支持函数指针、函数重载，所以函数类型在moses中没有什么用途。

### First Class Type
First Class Type可以说是 moses IR 中最重要的一部分，因为 First Class Type 相对于 User Defined Type 来说更接近“底层”。

#### Single Value Types
这些类型的值在代码生成器的角度来看可以直接在寄存器中表示的，例如i32，该类型可以很容易的优化到寄存器中。

##### Integer Types
integer类型是最简单的类型，由于moses暂时只支持32位int，所以integer类型就是i32。但是使用i1type表示bool类型，关于bool类型的表示，后面我们会详细介绍道。
**syntax**:
```
i32
```

#### Structure Type
**Overview**:
structure 类型表示在内存中的一组数据的聚合，直接对应于 moses 中的 **匿名类型** 和 **用户自定义类型**。

在内存中的结构体数据需要使用 **load** 和 **store** 指令获取。另外， structure 的 element 需要通过 **getelementptr** 指令获取其地址。在寄存器中的 structure 数据需要通过 **extractvalue** 和 **insertvalue** 指令获取。

上面我们介绍过，"literal" structur对应于moses中匿名类型，可以在任意位置定义。但是用户自定义类型（class type）只能在top-level定义，并且有对应的类型名。

**Note:moses在未来可能会实现结构体重排技术，该技术会通过重排类型内部成员以占用更小的内存空间。LLVM IR中有一个packed的概念，就是类型内部没有padding，这样的话同样可以占用更少的内存空间。**

**Syntax:**
```
%mytype = type { i32, i32 }
%atype = anonytype {i32, i32}
```


----------


## Constants
moses IR对LLVM IR关于constant的部分进行了精简，包括以下几种constant。

### Simple Constants
**Boolean constants**
字符串'true'和'false'对于 i1 类型来说都是有效的常量字串。

**Integer constants**
moses关于整型只有int类型，用i32表示。

### Complex Constants
负责常量是简单常量(simple constants)的和更小的复杂常量(complex constants)的组合。

**Structure constants**
结构体常量和结构体类型的表示很相似，例如："{i32 4, i1 false}"，其实结构体常量和匿名类型的初始化表达式很相似。


----------
## Global Variable and Function Addresses
moses IR中关于全局变量和函数的地址都是有效的，因为在引用全局变量或者调用函数时都是通过两者的地址实现的。
```
@X = global i32 17
@Y = global i32 42
```


----------
## Metadata
LLVM IR中非常有趣的一部分就是提供了Metadata数据，就是自我携带一部分数据来描述自身的一些信息，例如分支语句就可以携带一个 "unpredictable" Metadata来描述自身信息。mose有可能加入这个概念。暂时不会加入这个概念。


----------
## Instruction Reference
moses IR包含几种不同的指令类型： *terminator instructions*,  *binary instrucions*, *memory instructions* 和其他指令类型。

### Terminator Instructions
上面曾经提到过，程序中的每一个基本块都以 "Terminator" 结尾，表示当前基本块结束之后应该执行哪个基本块。这些 "terminator instructions"简单地产生一个 'void' 值：这些指令产生的是**控制流(control flow)**，并不是值。

在moses中终结指令有两个：'ret', 'br'.
#### ret instructions

**ret syntax**:
```
ret <type> <value>  ; Return a value from a non-void funcion
ret void            ; Return from void function
```
'ret'指令用于将控制流（还以一个可选值）从当前函数返回到caller。有两种形式的'ret'指令，一种是返回一个 *value* 并造成控制流的转移，另一种形式就是简单的造成控制流的转移。

**'ret' Arguments**
'ret'指令会接收一个可选的单实参，一个返回值。

例如：
```
ret i32 5                       ; Return an integer value of 5
ret void                        ; Return from a void function
ret {i32, i1} {i32 10, i1 true} ; Return a struct of values 4 and true
```

#### 'br' Instruction

**br syntax**:
```
br i1 <cond>, label <iftrue>, label <iffalse>
br label <dest>         ; Unconditional branch
```

'br'指令用来在函数内部造成控制流的转移，从一个基本块转移到另一个基本块。这里有两种形式的指令，一种是条件跳转指令，一种是无条件跳转指令。

**br arguments**
条件跳转指令会接收一个 'i1' 类型的实参和两个 'label' 类型值。无条件跳转指令只接收一个 'label' 值作为跳转地址。

**br semantic**
在 'br' 跳转指令执行之前，会对 'i1' 类型的实参进行求值。如果值为真（true），控制流则会跳转到 'iflabel' 实参。如果 "cond" 为假（false），控制流则会跳转到 'iffalse' label实参。

例如：
```
Test:
%cond = icmp eq i32 %a, %b
br i1 %cond, label %IfEqual, label %IfUnequal
IfEqual:
ret i32 1
IfUnequal:
ret i32 0
```

### Binary Operations
二元运算符几乎是程序中使用得最多的运算符。Binary Operation 需要两个相同类型的操作数，执行运算操作后，得到一个单值。由于moses的类型系统还很简单，暂时只支持 *int* 类型的相加。

#### 'add' Instruction

**add syntax:**
```
<result> = add <type> <op1>, <op2>
```
两个整数相加有可能会溢出，moses暂时不会对溢出作出特出处理。

#### 'sub Instruction'

**sub syntax:**
```
<result> = sub <type> <op1>, <op2>
```

#### 'mul' Instruction

**mul syntax**:
```
<result> = mul <type> <op1>, <op2>
```

#### 'div' Instruction

**div syntax**:
```
<result> = div <type> <op1>, <op2>
```

#### 'rem' Instruction

**rem syntax**:
```
<result> = rem <type> <op1>, <op2>
```

#### 'and' Instruction

**and syntax:**
```
<result> = and <type> <op1>, <op2>
```

#### 'or' Instruction

**or Instruction:**
```
<result> = or <type> <op1>, <op2>
```

----------
### Aggregate Operations
moses IR支持几种对聚合类型操作的指令。

#### 'extractvalue' Instruction
**extractvalue syntax:**
```
<result> = extractvalue <aggregate type> <val>, <idx>{, <idx>}*
```

'extractvalue'指令能够聚合类型（聚合类型暂时只包括结构体类型）提取出 member 域。
 
 **extractvalue arguments:**
 'extractvalue'指令的第一个操作数是结构体类型，另一部分操作数是下标常量，这些下标用来表示结构体的某部分成员。'extractvalue'指令和'getelementptr'相似。

两者的区别在于：
> * 因为 'extractvalue' 指令获取的是值，所以第一个下标就省略了，默认为0
> * 至少有一个index

因为moses中的匿名类型，是不允许被动态扩展的。所以LLVM IR中对应 'insertvalue' 在moses中不存在的。

### Memory Access and Addressing Operations
一个基于 SSA-based IR 如何表示内存时很重要的一点，在LLVM中，内存不是SSA形式的，这样在中间代码生成的时候比较简单。这一部分介绍如何在moses IR中 read, write, 和 allocate 内存。

#### 'alloca' Instruction

**alloca syntax:**
```
<result> = alloca <type> [, <ty> <NumElements>]
```

'alloca'指令会在当前运行函数的栈帧上分配一块内存，并在该函数结束时自动释放（类似于C++中的RAII，但是C++的RAII要比这个强很多）。

**alloca arguments:**
'alloca'指令会从当前运行栈帧上分配 **sizeof(<type>) * NumElements** 个字节，并返回“合适类型”的指针指向内存开始。如果 **"NumElements"** 指定了，那么就会有这么多个元素的内存被分配，否则 **"NumElements"** 默认指定为1。由于moses暂时没有array类型，并且不支持指针，所以在moses中能够分配多个元素空间的场景是就是匿名类型变量了。

**alloca semantics:**
该指令会分配内存，返回对应指针。如果当前栈帧空间不足，执行该指令会产生未定义行为。'alloca'指令主要用来表示有地址的“自动变量”。当函数返回时，该空间被自动回收。

例如：
```
%ptr = alloca i32
%ptr = alloca i32, i32 4
```

#### 'load' Instruction

**load syntax:**
```
<result> = load <ty>, <ty>* <pointer>
```

注意，在moses不支持LLVM IR中的 **volatile** 和 **atomic**。'load'指令用于从内存中读数据。

**load arguments:**
'load'指令的实参用于指定从哪个位置开始装载数据。
**Note: If the load is marked as volatile, then the optimizer is not allowed to modify the number or order of execution of this load with other volatile operations. 但是moses不支持 volatile 和 atomic 选项。**

例如：
```
%ptr = alloca i32
store i32 3, i32* %ptr
%val = load i32, i32* %ptr
```

#### 'store' Instruction

**store syntax:**
```
store <ty> <value>, <ty>* <pointer>
```
'store'指令主要用于向内存写入数据。

**store arguments:**
对于 'store' 指令来说，有两个实参需要指定：一个带存储的值和一个存储地址。

**store semantics:**
'store'指令中 <pointer> 参数所对应的内存被 <value> 值刷新。

例如：
```
%ptr = alloca i32
store i32 3, i32* %ptr
%val = load i32, i32 * %ptr
```

#### 'getelementptr' Instruction

**getelementptr syntax:**
```
<result> = getelementptr <ty>, <ty*> <ptrval>{, <ty> <idx>}*
```
'getelementptr'指令用于获取聚合类型子元素的地址。该指令只会进行地址计算，并不会读取内存。

**getelementptr arguments:**
第一个实参总是基类型，第二个参数总是一个指向基地址的指针。剩下的实参就是一系列的下标，用来标识聚合类型子元素。下标的解释与其索引的类型相对应，类似于C语言指针自增的偏移由其类型决定。

LLVM IR对应示例：
```
struct RT {
    char A;
    int B[10][20];
    char c;
}；

struct ST {
    int X;
    double Y;
    struct RT Z;
};

int *foo(struct ST *s) {
    return &s[1].Z.B[5][13];
}
```
由Clang产生的LLVM code如下所示：
```
%struct.RT = type {i8, [10 x [20 x i32]], i8}
%struct.ST = type {i32, double, %struct.RT}

define i32* @foo(%struct.ST* %s) nonwind uwtable readnone optsize ssp {
entry:
    %arrayidx = getelementptr inbounds %struct.ST, %struct.ST* %s, i64 1, i32 2, i32 1, i64 5, i64 13
    ret i32* %arrayidx
}
```

上面示例中的第一个index索引的是 '%struct.ST*' 类型，得到的是 '%struct.ST' = '{i32, double, %struct.RT}' 类型，一个结构体。第二个index索引的是结构体中的第三个元素，得到的是 '%struct.RT' = '{ i8, [10 x [20 x i32]], i8}' 类型，一个新的结构体。第三个index索引的是结构体中的第二个元素，得到的是 '[10 x [20 x i32]]' 类型，一个array类型。当然也可以分别计算index，如下LLVM code所示：
```
define i32* @foo(%struct.ST* %s) {
    %t1 = getelementptr %struct.ST, %struct.ST* %s, i32 1
    %t2 = getelementptr %struct.ST, %struct.ST* %t1, i32 0, i32 2
    %t3 = getelementptr %struct.RT*, %struct.RT* %t2, i32 0, i32 1
    %t4 = getelementptr [10 x [20 x i32]], [10 x [20 x i32]]* %t3, i32 0, i32 5
    %t5 = getelementptr [20 x i32], [20 x i32]* %t4, i32 0, i32 13
    return i32* %t5
}
```
### Conversion Operations
在moses中只有一种类型转换，就是匿名类型（anonymous type）转换为用户自定义类型(class type)，所以我们在moses IR中引入一个类型转换运算符 'bitcast .. to'。

#### 'bitcast .. to' Instruction
**syntax:**
```
<result> = bitcast <ty> <value> to <ty2>
```
'bitcast'指令在不改变任何位的前提下将value转换为类型ty2。LLVM IR中对于 'bitcast' 指令要求转换的类型不能是聚合类型，这里我们将其语义改变为只用于聚合类型的转换（仅仅是匿名类型向用户自定义类型的转换）。

### Other Operations
#### 'icmp' Instruction
**icmp syntax:**
```
<result> = icmp <cond> <ty> <op1>, <op2>
```
'icmp'指令会基于两个整数的比较返回一个boolean值。

**icmp arguments:**
'icmp' 指令有三个操作数，第一个是 *condition code* 表明执行何种比较，用一系列关键字表示。 *condition code* 如下所示：
```
1. eq: equal
2. ne: not equal
3. gt: greater than
4. ge: greater or equal
5. lt: less than
6. le: less or equal
```
剩下的操作时就是两个待比较的操作数。

例如：
```
<result> = icmp eq i32 4, 5     ; yields: result = false
<result> = icmp ne i32* %x, %x  ; yields: reault = false
<result> = icmp lt i32 4, 5     ; yields: result = true
```

#### 'phi' Instruction
**phi syntax:**
```
<result> = phi <ty> [<val0>, <label0>], ...
```
'phi'指令用于在SSA图中实现φ节点。

**phi arguments:**
'phi'指令的第一个参数指定参数类型，后面跟一组实参对，每一对由一个前驱块（predecessor block）以及其对应的值组成。'phi'指令必须位于每一个基本块的头部（如果有'phi'指令的话）。

程序运行时，'phi'指令会选择某个执行的前驱基本块（每次执行仅有一个前驱块被执行）对应的值，来作为当前基本块的值。

例如：
```
Loop:       ; Infinite Loop that counts from 0 on up ...
%indvar = phi i32 [ 0, %LoopHeader ], [%nextindvar, %Loop ]
%nextindvar = add i32 %indvar, 1
br label %Loop
```

#### 'call' Instruction

**call syntax:**
```
<result> = [tail | musttail | notail ] call [ret attrs] <ty> [<fnty>*] <fnptrval>(<function args>)
```

**call arguments:**
'call'指令需要一组实参：

 - tail
 *tail* 是一个标记，类似于 C++ 中的 *inline*，建议对该函数调用执行尾调用优化（tail call optimization）。*tail* 标记只是一种提示，可以忽略。
 - musttail
 与 *tail* 不同，*musttail* 表示必须对该函数调用执行尾调用优化。
对应此次函数调用来说，*musttail* 可以保证即使该函数调用是函数调用图中的一个递归环的一部分，该函数调用也不会导致栈的无限增长。
*tail* 和 *musttail* 表明 callee 不会读取 caller 中通过 alloca 分配内存的变量。标记为 *musttail* 的调用必须能够满足如下要求：
(1) 函数调用必须紧挨着 *ret* 指令
(2) *ret* 指令只能返回前面的函数调用返回的值或者返回空
(3) caller 和 callee的类型必须能够匹配。
(4) callee 和 caller 的函数调用惯例必须相同

只要满足以下要求，标记为 *tail* 的函数调用一定能够保证尾调用优化（tail call optimization）：
(1) caller 和 callee 都使用 *fastcc* 调用惯例。
(2) 函数调用在尾部（tail position），即 *ret* 指令紧跟在函数调用后面，并且 *ret* 指令使用函数调用返回的值或者void值。

 - nontail
 *nontail* 表明禁止对该函数调用执行尾调用优化。
 - ty
 *ty* 表示该函数调用的类型，也就是函数的返回类型。
 - fnty
 函数签名。
 - fnptrval
 前面我们提到过每一个函数对应一个函数指针，函数调用的时候使用的就是该指针值。
 - function args
 实参列表，其中实参的类型需要和函数签名相匹配。

例如：
```
%retval = call i32 @test(i32 %argc)
%X = tail call i32 @foo()
%Y = tail call fastcc i32 @foo()
call void %foo(i8 97)

%struct.A = type { i32, i8 }
%r = call %struct.A @foo()
```


----------
#Intrinsic Functions
LLVM IR支持“内置函数”这一概念，这些内置有众所周知的名字和功能，相对应的moses IR也支持这一概念。其实这个概念有点儿类似于C语言中的 **sizeof()** 关键字，但是在C语言中 *sizeof* 被明确定义为关键字。

moses IR中的内置函数以 **mosesir.** 前缀开头。这个前缀是保留字，也就是用户自定义函数不能以这个前缀开头。内置函数在LLVM中定义为external函数，也就是说没有函数体，其实函数体是由编译器或者解释器来规定的。moses IR中的内置函数很少，也没有重载的必要。


----------

## Accurate Garbage Collection Intrinsics
由于moses语言需要垃圾回收的支持，所以需要一些内置函数来提供一些简单固有的操作。

这些内置函数可以用来确认栈上的 **GC roots**，以及垃圾回收器的实现需要 *read barriers* 和 *write barriers*。moses语言是类型安全的，编译器前端需要在中间代码生成的时候产生出这些内置函数。

### 'mosesir.gcroot' Intrinsic
**mosesir.gcroot syntax:**
```
func void @mosesir.gcroot(i8** %ptrloc, i8* %metadata)
```

**mosesir.gcroot arguments:**
'mosesir.gcroot' 内置函数向垃圾回收器说明了 *GC root* 的存在（注：翻译器LLVM官网，语句不通顺），并允许携带一些元数据。第一个实参确定了包含 *root pointer* 栈上对象的地址，第二个指针（一个整数常量或者一个全局值的地址）包含一些与根集相关的元数据。

在程序运行的时候，*mosesir.gcroot* 内置函数会在 'ptrloc' 的位置存储一个空指针。在编译期间，代码生成器会生成一些信息，以便让运行时系统能够在 *GC safe points* 确定根集。

### 'mosesir.gcread' Intrinsic

**mosesir.gcread syntax:**
```
func i8* @mosesir.gcread(i8* %ObjPtr, i8** %Ptr)
```

由于垃圾回收有很多种，有一种增量式（Incremental）垃圾回收就是垃圾回收过程和增变者（mutator）交替执行，增变者的执行有可能会修改前面垃圾回收器记录好的信息，所以需要记录增变者对于 *heap* 的读写。

'mosesir.gcread' 内置函数会将对 *heap* 内存的读操作信息告诉垃圾回收器。'mosesir.gcread' 内置函数和 'load' 指令有相同的语义。

### 'mosesir.gcwrite' Intrinsic
**mosesir.gcwrite syntax:**
```
func void @mosesir.gcwrite(i8* %P1, i8* %Obj, i8** %P2)
```

'mosesir.gcwrite'内置函数会识别出对于 'heap' 内存的写操作，垃圾回收器可以通过该内置函数实现 *write barriers* （像分代和引用计数）。

'mosesir.gcwrite'内置函数类似 'store' 指令的语义，但是更复杂。

## Code Generator Intrinsics
这些内置函数与代码生成相关，LLVM定义了很多这样的函数有的用于调试，有的方便执行某些操作，例如 'reurnaddress' 'memset.*'等。此处 moses IR 暂不提供这些操作。

作者 [@movie-travel-code][6]   
2016 年 05月 14日    


  [1]: http://blog.csdn.net/phunxm/article/details/8985321
  [2]: http://img.blog.csdn.net/20130529091544921
  [3]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/03.2.html
  [4]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/images/3.2.funcall.jpg
  [5]: https://en.wikipedia.org/wiki/Dominator_%28graph_theory%29
  [6]: http://weibo.com/movietravelcode