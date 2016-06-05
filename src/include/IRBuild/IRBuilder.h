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
		/// 这个类用来保存最终得到的所有Instruction，由于moses语法类似python，所以顶层
		/// 通过block相关联，但是Function Definition如何嵌入进去。
		/// 
		/// 另外，这里使用overload(重载)实现第二层的dispatch。
		/// -------------------------------------------------------------------
		class ModuleBuilder : public IRBuilder
		{
		private:
			std::list<std::shared_ptr<Value>> IRs;
		public:
			// Emit IR for Module.
			// 对于moses IR来说，最外层的有变量声明（使用ValueSymbolTable存储），instruction
			// BasicBlock，也就是说最外层是value-list。使用visit进行single dispatch

			/// \brief 当前函数是总控性函数，可以通过遍历AST每个节点，调用相应的visitor
			/// 使用unique_ptr的作用就是在IR生成之后，AST被析构掉。
			void VisitChildren(std::vector<std::unique_ptr<StatementAST>> AST);

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
			/// -- 该函数是专门为IfStmt的生成而创建的。
			/// (TrueCount should be the number of times we expect the condition to
			/// evaluate to true based on PGO data.)
			/// -- Clang中的版本为了利用Profile data，记录了true分支的采用次数。
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
			// 在解析Function Body的时候返回的是In-memory形式的IR
			//------------------------------------------------------------------
		};

		/// \brief - ClassBodyBuilder
		class ClassBodyBuilder : public IRBuilder
		{
		public:
			//-------------------------nonsense for coding----------------------
			// moses中的class声明暂时还是很简化的，只有一系列的变量声明
			//------------------------------------------------------------------
		};

		/// \brief - CompoundStmtBuilder
		class CompoundStmtBuilder : public IRBuilder
		{
			//-------------------------nonsense for coding----------------------
			// 注意compoundstmt与BasicBlock不是等价的
			// 在IR生成的时候，需要为CompoundStmt建立SymTab吗？
			// 为了以后实现精确的RAII，需要在CompoundStmt结尾插入一些对象的析构函数
			//------------------------------------------------------------------
		};
	}
}

#endif