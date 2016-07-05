//===----------------------------IRBuilder.h-----------------------------===//
// 
// This class for intermediate generation.
// Note: IRBuilder refer to 《Crafting a Compiler》- "Syntax-directed 
// Translation" and "Code Generation for a Virtual Machine".
// 
//===--------------------------------------------------------------------===//

// ---------------------------nonsense for coding---------------------------
// At some pounsigned in each of the code-generation "visit" methods, actual code
// must be emitted. The syntax and specification of such code depend on the
// form of moses IR.
// -------------------------------------------------------------------------

#ifndef MOSES_IR_BUILDER_H
#define MOSES_IR_BUILDER_H
#include <list>
#include <memory>
#include <cassert>
#include "CodeGenTypes.h"
#include "../IRBuild/CGCall.h"
#include "../Parser/constant-evaluator.h"
#include "../Parser/ast.h"
#include "../IR/Value.h"
#include "../IR/Instruction.h"
#include "../IR/ConstantAndGlobal.h"
#include "../IR/Function.h"
#include "../IR/BasicBlock.h"
#include "../Parser/sema.h"
#include "../IR/ConstantFolder.h"
#include "../IR/MosesIRContext.h"
#include "CGValue.h"

namespace compiler
{
	namespace IRBuild
	{
		using namespace ast;
		using namespace sema;
		using namespace IR;
		using namespace CodeGen;		
		
		typedef BinaryOperator::Opcode Opcode;

		namespace CGExpr
		{
			struct BinOpInfo
			{
				ValPtr LHS;
				ValPtr RHS;
				compiler::IRBuild::ASTTyPtr Ty;
				const BinaryExpr* BE;
			};
		}

		/// \brief FunctionBuilderStatus - hold the status when we are generating code 
		/// for Funciton.
		struct FunctionBuilderStatus
		{
			FunctionDecl* CurFuncDecl;
			std::shared_ptr<CGFunctionInfo const> CurFnInfo;
			TyPtr FnRetTy;
			FuncPtr CurFn;
			// Count the return expr(contain implicit return-expr).
			unsigned NumReturnExprs;

			/// ReturnBlock - Unified return block.
			/// ReturnBlock can omit later.
			BBPtr ReturnBlock;

			/// ReturnValue - The temporary alloca to hold the return value. This is null
			/// iff the function has no return value.
			/// If we have multiple return statements, we need this tempprary alloca.
			/// e.g		func add(parm : var)
			///			{
			///				int num = 10;
			///				int mem = 20;
			///				if (num > 10)
			///					-----> we will load from num and store to the temp alloca.
			///					return num;	
			///				else
			///					-----> Just like 'return num', we need a temp alloca to hold the return value.
			///					return mem;	
			///			}
			///	moses IR:
			///			%1 = alloca i32 ----> temp alloca to hold the return value.
			///			...
			///		; <label>:then
			///			%6 = load i32* %num
			///			store i32 %6, i32* %1
			///			br <label> %9
			///		; <label>:else
			///			%7 = load i32* %mem
			///			store i32 %7， i32* %1
			///			br <label> %8
			///		; <label>:8
			///			%9 = load i32* %1
			///			ret i32 %9
			ValPtr ReturnValue;

			explicit FunctionBuilderStatus() : 
				CurFuncDecl(nullptr), CurFnInfo(nullptr), FnRetTy(nullptr), CurFn(nullptr), 
				ReturnValue(nullptr), NumReturnExprs(0)
			{}
		};

