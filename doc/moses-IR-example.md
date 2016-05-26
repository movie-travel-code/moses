# Moses与Moses IR示例
---
## 函数调用
moses的函数调用如下代码：
```
func add(parm : int) -> int
{
    return parm;
}
add(10);
```
对应的moses IR如下所示：
```
define i32 @func(i32 %parm) 
{
    1% = alloca i32
    store i32 %parm, i32* %1
    %2 = load i32* %1
    ret i32 %2
}
%1 = call i32 @func(i32 10)
```


----------
## if-else
moses中的if-else源码示例：
```
var num = 0;
if (num == 0)
{
    num = 1;
}
else
{
    num = 0;
}
```
对应的moses IR代码如下所示：
```
%num = alloca i32
store i32 0, i32* %num
%1 = load i32* %num
2% = cmp eq i32 %2, 0
br i1 %2, label %3, label %4

; <label>:3
store i32 1, i32* %num
br label%5

; <label>:4
store i32 9, i32* %num
br label %5

; <label>:5
```
从上面moses IR代码中可以看到，label也是会参与到名字的编码的，只是不会在moses IR显示的写上label名（是以注释的形式展现的）。


----------
## while-loop
moses中while-loop示例代码如下所示：
```
func add(parm : int) -> int
{
    var sum = 0;
    while parm > 0
    {
        sum += parm--;
    }
    return sum;
}
```
对应的moses IR的代码如下所示：
```
define i32 @add(i32 %parm)
{
    %1 = alloca i32
    %sum = alloca i32
    store i32 %parm, i32* %1
    store i32 0, i32* %sum
    br label %2
    
; <label>:2
    %3 = load i32* %1
    %4 = cmp gt i32 %3, 0
    br i1 %4, label %5, label %10

; <label>:5
    %6 = load i32* %1
    %7 = add i32 %6, -1
    store i32 %7, i32* %1
    %8 = load i32* %sum
    %9 = add i32 %8, %6
    store i32 %9, i32* %sum
    br label %2

; <label>:10
    %11 = load i32* %sum
    ret i32 %11
}
```
从上面也可以看出moses IR基本上是RISC的架构。


----------
## Class Type
在moses中，class的定义如下所示：
```
class Point
{
    var start : int;
    var end : int;
};

Point p;
p.start = 0;
p.end = 1;
```
对应的moses IR代码如下所示：
```
%Point = type {i32, i32}

%p = alloca %Point
%1 = getelementptr &Point* %p, i32 0, i32 0
store i32 0, i32* %1
%2 = getelementptr %Point* %p, i32 0, i32 1
store i32 1, i32* %2
```
class Type中有嵌套class type的情况如下代码所示：
```
class XY
{
    var start : int;
    var end : int;
};
class Point
{
    var XY : xy;
    var height : int;
};
var p : Point;
p.xy.start = 0;
p.xy.end = 1;
n.height = 10;
```
对应的moses IR如下代码所示：
```
%Point = type { %XY, i32}
%XY = type { i32, i32 }

%n = alloca %Point
%1 = getelementptr %Point* %n, i32 0, i32 0
%2 = getelementptr %XY* %1, i32 0, i32 0
store i32 0, i32* %2
%3 = getelementptr %Point* %n, i32 0, i32 0
%4 = getelementptr %XY* %3, i32 0, i32 1
store i32 1, i32* %4
%5 = getelementptr %Point* %n, i32 0, i32 1
store i32 10, i32* %5
```
在 moses IR 中用户自定义类型类似于 moses 中的匿名类型，从这里我们也可以看到其实只要结构上等价，都是可以相互赋值的（注意：目前moses不支持访问控制，如果存在访问权限的话，需要考虑访问权限）。另外，**bool** 变量在moses IR中是通过 **i1** 表示的。


