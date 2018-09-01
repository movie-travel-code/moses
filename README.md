## What is moses for?
moses is a simple project just for improving my compiler skills, including 

 - 1.How to write a parser
 - 2.How to apply semantic analysis
 - 3.Code generation
 - 4.Static program analysis 
   - Optimization
   - Symbolic execution
   - Pointer analysis
   - Type analysis
   - Or any static analysis skills
 - 5.How to write interpreter
 - 6.How to write a linker, debugger etc
 - 7. Garbage collection

But so far, I just finished `1`, `2`, `3`, `5` in 2016.06. Current buggy implementation is very simple and have many places borrowed from llvm2.6, like the IR design.

Another rule of moses is to improve my English writing skills :).

*Note: The reason why it was named [moses][1] is that I am learning religious history when I am implementing this. Yea, this name is farfetched.*

## How to build

```Bash
$ mkdir build
$ cd build
$ cmake ..
$ sudo make
```

## How to use
Although moses's functionality is not perfect, we can still achieve our goals by writing some simple programs, such as calculating factorials. There is a sample program for calculating factorials, see `test/Factorial.mo`, the implementation is as follows:

```Swift
func fac(parm:int) -> int {
  var result = 1;
  while(parm > 1) {
    result *= parm--;
  }
  return result; 
}

var result = fac(10);
print(result);
```

We can use the command `$ ./moses ../test/Factorial.mo` to compute the factotial. In order to facilitate observation and debugging, we will print the tokens and IR by default. The ouput is as follows. As you can see, the `10!` is 3628800.

```C
func
fac
(
parm
:
int
)
->
int
{
var
result
=
1
;
while
(
parm
>
1
)
{
result
*=
parm
--
;
}
return
result
;
}
var
result
=
fac
(
10
)
;
print
(
result
)
;
FILE_EOF
Parser done!
--------------------------------------------------------------------------------
 entry:
%result.addr = alloca int        ; < int* >
%0 = call int fac( int 10.000000)        ; < int>
store int %0, int* %result.addr        ; < void >
%1 = load int* %result.addr        ; < int >
call mosesir.print( int %1)        ;

define int fac( int %parm.addr)
{
 entry:
%retval = alloca int        ; < int* >
%0 = alloca int        ; < int* >
%result.addr = alloca int        ; < int* >
store int %parm.addr, int* %0        ; < void >
store int 1.000000, int* %result.addr        ; < void >
br label %while.cond1
 %while.cond1:
%4 = load int* %0        ; < int >
%gt.result5 = cmp gt int %4, int 1.000000        ; <  bool >
br bool %gt.result5, label %while.body3, label %while.end2
 %while.body3:
%6 = load int* %0        ; < int >
%dec7 = add int %6, int -1        ; < int >
store int %dec7, int* %0        ; < void >
%8 = load int* %result.addr        ; < int >
%mul.tmp9 = mul int %8, int %6        ; < int >
store int %mul.tmp9, int* %result.addr        ; < void >
%10 = load int* %result.addr        ; < int >
br label %while.cond1
 %while.end2:
%11 = load int* %result.addr        ; < int >
store int %11, int* %retval        ; < void >
%12 = load int* %retval        ; < int >
ret int %12
}

--------------------------------------------------------------------------------
IDom(entry): entry
--------------------------------------------------------------------------------
3628800
```

## Internal implementation

Visit the wiki for the internal implementation details.

----------
## What is moses?
`moses` is a simple language, reference to swift, but simpler. 

### Variable declaration
moses support the type declarations and no-type declarations. Like,

```Swift
var num : int;
var num = 10;
```

`moses` use the keyword `var` to start the variable declaration, we can explicitly specify the type of the variable by following with `:`. In addition, we can also use expression to initialize the variable and `moses` will infer the variable type by the initializer.

`moses` support four types temporarily, **int**, **bool**, **user defined type**, **anonymous type**. The **user defined type** is similar to `struct` in `C`. The **anonymous type** represent the types infered by the anonymous expression.

Given the code below, the `{10, {!false, size}}` represent the anonymous expression, `moses` will infer the anonymous type through this anonymous expression. 

```Swift
var flag = false;
var size = 500;
var num = {10, {!false, size}}; // Anonymous type.
```

The anonymous type of the variable `num` is shown below:

```Swift
class 
{
  var : int;
  {
    var : bool;
    var : int;
  }
}
```

For the completeness of the language semantic, we can also use anonymous type in the variable declaration, and do the unpack operations for the variable with anonymous type. Given the code below, we can unpack `num` into three variables.

```Swift
var num : {int, {int, bool}}; 
num = {0, {0, true}};
var num = {10, {!false, true}};
var {a, {b, c}} = num; // After the unpack operation, a = 10, b = true, c = true
```

There are two ways use the unpack operations:
 - Use the `unpack declaration` to trigger the unpack operations
 - Use the `user defined type` to trigger the unpack operations

The variable use `unpack declaration` is const qualified, and can only passed by value. We can use `unpack operation` for the variable declaration and for the call expression whose type is anonymous type, e.g the call for the functions whose return type is anonymous.

```Swift

// (1) Use the `unpack declaration` to trigger the unpack operation.
var num = {0, 1};
var {start, end} = num;

// (2) Use the `user defined type` to trigger the `unpack operation`
// Note: `user defined type` can't convert to the `anonymous type`
class base
{
  var start : int;
  var end : int;
};

var num = {0, 1};
base b = num;
```

Like `swift`, moses support the const semantic too.

```Swift
// In `moses`, `const` variable do not necessarily have to be intialized.
const num : int; 
num = 10; // `num` cannot reassigned
```

### Type
`moses` only have two builtin types, **int** and **bool**, the length of **int** is 32bits. For the `user defined type`, e.g. `class`, it's similar to the `struct` in `C`, the default access is `public`. However, `moses` doesn't support inheritance and access control for now.

`moses` adopts structural type equivalence, not like C/C++ or Java which adopt nominal type system, anonymous type and structural type system complement each other.

```Swift
class base
{
  var start : int;
  var end : int;
  var flag : bool;
};
var b : base;
var anony = {0, 0, true};
b = anony; // Since the structural type equivalence, it is legal in `moses`
```

From the sample code, we can see **anonymous type** and **user defined type**
can converted to each other.

### Function

The function definition is as follows.
```Swift
func add(lhs : int, rhs : int) -> int  
{  
	return lhs + rhs;  
}  
```

The function definition is started with `func`, and specify the return type by following the parameter list with `->`. The parameter can also be const qualified.

```Swift
func add(const lhs : int, const rhs : int) -> int
{
	return lhs + rhs;
}
```

In order to be compatible with anonymous type, we can also use anonymous type when passing parameters and returning values.

```Swift
var num = {0, 0, {true, 0}};
func add(lhs : {int, int, {bool, int}}, rhs : int) -> {int, int}
{
  var {start, end, {flag, num}} = lhs;
  return {start, end};
}
```

In addition, `moses` provide a simple builtin function `print`.

### Value semantic and reference semantic

`moses` imitates `JAVA`, the builtin type use `value semantic` and user defined type use `reference semantic`. `pointer` and `reference` are not supported for the time being. In order to support `reference semantic`, maybe we need to implement the garbage collection.

``` Swift

// Since user defined type use `reference semantic`, the modification of `parm` will reflected on the arguments.
func add(parm : base) -> int
{
  return base.num;
}

func sub(const parm : base) -> int {}
```

For the parameter with anonymous type, we have to unpack the values into the local variables. When we unpack the parameter's value, `moses` use value semantic.

### Return Multiple Values.

Since `moses` support anonymous types, we can return multiple values through anonymous types. In `C++`, we can use `user defined types` to return multiple values. In `moses`, we do not need to explicitly define `user defined type` for the multiple values return.

```Swift
func add() -> {int, int}
{
  return {0, 0};
}
var num = add();

// For anonymous type, since we cannot visit the field declaration, we have to
// unpack the anonymous types which used for the multiple values return.
var {a, b} = num;

// Of course, the best manner is unpack the return value directly.
var {start, end} = add();
```
### Control statements

`moses` support two control statements, `if else` and `while` loop.

```Swift
// Conditional expression does not need `(` and `)`.
if lhs < rhs
{
  //...
}

while flag
{
  // ...
}
```

### Sample code

```Swift
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

The IR for SSA form is shown below, the IR is premitive, with lots of redundancy and no optimization. In the future, I will improve the IR and apply the optimization algorithms.

```C
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

[1]: https://en.wikipedia.org/wiki/Moses