		/// \brief ModuleBuilder - This class for module code generation.
		/// The outermost portions of an AST contain class and method declarations.
		/// The ModuleBuilder is responsible for processing each class and method
		/// declarations.
		/// ------------------------nonsense for coding------------------------
		/// 这个类用来保存最终得到的所有Instruction，由于moses语法类似python，所以顶层
		/// 通过block相关联，但是Function Definition如何嵌入进去。
		/// 
		/// 另外，这里使用overload(重载)实现第二层的dispatch。
		/// -------------------------------------------------------------------
		class ModuleBuilder : public StatementAST::Visitor<ValPtr>
		{	
		public:
			using FuncStatusPtr = std::shared_ptr<FunctionBuilderStatus>;
		private:
			// We need symbol table to generate code.
			// e.g：
			//		func add(lhs : int, rhs : int) -> int
			//		{
			//			return lhs + rhs * 2 - 40 + lhs * (rhs - rhs / 10);
			//		}
			//		const global = 10;
			//		var num = add(global, 20) + 23;
			// ScopeTree:
			//  ------ -------- -----
			// | add  | global | num |
			//  ------ -------- -----
			//     |
			//     |
			//    \|/
			//  ------- -----
			// |  lhs  | rhs |
			//  ------- -----
			std::shared_ptr<Scope> SymbolTree;
			std::shared_ptr<Scope> CurScope;

			// Auxiliary module.
			ConstantEvaluator evaluator;
			MosesIRContext &Context;
			CodeGenTypes Types;

			// IRs for the whole translation-unit.
			// e.g.		var num = 10;									------------		
			//			if (num > 0)								   |	BBPtr	|
			//			{												------------
			//				num = -num;								   |	BBPtr	|
			//			}												------------
			//			func add(lhs : int, rhs : int) -> int		   |   FuncPtr	|
			//			{												------------
			//				var sum = 0;
			//				while(lhs > rhs)
			//				{
			//					lhs--;
			//					sum = sum + rhs;
			//				}
			//			}
			std::list<std::shared_ptr<Value>> IRs;

			BBPtr CurBB;
			// CurFunc - Contains all the state information about current function.
			FuncStatusPtr CurFunc;

			// 'InsertPoint' is pointing at the end of current basic block of most of the time.
			// Note we should clear 'InsertPoint' when we switch to the new block.
			std::list<InstPtr>::iterator InsertPoint;

			// The insert point for alloca instructions.
			// Alloca instruction only appear in the Entry Block.
			// e.g:		define i32 add(i32 parm)
			//			{
			//							---------------------
			//						   |	AllocaInst		 | ----> Entry Block
			//						   |	AllocaInst		 |	
			//						   |	Other Inst		 | ----> Alloca insert point.
			//							----------/------\----
			//									 /		  \
			//				--------------------/		   \------------------
			//			   |				     |		   |				  |
			//	BB1 <----  |					 |		   |				  |	----> BB2
			//			   |					 |		   |				  |
			//				---------------------			------------------
			//									...
			//			}
			// Before executing the function code, we should pre-allocate a portion of memory
			// on the stack.
			std::list<InstPtr>::iterator AllocaInsertPoint;
		public:
			ModuleBuilder(std::shared_ptr<Scope> SymbolInfo, MosesIRContext &context);

			/// \brief This method is used for get MosesIRContext.
			MosesIRContext& getMosesIRContext() { return Context; }

			// Emit IR for Module.
			// 对于moses IR来说，最外层的有变量声明（使用ValueSymbolTable存储），instruction
			// BasicBlock，也就是说最外层是value-list。使用visit进行single dispatch

			/// \brief 当前函数是总控性函数，可以通过遍历AST每个节点，调用相应的visitor
			/// 使用unique_ptr的作用就是在IR生成之后，AST被析构掉。
			void VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST);