----------
## Anonymous-Type
在moses中Anonymoust-type的示例代码如下所示：
```
// 声明一个Anonymous-type的变量，通过类型推导给num附加上一个确切的Anonymous-type
var num = {12, false};
// 使用{length, flag}将num中的值解包出来
var {length, flag} = num;
```
在moses中，解包声明的变量都是const类型的。对应的moses IR代码如下所示：
```
%num = alloca {i32, i1}
%start = alloca constant i32
%end = alloca constant i32

;这里有两种方式：
;(1) 使用 getelementptr + store
;(2) 使用 load + insertvalue + store
;这里采用的是第一种
;第二种的方式是直接将 %num 从内存 load到临时值上，修改临时值，然后再装载回去

%1 = getelementptr {i32, i1}* %num, i32 0, i32 0
store i32 12, i32* %1
%2 = getelementptr {i32, i1}* %num, i32 0, i32 1
store i32 0, i32* %2

;这里有两种方式：
;（1）使用getelementptr + load + store
;（2）使用load + extractvalue + store
; 使用C语言模拟的时候，采用的是第一种。第一种其实是比较高效的
; 因为load操作是将整个结构体load到一个临时值中，然后使用extractvalue从临时值
; 提取出指定数据，然后再store回到内存。存在多余的数据load
%3 = getelementptr {i32, i1}* %num, i32 0, i32 0
%4 = load i32* %3
store i32 %4, i32* %start

%5 = getelementptr {i32, i1}* %num, i32 0, i32 1
%6 = load i32* %5
store i32 %6, i32* %end
```


----------
## 函数返回
在讨论moses IR的函数返回时，需要探讨一下LLVM IR中对于函数返回值的设计。

### LLVM函数返回（返回值大不大于8字节）
下面是一段C语言的代码。
```
struct Point
{
    int start;
    int end;
};

struct Point add()
{
    struct Point p;
    p.start = 0;
    p.end = 1;
    return p;
}
```
从上面的源码中我们可以看到返回值p的大小为两个 *int* 的大小。使用 *clang* 进行编译得到的LLVM IR如下所示：
```
%struct.Point = type { i32, i32 }

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture, i8* nocapture readonly, i32, i32, i1) #1

; Function Attrs: nounwind
define i64 @add() #0 
{
    %1 = alloca %struct.Point, align 4
    %p = alloca %struct.Point, align 4
    
    %2 = getelementptr inbounds %struct.Point* %p, i32 0, i32 0
    store i32 0, i32* %2, align 4
    
    %3 = getelementptr inbounds %struct.Point* %p, i32 0, i32 1
    store i32 1, i32* %3, align 4
    
    %4 = bitcast %struct.Point* %1 to i8*
    %5 = bitcast %struct.Point* %p to i8*
    
    call void @llvm.memcpy.pi08.pi08.i32(i8* %4, i8* %5, i32 8, i32 4, i1 false)
    
    %6 = bitcast %struct.Point* %1 to i64*
    %7 = load i64* %6, align 1
    ret i64 %7
}
```
LLVM函数返回中只能返回单值（临时值），即使是 *struct* 只要不大于8字节，都可以转换成 *i64* 的单值，然后返回。LLVM不支持在原始内存上进行这样的指针转换（原因未知，原始内存不可能再被用到啊？），所以需要在 *stack* 另创建一个新的内存空间，然后将原始数据拷贝过去。在新内存上进行指针转换，转换到i64指针，然后 *load* 到临时值中，最终返回。

### LLVM函数返回（大于8字节，非优化版）
在LLVM返回值大于8字节的时候，会在 *caller stack frame* 中分配一块临时内存，将该内存地址作为参数传递给 *callee*，然后再 *callee* 中将待返回的值使用内置函数 *llvm.memcpy()* 拷贝到临时内存中，然后返回void。示例代码如下所示。
```
struct XY
{
    int start;
    int mid;
    int end;
};

struct XY func()
{
    struct XY xy;
    xy.start = 0;
    xy.mid = 1;
    xy.end = 2;
    return xy;
}
```
对应的LLVM IR如下所示。
```
%struct.XY = type {i32, i32, i32}

; Function Attrs: nounwind
define void @func(%struct.XY* noalias sret %agg.result) #0
{
    %xy = alloca %struct.XY, align 4
    
    %1 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 0
    store i32 0, i32* %1, align 4
    
    %2 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 1
    store i32 1, i32* %2, align 4
    
    %3 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 2
    store i32 2, i32* %3, align 4
    
    %4 = bitcast %struct.XY* %agg.result to i8*
    %5 = bitcast %struct.XY* %xy to i8*
    
    call void @llvm.memcpy.pi08.pi08.i32(i8* %4, i8* %5, i32 12, i32 4, i1 false)
    ret void
}
```
从LLVM IR中可以明显看到，修改了函数 *signature* ，在 *callee* 内部将待返回的值拷贝到 *caller* 的临时内存中。

