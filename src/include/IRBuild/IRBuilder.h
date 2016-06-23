//===----------------------------IRBuilder.h-----------------------------===//
// 
// This class for intermediate generation.
// Note: IRBuilder refer to ��Crafting a Compiler��- "Syntax-directed 
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
		/// ����������������յõ�������Instruction������moses�﷨����python�����Զ���
		/// ͨ��block�����������Function Definition���Ƕ���ȥ��
		/// 
		/// ���⣬����ʹ��overload(����)ʵ�ֵڶ����dispatch��
		/// -------------------------------------------------------------------
		class ModuleBuilder : public StatementAST::Visitor
		{
		private:
			// This is the Symbol Tree.
			// ������IR code gen��ʱ����Ҫsymbol table���������Ǿ͸�����
			// sema�е�symbol table(��ʵ�Ƕ����scope�����ǿ���ͨ������scope����ȡ��
			// ���е�symbol��Ϣ).
			// ���磺
			//		func add(lhs : int, rhs : int) -> int
			//		{
			//			return lhs + rhs * 2 - 40 + lhs * (rhs - rhs / 10);
			//		}
			//		const global = 10;
			//		var num = add(global, 20) + 23;
			// �ڱ�����֮�����Ǿ�ֻʣ���˶����scope������ͼ��ʾ��
			//  ------ -------- -----
			// | add  | global | num |
			//  ------ -------- -----
			//     |
			//     |
			//    \|/
			//  ------- -----
			// |  lhs  | rhs |
			//  ------- -----
			// �����յõ���symbol table��Ϣ��������scope tree����ʽչ�ֵġ�
			std::shared_ptr<Scope> SymbolTree;
			std::shared_ptr<Scope> CurScope;

			ConstantEvaluator evaluator;

			// �ڹ���ModuleBuilder��ʱ�򣬽��ⲿ������Context���롣
			MosesIRContext &Context;
			CodeGenTypes Types;

			// ���յõ���IR info���ڱ����﷨���Ĺ����л᲻�ϵ���ӡ�
			// var num = 10;
			// if (num > 0)
			// {
			//		num = -10;
			// }
			// func add() -> int {}
			// num = 11;
			std::list<std::shared_ptr<Value>> IRs;

			// ��BasicBlock��ʾ��ǰ��BasicBlock.
			BBPtr CurBB;
			// ��Function��ʾ��ǰFunction.
			FuncPtr Func;

			// Current Iterator
			std::list<InstPtr>::iterator InsertPoint;
		public:
			ModuleBuilder(std::shared_ptr<Scope> SymbolInfo, MosesIRContext &context);

			/// \brief This method is used for get MosesIRContext.
			MosesIRContext& getMosesIRContext() { return Context; }

			// Emit IR for Module.
			// ����moses IR��˵���������б���������ʹ��ValueSymbolTable�洢����instruction
			// BasicBlock��Ҳ����˵�������value-list��ʹ��visit����single dispatch

			/// \brief ��ǰ�������ܿ��Ժ���������ͨ������ASTÿ���ڵ㣬������Ӧ��visitor
			/// ʹ��unique_ptr�����þ�����IR����֮��AST����������
			void VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST);

			//===----------------------------------------------------------------------===//
			// �����һϵ�е�visit()����ͨ������ʵ�ֵڶ����dispatch
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
			/// -- �ú�����ר��ΪIfStmt�����ɶ������ġ�
			/// (TrueCount should be the number of times we expect the condition to
			/// evaluate to true based on PGO data.)
			/// -- Clang�еİ汾Ϊ������Profile data����¼��true��֧�Ĳ��ô�����
			void EmitBranchOnBoolExpr(ExprASTPtr Cond, BBPtr TrueB, BBPtr FalseBlock/*, unsigned TrueCount*/);
		private:
			//===---------------------------------------------------------===//
			// Helper for CodeGen.

		private:
			//===---------------------------------------------------------===//
			// Helper for Instruction Creation.
			/// \brief �÷�������IRBuilder������һ��ָ��󣬽���ָ�����BasicBlock.
			template<typename InstTy>
			InstTy InsertHelper(InstTy I, std::string Name = "") const
			{
				CurBB->Insert(InsertPoint, I);
				I->SetName(Name);
				return I;
			}

			/// \brief ��IRBuilder����ָ���ͬʱ���п��ܽ���constant folder����ʱ�õ�value�Ͳ�������ָ�
			/// ��Ȼ����Ҫ����BasicBlock���ˡ�
			/// ģ���ػ�
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
			// �����ڹؼ��ֵ�Ԥ�棬�����ṩһЩ��������ֵ��method
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

			/// \brief ��������һ����ֵ����.
			/// To Do: ��Ҫ����һ������ר�ű�ʾ��ֵ��������ʹ��Aggregate Type����ʾ��ֵ���ء�
			/// ���磺
			///	func add() -> {int, {int, bool}} {}
			/// �����ֵ�����е�ÿ��ֵ��һ���Ǵ���ͬһ���еģ�����LLVM�е�Aggregate Typeֻ�ܱ�ʾ����
			/// ��ֵ��ͬһ��level�С����磺 {int, int, int}
			///
			///	moses�еĶ�ֵʾ��ͼ��
			///
			ReturnInstPtr CreateAggregateRet(std::vector<ValPtr> retVals, unsigned N);

			/// \brief Create an unconditional 'br label X' instruction.
			/// ���磺��if stmt�ķ����У�then���Ľ�β����Ҫunconditional branch
			BrInstPtr Create(BBPtr Dest);

			/// \brief Create a conditional 'br Cond, TrueDest, FalseDest' instruction.
			/// ��LLVM���ṩ��һЩ����Branch����Ϣ�������Ƿ��Ԥ�����Ϣ��
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