			//===----------------------------------------------------------------------===//
			// 下面的一系列的visit()函数通过重载实现第二层的dispatch
			//===---------------------------------------------------------------------===//		
			// Note: We only need to consider the type of AST leaf node.
			ValPtr visit(const StatementAST* stmt) { return nullptr; }
			ValPtr visit(const ExprStatement* exprstmt) { return nullptr; }
			ValPtr visit(const CompoundStmt* comstmt);
			ValPtr visit(const IfStatement* ifstmt);			
			ValPtr visit(const WhileStatement* whilestmt);
			ValPtr visit(const ReturnStatement* retstmt);
			ValPtr visit(const DeclStatement* declstmt) { return nullptr; }
			ValPtr visit(const VarDecl* VD);
			ValPtr visit(const ParameterDecl* PD) { return nullptr; }
			ValPtr visit(const ClassDecl* CD);
			ValPtr visit(const FunctionDecl* FD);
			ValPtr visit(const UnpackDecl* UD);
			ValPtr visit(const BinaryExpr* B);
			ValPtr visit(const CallExpr* Call);
			ValPtr visit(const DeclRefExpr* DRE);
			ValPtr visit(const BoolLiteral* BL);
			ValPtr visit(const NumberExpr* NE);;
			ValPtr visit(const UnaryExpr* UE);
			ValPtr visit(const MemberExpr* ME);
			ValPtr visit(const Expr* E) { return nullptr; }
		private:
			//===-----------------------------------------------------------===//
			// Helper for variable declaration generation.
			//===-----------------------------------------------------------===//
			class VarEmission
			{
				const VarDecl* var;

				/// The address of the alloca. Invalid if the variable was emitted as a global
				/// constant.

				/// True if the variable is of aggregate type and has a constant initializer.
				bool IsConstantAggregate;
				bool wasEmittedAsGlobal() const {}
			public:
			};

			/// EmitVarDecl - This method handles emission of any variable declaration inside
			/// a function.
			void EmitLocalVarDecl(const VarDecl* var);

			/// \brief Emit a branch from the current block to the target one if this
			/// was a real block.
			void EmitBrach(BBPtr Target);

			/// EmitParmDecl - Emit an alloca (or GlobalValue depending on target)
			/// for the specified parameter and set up LocalDeclMap.
			void EmitParmDecl(const VarDecl* VD, ValPtr Arg);			

			/// EmitLocalVarAlloca - Emit the alloca.
			AllocaInstPtr EmitLocalVarAlloca(const VarDecl* var);
			//===--------------------------------------------------------------===//
			// Helper for function call generation.
			//===--------------------------------------------------------------===//
			void EmitCall();
			void EmitCallArg();

			/// EmitBranchOnBoolExpr - Emit a branch on a boolean condition(e.g for an
			/// if statement) to the specified blocks. Based on the condition, this might
			/// try to simplify the codegen of the conditional based on the branch.
			/// To Do: TrueCount for PGO(profile-guided optimization).
			void EmitBranchOnBoolExpr(ExprASTPtr Cond, BBPtr TrueB, BBPtr FalseBlock/*, unsigned TrueCount*/);

			//===---------------------------------------------------------===//
			// Helper for builder configuration.
			//===---------------------------------------------------------===//

			/// \brief Insert a new instruction to the current block.
			template<typename InstTy>
			InstTy InsertHelper(InstTy I, std::string Name = "") const
			{
				CurBB->Insert(InsertPoint, I);
				I->setName(Name);
				return I;
			}

			AllocaInstPtr InsertHelper(AllocaInstPtr I, std::string Name = "") const
			{
				CurBB->Insert(AllocaInsertPoint, I);
				I->setName(Name);
				return I;
			}

			/// \brief Template specialization.
			ValPtr InsertHelper(ConstantBoolPtr V, std::string Name = "") const { return V; }
			ValPtr InsertHelper(ConstantIntPtr V, std::string Name = "") const { return V; }
			/// \brief Clear the insertion point: created instructions will not be
			/// inserted into a block.
			void ClearInsertionPoint() 
			{
				CurBB = nullptr;
				// Note: We can't make 'InsertPoint' invalid or assign nullptr.
				// To Do: When we leave a BB, we need make 'InsertPoint' invalid.
				// InsertPoint = nullptr;
			}

			/// \brief This specifies that created instructions should be appended to the
			/// end of the ** specified block **.
			void SetInsertPoint(BBPtr TheBB);

			/// \brief This specifies that created instructions should be inserted before
			/// the specified instruction.
			void SetInsertPoint(InstPtr I);

			//===---------------------------------------------------------------===//
			// Miscellaneous creation methods.
			//===---------------------------------------------------------------===//