### LLVM函数返回（大于8字节，优化版）
这里我们使用C++代码写的源码如下所示：
```
class A
{
public:
    int mem_1;
    int mem_2;
    int mem_3;
}

A func()
{
    A a;
    a.mem_1 = 10;
    a.mem_2 = 10;
    a.mem_3 = 10;
    return a;
}

int main()
{
    A a = func();
    return 0;
}
```
对应的moses IR优化版本如下：
```
%class.A = type { i32, i32, i32 }

; Function Attrs: nounwind
define void @"\01?func@@YA?AVA@@XZ"(%class.A* noalias sret %agg.result) #0
{
    %1 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 0
    store i32 10, i32* %1, align 4
    
    %2 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 1
    store i32 10, i32* %2, align 4
    
    %3 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 2
    store i32 10, i32* %3, align 4
    ret void
}

; Function Attrs: nounwind
define i32 @main() #0
{
    %1 = alloca i32, align 4
    %a = alloca %class.A, align 4
    store i32 0, i32* %1
    call void @"\01?func@@YA?AVA@@XZ"(%class.A* sret %a)
    ret i32 0
}
```
在 *main()* 函数中调用 *func()* 函数，由于返回值不能用传统的 *eax* 和 *ebx* 寄存器来传递，所以在 *caller stack frame* 中创建了临时内存，只是这块临时内存就是 *caller* 中被函数 *func()* 赋值的变量。其实一段代码在经过Clang编译到LLVM IR之后，函数的语义其实已经被相应的更改了，例如这里函数 *func()* 的 *signature* 已经被改变了。
### moses IR函数返回
其实[LLVM 2.3][1]就已经支持多值返回（multiple return value, MRV）了，具体细节参见这篇[日记][2]。

moses借助匿名类型来返回多值，示例代码如下图所示：
```
func add(parm1 : int, parm2 : int) -> {int, int}
{
    return {parm1, parm2};
}
```
moses IR对于多值的返回，借鉴LLVM的实现，在小于等于8字节的情况下可以拼成一个i64大小的值，修改函数 *signature* 为i64大小。在大于8字节时，在 *caller stack frame* 中创建一块临时内存区域，然后将值拷贝到该区域中。 
moses IR对于返回多值的支持如下所示：
```
define {i32, i32} @add(i32 %parm1, i32 %parm2)
{
    %1 = load i32* %parm1
    %2 = load i32* %parm2
    ；类似于 LLVM 将两个变量
    ret { i32, i32 } { %1, %2 }
}
```
在IR中不可能直接将内存中的数据值进行传递，必须 *load* 到临时变量，使用临时变量进行值的传递。在这里我们将返回值按照Go语言多值返回的机制实现，如下图所示：

![此处输入图片的描述][3]

moses关于多值返回（MRV）的源码如下所示：
```
func add(parm1 : int, pamr2 : int) -> {int, int}
{
    return {parm1, parm2};
}
var {start, end} = add(10, 10);
```

对应的moses IR实现如下代码所示：
```
; 修改function signature
define void @add(i32 %parm1, i32 %parm2, i32* %ret1, i32* %ret2)
{
    %1 = load i32* %parm1
    %2 = load i32* %parm2
    store %1 i32* %ret1
    store %2 i32* %ret2 
    ret void
}
; 在参数上方开辟好一段临时内存，然后修改 function signature ，将临时内存的地址传递给 add() 函数

; 为start, end分配栈上内存
%start = alloca i32
%end = alloca i32

; 为临时变量分配栈上内存
%ret1 = alloca i32
%ret2 = alloca i32

; 修改function signature
call void add(i32 10, i32 10, %32* ret1, %i32* ret2)

; 此时返回值已经放到 ret1 和 ret2位置上了
; 下面需要将数据拷贝到 %start %end上
%1 = load i32* %ret1
%2 = load i32* 
```
注意上面的代码在真正的target-code generation的时候，可能会按照返回值的大小，将其优化到register中。

关于结构体类型的返回类似于多值返回，也是在 *caller stack frame* 中创建一块临时内存，借助这块临时内存作为中转（同样如果返回值也可以优化到寄存器中）。其实这个要必须做好保证，因为以后moses支持多 *translate unit* 的时候需要在 *caller* 和 *callee* 两端做好约定的（calling convention）。

在 LLVM 中有 *First Class Value* 的概念，同理在moses IR也有这种概念。属于 *First Class Value* 的类型如下：
> * Integer
> * Structure

暂时只有这两种值可以作为函数返回值。这种分类参考的是 [LLVM 2.3][4]。


  [1]: http://llvm.org/releases/2.3/docs/ReleaseNotes.html
  [2]: http://nondot.org/sabre/LLVMNotes/MultipleReturnValues.txt
  [3]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/images/3.2.funcall.jpg
  [4]: http://nondot.org/sabre/LLVMNotes/FirstClassAggregates.txt