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
#include "../Parser/ast.h"
#include "../IR/Value.h"
#include "../Parser/constant-evaluator.h"
#include "../IR/BasicBlock.h"

namespace compiler
{
	namespace IR
	{
		using namespace ast;
		/// \brief IRBuilder - This class inherit Visitor
		class IRBuilder : public StatementAST::Visitor
		{
		public:
			virtual void visit()
			{
				
			}
		};

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
		class ModuleBuilder : public IRBuilder
		{
		private:
			std::list<std::shared_ptr<Value>> IRs;
		public:
			// Emit IR for Module.
			// ����moses IR��˵���������б���������ʹ��ValueSymbolTable�洢����instruction
			// BasicBlock��Ҳ����˵�������value-list��ʹ��visit����single dispatch

			/// \brief ��ǰ�������ܿ��Ժ���������ͨ������ASTÿ���ڵ㣬������Ӧ��visitor
			/// ʹ��unique_ptr�����þ�����IR����֮��AST����������
			void VisitChildren(std::vector<std::unique_ptr<StatementAST>> AST);

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

			void visit(const DeclStatement* declstmt);

			void visit(const VarDecl* VD);

			void visit(const ClassDecl* CD);

			void visit(const FunctionDecl* FD);

			void visit(const UnpackDecl* UD);

			/// \brief
			void visit(const Expr* expr);

			/// \brief
			void visit(const BinaryExpr* B);

			/// \brief
			void visit(const CallExpr* Call);

			void EmitCall();

			void EmitCallArg();

			/// EmitBranchOnBoolExpr - Emit a branch on a boolean condition(e.g for an
			/// if statement) to the specified blocks. Based on the condition, this might
			/// try to simplify the codegen of the conditional based on the branch.
			/// -- �ú�����ר��ΪIfStmt�����ɶ������ġ�
			/// (TrueCount should be the number of times we expect the condition to
			/// evaluate to true based on PGO data.)
			/// -- Clang�еİ汾Ϊ������Profile data����¼��true��֧�Ĳ��ô�����
			void EmitBranchOnBoolExpr(const Expr* Cond, BBPtr TrueB, BBPtr FalseBlock/*, unsigned TrueCount*/);
		};

		/// \brief FunctionBodyBuilder - This class for Function Body generation.
		/// class Function:
		///		std::list<BBPtr> BasicBlocks;
		///		std::list<ArgPtr> Arguments;
		///		SymTabPtr
		class FunctionBodyBuilder : public IRBuilder
		{
			// Emit code for Function
			//---------------------------nonsense for coding--------------------
			// �ڽ���Function Body��ʱ�򷵻ص���In-memory��ʽ��IR
			//------------------------------------------------------------------
		};

		/// \brief - ClassBodyBuilder
		class ClassBodyBuilder : public IRBuilder
		{
		public:
			//-------------------------nonsense for coding----------------------
			// moses�е�class������ʱ���Ǻܼ򻯵ģ�ֻ��һϵ�еı�������
			//------------------------------------------------------------------
		};

		/// \brief - CompoundStmtBuilder
		class CompoundStmtBuilder : public IRBuilder
		{
			//-------------------------nonsense for coding----------------------
			// ע��compoundstmt��BasicBlock���ǵȼ۵�
			// ��IR���ɵ�ʱ����ҪΪCompoundStmt����SymTab��
			// Ϊ���Ժ�ʵ�־�ȷ��RAII����Ҫ��CompoundStmt��β����һЩ�������������
			//------------------------------------------------------------------
		};
	}
}

#endif