			/// \brief Get the constant value for i1 true.
			ConstantBoolPtr getTrue() { return ConstantBool::getTrue(); }
			/// \brief Get the constant value for i1 false;
			ConstantBoolPtr getFalse() { return ConstantBool::getFalse(); }
			/// \brief Get the constant value for int.
			ConstantIntPtr getInt(int val) { return ConstantInt::get(val); }
		
			//===---------------------------------------------------------===//
			// Instruction creation methods: Terminators
			//===---------------------------------------------------------===//
			/// \brief Create a 'ret void' instruction.
			ReturnInstPtr CreateRetVoid();
			/// \brief Create a 'ret <val>' instruction.
			ReturnInstPtr CreateRet(ValPtr);

			/// \brief 用来创建一个多值返回.
			/// To Do: 需要定义一个类来专门表示多值。我们欲使用Aggregate Type来表示多值返回。
			/// 例如：
			///	func add() -> {int, {int, bool}} {}
			/// 这个多值返回中的每个值不一定是处在同一层中的，但是LLVM中的Aggregate Type只能表示其中
			/// 的值在同一个level中。例如： {int, int, int}
			///
			///	moses中的多值示意图：
			///
			ReturnInstPtr CreateAggregateRet(std::vector<ValPtr> retVals, unsigned N);

			/// \brief Create an unconditional 'br label X' instruction.
			/// 例如：在if stmt的翻译中，then语句的结尾就需要unconditional branch
			BrInstPtr Create(BBPtr Dest);

			/// \brief Create a conditional 'br Cond, TrueDest, FalseDest' instruction.
			/// 在LLVM中提供了一些关于Branch的信息，例如是否可预测等信息。
			/// MDNode *BranchWeights 
			/// MDNode *Unpredictable
			BrInstPtr CreateCondBr(ValPtr Cond, BBPtr True, BBPtr False);

			/// \brief Create an unconditional 'br label X' instruction.
			BrInstPtr CreateBr(BBPtr Dest);

			//===------------------------------------------------------------------===//
			// Instruction creation methods: Binary Operators
			//===------------------------------------------------------------------===//

