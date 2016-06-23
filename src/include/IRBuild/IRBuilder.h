//===----------------------------IRBuilder.h-----------------------------===//
// 
// This class for intermediate generation.
// Note: IRBuilder refer to 《Crafting a Compiler》- "Syntax-directed 
// Translation" and "Code Generation for a Virtual Machine".
// 
//===--------------------------------------------------------------------===//

// ---------------------------nonsense for coding---------------------------
// At some point in each of the code-generation "visit" methods, actual code
// must be emitted. The syntax and specification of such code depend on the
// form of moses IR.
// -------------------------------------------------------------------------

#ifndef MOSES_IR_BUILDER_H
#define MOSES_IR_BUILDER_H
#include <list>
#include <memory>
#include "../Parser/constant-evaluator.h"
#include "../Parser/ast.h"
#include "../IR/Value.h"
#include "../IR/Instruction.h"
#include "../IR/ConstantAndGlobal.h"
#include "../IR/BasicBlock.h"
#include "../Parser/sema.h"
#include "../IR/ConstantFolder.h"
#include "../IR/MosesIRContext.h"
#include "CodeGenTypes.h"
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
		class ModuleBuilder : public StatementAST::Visitor
		{
		private:
			// This is the Symbol Tree.
			// 由于在IR code gen的时候，需要symbol table，所以我们就复用了
			// sema中的symbol table(其实是顶层的scope，我们可以通过顶层scope来获取到
			// 所有的symbol信息).
			// 例如：
			//		func add(lhs : int, rhs : int) -> int
			//		{
			//			return lhs + rhs * 2 - 40 + lhs * (rhs - rhs / 10);
			//		}
			//		const global = 10;
			//		var num = add(global, 20) + 23;
			// 在编译完之后，我们就只剩下了顶层的scope，如下图所示：
			//  ------ -------- -----
			// | add  | global | num |
			//  ------ -------- -----
			//     |
			//     |
			//    \|/
			//  ------- -----
			// |  lhs  | rhs |
			//  ------- -----
			// 我最终得到的symbol table信息，就是以scope tree的形式展现的。
			std::shared_ptr<Scope> SymbolTree;
			std::shared_ptr<Scope> CurScope;

			ConstantEvaluator evaluator;

			// 在构造ModuleBuilder的时候，将外部创建的Context传入。
			MosesIRContext &Context;
			CodeGenTypes Types;

			// 最终得到的IR info，在遍历语法树的过程中会不断的添加。
			// var num = 10;
			// if (num > 0)
			// {
			//		num = -10;
			// }
			// func add() -> int {}
			// num = 11;
			std::list<std::shared_ptr<Value>> IRs;

			// 该BasicBlock表示当前的BasicBlock.
			BBPtr CurBB;
			// 该Function表示当前Function.
			FuncPtr Func;

			// Current Iterator
			std::list<InstPtr>::iterator InsertPoint;
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
			/// \brief IR gen for NumberExpr
			void visit(const ExprStatement* exprstmt);

			/// \brief Generate code for CompoundStmt.
			void visit(const CompoundStmt* comstmt);

			/// \brief Generate code for IfStatement.
			void visit(const IfStatement* ifstmt);			

			void visit(const WhileStatement* whilestmt);

			void visit(const VarDecl* VD);

			void visit(const ClassDecl* CD);

			void visit(const FunctionDecl* FD);

			void visit(const UnpackDecl* UD);

			/// \brief
			void visit(const BinaryExpr* B);

			/// \brief
			void visit(const CallExpr* Call);

		private:
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
			void EmitLocalVarDecl(VarDeclPtr var);

			/// \brief Emit a branch from the current block to the target one if this
			/// was a real block.
			void EmitBrach(BBPtr Target);

			/// EmitLocalVarAlloca - Emit the alloca.
			void EmitLocalVarAlloca(VarDeclPtr var);

			void EmitCall();

			void EmitCallArg();

			/// EmitBranchOnBoolExpr - Emit a branch on a boolean condition(e.g for an
			/// if statement) to the specified blocks. Based on the condition, this might
			/// try to simplify the codegen of the conditional based on the branch.
			/// -- 该函数是专门为IfStmt的生成而创建的。
			/// (TrueCount should be the number of times we expect the condition to
			/// evaluate to true based on PGO data.)
			/// -- Clang中的版本为了利用Profile data，记录了true分支的采用次数。
			void EmitBranchOnBoolExpr(ExprASTPtr Cond, BBPtr TrueB, BBPtr FalseBlock/*, unsigned TrueCount*/);
		private:
			//===---------------------------------------------------------===//
			// Helper for CodeGen.

		private:
			//===---------------------------------------------------------===//
			// Helper for Instruction Creation.
			/// \brief 该方法会在IRBuilder创建完一条指令后，将该指令插入BasicBlock.
			template<typename InstTy>
			InstTy InsertHelper(InstTy I, std::string Name = "") const
			{
				CurBB->Insert(InsertPoint, I);
				I->SetName(Name);
				return I;
			}

			/// \brief 在IRBuilder创建指令的同时，有可能进行constant folder，这时得到value就不能算是指令，
			/// 自然不需要插入BasicBlock中了。
			/// 模板特化
			ValPtr InsertHelper(ConstantBoolPtr V, std::string Name = "") const { return V; }

			ValPtr InsertHelper(ConstantIntPtr V, std::string Name = "") const { return V; }

			/// \brief This specifies that created instructions should be appended to the
			/// end of the ** specified block **.
			void SetInsertPoint(BBPtr TheBB);

			/// \brief This specifies that created instructions should be inserted before
			/// the specified instruction.
			void SetInsertPoint(InstPtr I);

			//===---------------------------------------------------------------===//
			// Miscellaneous creation methods.
			// 类似于关键字的预存，这里提供一些创建字面值的method
			//===---------------------------------------------------------------===//

			/// \brief Get the constant value for i1 true.
			ConstantBoolPtr getTrue() { return ConstantBool::getTrue(); }

			/// \brief Get the constant value for i1 false;
			ConstantBoolPtr getFalse() { return ConstantBool::getFalse(); }

			/// \brief Get the constant value for int.
			ConstantIntPtr getInt(int val) { return ConstantInt::get(val); }
		private:
			//===---------------------------------------------------------===//
			// Instruction creation methods: Terminators
			//===---------------------------------------------------------===//
			/// \brief Create a 'ret void' instruction.
			ReturnInstPtr CreateRetVoid();

			/// \brief Create a 'ret <val>' instruction.
			ReturnInstPtr Create(ValPtr V);

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

			//===------------------------------------------------------------------===//
			// Instruction creation methods: Binary Operators
			//===------------------------------------------------------------------===//
		private:
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

		private:
			//===---------------------------------------------------------===//
			// Helpers for value.
			//===---------------------------------------------------------===//
		};
	}
}
#endif