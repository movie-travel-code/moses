# Constant Folding in the AST
Clang在 AST 上提供了 **constant folding** 的能力，但不是直接修改AST，而是在代码生成的时候，直接生成 **constant folding** 过后的代码（参见[Constant Folding in the Clang AST][1]）。moses也打算执行此类优化，因为越早的执行这个优化，能够更多的揭示出其他的优化机会。**constant folding** 主要在 *expression* 上进行，通过 **expression evaluate 机制** 实现。其中 **expression evaluate** 在Clang中的另一个用途就是给 C++11（或者说是C++14）提供强有力的支持。

----------

#Constant Folding
其实在AST上做的 **constant folding** 就是一种隐式地constexpr，我们需要编译器能够向一个 ***virtual machine*** 一样生成在compile-time时对expression进行constant-folding。
Note: 关于constexpr有一篇不错的博客[Why a ‘constexpr’ is just a return statement][2]，这里面解释为什么C++11规定constexpr修饰的函数只能是return statement.

大体意思就是Compiler虽然在做某些优化时，可以认为是一个virtual machine，但不是一个完全的virtual machine，否则会本末倒置。compiler的基本需求还是对源码进行翻译。

我曾经对Clang极力进行constant-folding表示不解，为了区区一点儿效率或者code size，花费很大代价去做constant-folding是否值得。
> In order to produce efficient runtime code it is often necessary to evaluate the code. Some common optimizations are const-folding and inlining. An optimizer will trace through expressions involving constants and replace them with the resulting value. Beyound just saving some calculation, it allows the optimizer to do things like remove constant branches, perform loop unrolling, and further inline code.


----------
#moses的Constant Folding
moses暂时提供的constant folding非常简陋，只支持很有限的几种folding机制。

具体的如下代码是所示：
```
virtual bool EvalBinaryExpr(const BinaryExpr* B, EvalInfo& Info,
        EvalInfo &lhs, EvalInfo &rhs) = 0;
        
virtual bool EvalUnaryExpr(const UnaryExpr* U, EvalInfo &info, 
        EvalInfo &subVal) = 0;
        
virtual bool EvalCallExpr(const CallExpr* CE, EvalInfo &info) = 0;

virtual bool EvalDeclRefExpr(const DeclRefExpr* DRE, EvalInfo &info);
```
对于eval的类型也有严格的限制，现在只支持 int 和 bool 内置类型，另外对于EvalCall也有层数的限制，EvalCall的深度最大为1. 并且 **EvalCallExpr()** 对于 CallExpr 也有要求，只能含有一条return stmt（类似于C++11的constexpr）。当然后期我们会慢慢的对constant-evaluator进行加强，我们也预留了相关接口。例如对于EvalCall来说，有如下成员：
```
// 在对 CallExpr 进行eval的时候，需要对实参进行推导，所以在对函数体进行eval的
			// 时候，需要记录实参的值（就像真正的函数调用一样）
			// 例如：
			// const length = 20;
			//	func add(parm1 : int, parm2 : int) -> int 
			//	{
			//		return parm1 + parm2 * length;
			//	}
			//	
			//	const start = 10;
			//	const end = 11;
			//	var num = add(start, end);
			//
			//	level0:		对实参进行eval，得到value_list如下，
			//			value_list <start, 10> <end, 11> 
			//
			//	level1:		return parm1 + parm2 * length;
			//
			//	在对return语句进行eval的时候，需要对expression表达式中的变量进行eval，所以就需要记录到以前
			//	eval得到的结果。
			// Note: 现在我们对CallExpr只能eval一层，以后也有可能eval多层
			//		0：				add(start, end)				记录第0层的实参值
			//							   ||
			//							   ||
			//							   \/
			//		1：		return parm1 + parm2 * length;		有可能需要记录第1层的实参值
			//						//				\\
			//					   //				 \\
			//
			//		2：	有可能有第三层，所以需要					有可能需要记录第2层的实参值

			// To Do: 使用栈来记录实参值（模拟真实程序运行）
			// 例如： 动态扩展的active stack，每一个栈帧都会存储实参值对
			//	$$<start, 11> <end, 10> $$ <flag, false> $$ <>
			//  <level0, 2> <level2, 1> 
			// Note: moses对CallExpr的推导暂时只有一层。

			// （1） 该vector存储active stack info.
			std::vector<std::pair<std::string, EvalInfo>> ActiveStack;

			// （2） 该vector存储stack bookkeeping info，level以及每层有多少个实参值.
			std::vector<std::pair<int, unsigned>> ActiveBookingInfo;
```
虽然moses对于ZEvalCall只有一层，但是我们未来打算对其进行扩展。

  [1]: http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
  [2]: https://mortoray.com/2013/06/22/why-a-constexpr-is-just-a-return-statement/