			BOInstPtr CreateInsertBinOp(BinaryOperator::Opcode Opc, ValPtr LHS, ValPtr RHS,
				std::string Name);
			ValPtr CreateAdd(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateSub(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateMul(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateDiv(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateRem(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateShl(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateShr(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateAnd(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateOr(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateNeg(ValPtr V, std::string Name = "");
			ValPtr CreateNot(ValPtr V, std::string Name = "");

			//===------------------------------------------------------------------===//
			// Instruction creation methods: Memory Instructions.
			//===------------------------------------------------------------------===//
			AllocaInstPtr CreateAlloca(TyPtr Ty, std::string Name = "");
			LoadInstPtr CreateLoad(ValPtr Ptr);
			StoreInstPtr CreateStore(ValPtr Val, ValPtr Ptr);
			GEPInstPtr CreateGEP(TyPtr Ty, ValPtr Ptr, std::vector<ValPtr> IdxList, std::string Name = "");

			//===---------------------------------------------------------------===//
			// Instruction creation methods: Compare Instructions.
			//===---------------------------------------------------------------===//
			ValPtr CreateCmpEQ(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmpNE(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmpGT(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmpGE(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmpLT(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmpLE(ValPtr LHS, ValPtr RHS, std::string Name = "");
			ValPtr CreateCmp(CmpInst::Predicate P, ValPtr LHS, ValPtr RHS, std::string Name = "");

			//===---------------------------------------------------------------===//
			// Instruction creation methods: Other Instructions.
			//===---------------------------------------------------------------===//
			PHINodePtr CreatePHI(TyPtr Ty, unsigned NumReservedValues, std::string Name = "");
			CallInstPtr CreateCall(ValPtr Callee, std::vector<ValPtr> Args, std::string Name = "");
			EVInstPtr CreateExtractValueValue(ValPtr Agg, std::vector<unsigned> Idxs, std::string Name = "");

			//===----------------------------------------------------------------------===//
			// Utility creattion methods
			//===----------------------------------------------------------------------===//
			/// \brief Return an i1 value testing if \p Arg is null
			ValPtr CreateIsNull(ValPtr Arg, std::string Name = "");
			/// \brief Return an i1 value testing if \p Arg is not null.
			ValPtr CreateIsNotNull(ValPtr Arg, std::string Name = "");

			//===----------------------------------------------------------------------===//
			// Other create*().
			//===----------------------------------------------------------------------===//
			BBPtr CreateBasicBlock(std::string Name, FuncPtr parent = nullptr, BBPtr before = nullptr);			
		private:
			//===---------------------------------------------------------===//
			// Helpers for value.
			//===---------------------------------------------------------===//
		private:
			//===---------------------------------------------------------===//
			/// \brief Emit code for function decl.
			void EmitFunctionDecl(const FunctionDecl* FD);

			/// \brief Handle the start of function.
			void StartFunction(std::shared_ptr<CGFunctionInfo const> FnInfo, FuncPtr Fn);

			/// \brief Complete IR generation of the current function. It is legal to call
			/// this function even if there is no current insertion point.
			void FinishFunction();

			// EmitFunctionPrologue - Mainly for transferring formal parameters.
			// https://en.wikipedia.org/wiki/Function_prologue
			// -------------------------nonsense for coding------------------------
			// The function prologue is a few lines of code at the beginning of a 
			// function, which prepare the 'stack' and 'registers' for use within 
			// the funciton.
			// - Pushes the old base pointer onto the stack, such it can be restored later.
			// - Assign the value of stack pointer(which is pointed to the saved base pointer
			//	 and the top of the old stack frame) into base pointer such that a new stack 
			// frame will be created on top of the old stack frame(i.e. the top of the old 
			//	stack frame will become the base of the new stack frame).
			// - Move the stack pointer further by decreasing or increasing its value, depending
			//  on the whether the stack grows down or up. On x86, the stack pointer is decreased
			//  to make room for variables(i.e. the functions's local variables).
			// e.g.		======================
			//		   |	pushl %ebp		  |
			//			======================
			//		   |	movl %esp, %ebp	  |
			//			======================
			//		   |	subl $N, %esp	  |
			//			======================
			void EmitFunctionPrologue(std::shared_ptr<CGFunctionInfo const> FunInfo, FuncPtr fun);

			// EmitFunctionEpilogue - Reverse the actions of the function prologue and returns
			// control to the calling funciton. It typically does the following actions (this 
			// procedure may differ from one architecture to another):
			// - Replaces the stack pointer with the current base (or frame) pointer, so the
			//   stack pointer is restored to its value before the prologue
			// - Pops the base pointer off the stack, so it is restored to its value before the
			//   prologue.
			// - Returns to calling function(caller), by popping the previous frame's program 
			//   counter off the stack and jumping to it.
			// e.g.			======================
			//			   |	movl %ebp, %esp	  |
			//				======================
			//			   |	popl %ebp		  |
			//				======================
			//			   |	ret				  |
			//				======================
			void EmitFunctionEpilogue();
			//===---------------------------------------------------------===//
			// Emit code for stmts.

			void EmitFunctionBody(StmtASTPtr body);

			/// \brief Emit the unified return block, trying to avoid its emission when possible.
			void EmitReturnBlock();

			/// EmitCompoundStmt - Emit code for compound statements.
			/// e.g.	{
			///				stmts.
			///			}
			/// FixMe: I don't know why we need to return a value. But Clang 'EmitCompoundStmt'
			///	need a value to return.
			ValPtr EmitCompoundStmt(const StatementAST* S);
			void EmitIfStmt(const IfStatement* ifstmt);
			void EmitConBrHints();
			void EmitWhileStmt(const WhileStatement* S);
			void EmitReturnStmt(const ReturnStatement* S);
			void EmitDeclStmt(const DeclStatement* S);
			void EmitBreakStmt(const BreakStatement* S);
			void EmitContinueStmt(const ContinueStatement* S);

			//===---------------------------------------------------------===//
			// Emit code for expressions.

			/// EmitLValue() - Emit code to compute a designator that specifies the location
			/// of the expression. This function just return a simple address.
			/// For example:
			///		var num : int;
			///		num = 10;
			///		~~~
			/// We should emit code for 'num'.
			LValue EmitLValue(const Expr* E);

			LValue EmitDeclRefLValue(const DeclRefExpr* DRE);

			/// EmitDeclRefLValue - Emit code to compute the location of DeclRefExpr.
			/// For example:
			///		num = mem;
			///		~~~
			LValue EmitDeclRefExpr(const DeclRefExpr* E);

			/// EmitExprAsInit - Emits the code necessary to initialize a location in
			/// memory with the given initializer.
			ValPtr EmitBinaryExpr(const BinaryExpr* BE);
			void EmitCallExpr(const CallExpr* CE);

			/// \brief EmitUnaryExpr - Emit code for unary expression, including '-' '!' '--' '++'.
			ValPtr EmitUnaryExpr(const UnaryExpr* UE);

			/// \brief EmitPrePostIncDec - Emit code for '--' '++'.
			ValPtr EmitPrePostIncDec(const UnaryExpr* UE, bool isInc, bool isPre);

			void EmitMemberExpr(const MemberExpr* ME);

			/// \brief Handle the algebraic and boolean operation, include '+' '-' '*' '/' '%'
			///	'<' '>' '<=' '>=' '==' '!='.
			ValPtr EmitAlgAndBooleanOp(const CGExpr::BinOpInfo& BE);

			/// \brief Handle the assign operation, include '='.
			ValPtr EmitBinAssignOp(const BinaryExpr* BE);

			/// \brief Handle the compound assign operation, include '*=' '/=' '%=' '+=' '-='
			///	'&&=' '||='.
			ValPtr EmitCompoundAssignOp(const BinaryExpr* BE);

			void EmitExprAsInit(const Expr *init, const VarDecl* D, LValue lvalue);
			void EmitScalarInit(const Expr* init, const VarDecl* D, LValue lvalue);

			/// \brief EmitLoadOfLValue - Given an expression with complex type that represents
			/// a l-value, this method emits the address of the l-value, then loads and returns
			/// the result.
			ValPtr EmitLoadOfLValue(const Expr* E);

			/// \brief EmitLoadOfLValue - Given an expression that represents a value lvalue,
			/// this method emits the address of the lvalue, then loads the result as an rvalue,
			/// returning the rvalue.
			RValue EmitLoadOfLValue(LValue LV, ASTTyPtr ExprTy);

			/// EmitStoreOfScalar - Store a scalar value to an address.
			void EmitStoreOfScalar(ValPtr Value, ValPtr Addr);

			/// EmitScalarExpr - Emit the computation of the specified expression of
			/// scalar type, returning the result.
			ValPtr EmitScalarExpr(const Expr* E);

			/// EmitStoreThroughLValue - Store the specified rvalue into the specified
			/// lvalue.
			///	For Example:
			///		num = mem.	--------> EmitBinaryExpr()
			///		   ~~~
			///
			/// At the end of the 'EmitBinaryExpr()', we need handle the assignment(
			///	load from mem and store to the num).
			void EmitStoreThroughLValue(RValue Src, LValue Dst, bool isInit = false);


			/// EmitBlock - Emit the given block \arg BB and set it as the insert point, 
			/// adding a fall-through branch from current insert block if necessary.
			void EmitBlock(BBPtr BB, bool IsFinished = false);

			/// EvaluateExprAsBool = Perform the usual unary conversions on the specified
			/// expression and compare the result against zero, returning an Int1Ty value.
			ValPtr EvaluateExprAsBool(ExprASTPtr E);
		};
	}
}
#endif