# moses
Moses是一门很简单的编程语言，参考了swift，但是比swift更简单。

----------

## 变量声明
变量声明支持有类型的声明和无类型的声明。如下所示：

```
var num : int;
var num = 10;
```
以 **var** 关键字开头表示变量声明，后面可以跟 **:** 和类型名来显示指定变量类型，同时也使用表达式进行初始化，moses会通过初始化表达式推断出相应的类型。moses暂时支4种类型，**int**、**bool** 、**user defined type**、**anonymous type**。其中 **user defined type** 表示用户自定义类型，类似于C中的struct，而 **anonymous type** 表示通过匿名类型表达式（由'{' '}'指定）推断出来的匿名类型。

对于匿名类型变量的初始化，如下代码所示：

```
var flag = false;
var size = 500;
var num = {10, {!false, size}}; // Anonymous type.
```
第三行的 **{10, {!false, size}}** 表示匿名类型表达式，moses会根据该表达式推导出一个匿名类型附加在变量 **num** 上，其中num推导出的匿名类型如下所示：

```
class 
{
    var : int;
    {
    	var : bool;
		var : int;
	}
}
```

为了语义的完整性，我们也可以在变量声明时使用匿名类型，同时对匿名类型的变量可以进行解包操作。如下所示：

```
var num : {int, {int, bool}}; 
num = {0, {0, true}};
var num = {10, {!false, true}};
var {a, {b, c}} = num; // 进行解包操作之后，a = 10, b = true, c = true
```
解包操作有两种实现方式，第一种就是通过解包声明（unpack declaration）来进行解包，第二种就是通过用户自定义的类型变量来进行解包。解包声明的变量类型是const的，并且作用于就在当前的scope中,匿名类型变量只作为值的传递方式出现(有点儿类似于C++中的临时变量）。只能够对匿名类型变量和返回类型为匿名类型的函数调用表达式进行解包。

```
// (1) 通过用户自定义类型来进行解包(注： user defined type不能向anonymous type转换)
class base
{
    var start : int;
    var end : int;
};

var num = {0, 1};
base = num;

// (2) 通过解包声明来进行解包
var num = {0, 1};
var {start, end} = num;

```

类似于swift，moses支持const变量，

```
// 在moses中const类型不再需要必须初始化
// 从这个角度上来看，moses中的const更类似于C#的readonly关键字
const num : int; 
num = 10;
```
----------

## 类型
moses内置类型暂时只有 **int** 和 **bool**，其中 **int** 是32位。关于用户自定义类型（也就是class），类似于C语言中的struct，默认数据成员都是public的。class的设计还很简陋，相当于类型的聚合，暂时不提供继承，访问控制等特性。

moses采用结构类型等价（structural type equivalence），不像C/C++或者Java采用名称等价，结构类型等价是和匿名类型相辅相成的。
例如：

```
class base
{
	var start : int;
	var end : int;
	var flag : bool;
};
var b : base;
var anony = {0, 0, true};
b = anony; // 由于结构类型等价的存在，这样做在moses中是合法的
```
上面提到moses暂时支持4种类型，**int**、**bool**、**user defined tpye**、**anonymous type**。从示例代码中我们可以看出moses支持 **anonymous type** 向 **user defined type** 的转换。


----------
##函数

函数定义如下：
```
func add(lhs : int, rhs : int) -> int  
{  
	return lhs + rhs;  
}  
```
以 **func** 关键字开头，表示函数定义，并在参数列表后面由 **->** 指明返回类型。关于形参列表，moses也支持const形参。

```
func add(const lhs : int, const rhs : int) -> int
{
	return lhs + rhs;
}
```
为了与匿名类型兼容，function进行传参以及值返回时，支持匿名类型。匿名类型设计的初衷要比class简单，因为匿名类型推导时，无法获知class后面可以由用户添加的类似于 **public**（暂时还不支持） 的访问控制信息，如下代码所示：

```
var num = {0, 0, {true, 0}};
func add(lhs : {int, int, {bool, int}}, rhs : int) -> {int, int}
{
	// 匿名类型的形参需要进行解包操作
	var {start, end, {flag, num}} = lhs;
	return {start, end};
}
```
函数支持匿名类型传参以及匿名类型的变量返回。

----------

##值语义与引用语义
moses仿照 java 中的设计，内置类型采用值语义，而用户自定义类型默认采用引用语义。moses不存在指针和引用，为了支持用户自定义类型默认引用语义，moses需要实现垃圾回收机制。

```
// 形参lhs默认采用的是引用语义，函数内部对其的修改会真切的应用到实参上
// 所以moses支持const 形参
func add(parm : base) -> int
{
	return base.num;
}

// 形参lhs是const类型
func sub(const parm : base) -> int 
{}
```
但是匿名类型形参在函数内部需要进行解包操作，这个操作就是一种值语义，需要将形参（所对应的实参）的值一一解包拷贝到函数的局部变量上。

##函数返回多值
由于匿名类型的存在，我们可以通过匿名类型的机制来返回多值。

```
func add() -> {int, int}
{
	return {0, 0};
}
var num = add();
// 匿名类型的返回可以由变量，但是变量没办法访问其内部的数据成员
// 所以即使使用变量来接受匿名类型值的返回，但是仍然需要进行解包
var {a, b} = num;
// 当然最好是一步到位，直接对返回结果进行解包
var {start, end} = add();
```
##控制结构
moses暂时支持两种控制结构，if-else和while-loop。

```
if lhs < rhs
{
	//...
}

while flag
{
	// ...
}
```
条件表达式不需要使用 "(" 与")" 进行包裹。

## 示例
moses，如下代码所示：
```
var number = 100;
var sum : int;

while(number > 0)
{
	sum += number--;
}

var result : bool;
if (sum > 100000)
{
	result = true;
	while(sum > 0)
	{
		sum--;
	}
}
else
{
	result = false;
}
```
生成的近SSA的IR如下，现在的IR很原始，有很多冗余而且没有任何优化可言。后面会将该IR提升到完全SSA形式，然后在其上应用相关优化算法。
```
 entry:
%number.addr = alloca int        ; < int* >
%sum.addr = alloca int        ; < int* >
%result.addr = alloca bool        ; < bool* >
store int 100.000000, int* %number.addr        ; < void >
br label %while.cond0

 %while.cond0:
%3 = load int* %number.addr        ; < int >
%gt.result4 = cmp gt int %3, int 0.000000        ; <  bool > 
br bool %gt.result4, label %while.body2, label %while.end1

 %while.body2:
%5 = load int* %number.addr        ; < int >
%dec6 = add int %5, int -1        ; < int >
store int %dec6, int* %number.addr        ; < void >
%7 = load int* %sum.addr        ; < int >
%add.tmp8 = add int %7, int %5        ; < int >
store int %add.tmp8, int* %sum.addr        ; < void >
%9 = load int* %sum.addr        ; < int >
br label %while.cond0

 %while.end1:
%13 = load int* %sum.addr        ; < int >
%gt.result14 = cmp gt int %13, int 100000.000000        ; <  bool > 
br bool %gt.result14, label %if.then10, label %if.else12

 %if.then10:
store bool , bool* %result.addr        ; < void >
%15 = load bool* %result.addr        ; < bool >
br label %while.cond16

 %while.cond16:
%19 = load int* %sum.addr        ; < int >
%gt.result20 = cmp gt int %19, int 0.000000        ; <  bool > 
br bool %gt.result20, label %while.body18, label %while.end17

 %while.body18:
%21 = load int* %sum.addr        ; < int >
%dec22 = add int %21, int -1        ; < int >
store int %dec22, int* %sum.addr        ; < void >
br label %while.cond16

 %while.end17:
br label %if.end11

 %if.else12:
store bool , bool* %result.addr        ; < void >
%23 = load bool* %result.addr        ; < bool >
br label %if.end11

 %if.end11:
```