# Constant Folding in the AST
Clang�� AST ���ṩ�� **constant folding** ��������������ֱ���޸�AST�������ڴ������ɵ�ʱ��ֱ������ **constant folding** ����Ĵ��루�μ�[Constant Folding in the Clang AST][1]����mosesҲ����ִ�д����Ż�����ΪԽ���ִ������Ż����ܹ�����Ľ�ʾ���������Ż����ᡣ**constant folding** ��Ҫ�� *expression* �Ͻ��У�ͨ�� **expression evaluate ����** ʵ�֡����� **expression evaluate** ��Clang�е���һ����;���Ǹ� C++11������˵��C++14���ṩǿ������֧�֡�

----------

#Constant Folding
��ʵ��AST������ **constant folding** ����һ����ʽ��constexpr��������Ҫ�������ܹ���һ�� ***virtual machine*** һ��������compile-timeʱ��expression����constant-folding��
Note: ����constexpr��һƪ����Ĳ���[Why a ��constexpr�� is just a return statement][2]�����������ΪʲôC++11�涨constexpr���εĺ���ֻ����return statement.

������˼����Compiler��Ȼ����ĳЩ�Ż�ʱ��������Ϊ��һ��virtual machine��������һ����ȫ��virtual machine������᱾ĩ���á�compiler�Ļ��������Ƕ�Դ����з��롣

��������Clang��������constant-folding��ʾ���⣬Ϊ������һ���Ч�ʻ���code size�����Ѻܴ����ȥ��constant-folding�Ƿ�ֵ�á�
> In order to produce efficient runtime code it is often necessary to evaluate the code. Some common optimizations are const-folding and inlining. An optimizer will trace through expressions involving constants and replace them with the resulting value. Beyound just saving some calculation, it allows the optimizer to do things like remove constant branches, perform loop unrolling, and further inline code.


----------
#moses��Constant Folding
moses��ʱ�ṩ��constant folding�ǳ���ª��ֻ֧�ֺ����޵ļ���folding���ơ�

��������´�������ʾ��
```
virtual bool EvalBinaryExpr(const BinaryExpr* B, EvalInfo& Info,
        EvalInfo &lhs, EvalInfo &rhs) = 0;
        
virtual bool EvalUnaryExpr(const UnaryExpr* U, EvalInfo &info, 
        EvalInfo &subVal) = 0;
        
virtual bool EvalCallExpr(const CallExpr* CE, EvalInfo &info) = 0;

virtual bool EvalDeclRefExpr(const DeclRefExpr* DRE, EvalInfo &info);
```
����eval������Ҳ���ϸ�����ƣ�����ֻ֧�� int �� bool �������ͣ��������EvalCallҲ�в��������ƣ�EvalCall��������Ϊ1. ���� **EvalCallExpr()** ���� CallExpr Ҳ��Ҫ��ֻ�ܺ���һ��return stmt��������C++11��constexpr������Ȼ�������ǻ������Ķ�constant-evaluator���м�ǿ������ҲԤ������ؽӿڡ��������EvalCall��˵�������³�Ա��
```
// �ڶ� CallExpr ����eval��ʱ����Ҫ��ʵ�ν����Ƶ��������ڶԺ��������eval��
			// ʱ����Ҫ��¼ʵ�ε�ֵ�����������ĺ�������һ����
			// ���磺
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
			//	level0:		��ʵ�ν���eval���õ�value_list���£�
			//			value_list <start, 10> <end, 11> 
			//
			//	level1:		return parm1 + parm2 * length;
			//
			//	�ڶ�return������eval��ʱ����Ҫ��expression���ʽ�еı�������eval�����Ծ���Ҫ��¼����ǰ
			//	eval�õ��Ľ����
			// Note: �������Ƕ�CallExprֻ��evalһ�㣬�Ժ�Ҳ�п���eval���
			//		0��				add(start, end)				��¼��0���ʵ��ֵ
			//							   ||
			//							   ||
			//							   \/
			//		1��		return parm1 + parm2 * length;		�п�����Ҫ��¼��1���ʵ��ֵ
			//						//				\\
			//					   //				 \\
			//
			//		2��	�п����е����㣬������Ҫ					�п�����Ҫ��¼��2���ʵ��ֵ

			// To Do: ʹ��ջ����¼ʵ��ֵ��ģ����ʵ�������У�
			// ���磺 ��̬��չ��active stack��ÿһ��ջ֡����洢ʵ��ֵ��
			//	$$<start, 11> <end, 10> $$ <flag, false> $$ <>
			//  <level0, 2> <level2, 1> 
			// Note: moses��CallExpr���Ƶ���ʱֻ��һ�㡣

			// ��1�� ��vector�洢active stack info.
			std::vector<std::pair<std::string, EvalInfo>> ActiveStack;

			// ��2�� ��vector�洢stack bookkeeping info��level�Լ�ÿ���ж��ٸ�ʵ��ֵ.
			std::vector<std::pair<int, unsigned>> ActiveBookingInfo;
```
��Ȼmoses����ZEvalCallֻ��һ�㣬��������δ��������������չ��

  [1]: http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
  [2]: https://mortoray.com/2013/06/22/why-a-constexpr-is-just-a-return-statement/