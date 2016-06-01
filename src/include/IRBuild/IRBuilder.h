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
			void VisitChildren(std::vector<std::unique_ptr<StatementAST>> AST)
			{
				unsigned ASTSize = AST.size();
				for (int i = 0; i < ASTSize; i++)
				{
					// 在遍历AST的过程中，会根据AST的具体节点来选择对应的Accept函数。
					// 例如：如果这里是 "IfStmt" 的话，会选择IfStmt的Accept()函数
					AST[i].get()->Accept(this);
				}
			}

			//===----------------------------------------------------------------------===//
			// 下面的一系列的visit()函数通过重载实现第二层的dispatch
			//===---------------------------------------------------------------------===//
			
			/// \brief IR gen for NumberExpr
			void visit(ExprStatement* exprstmt)
			{
				// 由于 virtual void Accept() 的原因，AST所有类型节点都有对应的RTTI信息，
				// 我们可以根据具体的type info来进行不同的操作。
				if (const NumberExpr* NE = dynamic_cast<NumberExpr*>(exprstmt))
				{

				}
				else if (const CharExpr* CE = dynamic_cast<CharExpr*>(exprstmt))
				{

				}
				else if (const StringLiteral* SL = dynamic_cast<StringLiteral*>(exprstmt))
				{

				}
				else if (const BoolLiteral* BL = dynamic_cast<BoolLiteral*>(exprstmt))
				{

				}
				else if (const DeclRefExpr* DRE = dynamic_cast<DeclRefExpr*>(exprstmt))
				{

				}
				else if (const BinaryExpr* BE = dynamic_cast<BinaryExpr*>(exprstmt))
				{

				}
				else if (const UnaryExpr* UE = dynamic_cast<UnaryExpr*>(exprstmt))
				{

				}
				else if (const CallExpr* call = dynamic_cast<CallExpr*>(exprstmt))
				{

				}
				else if (const MemberExpr* ME = dynamic_cast<MemberExpr*>(exprstmt))
				{

				}
				else if (const AnonymousInitExpr* AIE = dynamic_cast<AnonymousInitExpr*>(exprstmt))
				{

				}
				else
				{


				}
			}

			void visit(CompoundStmt* comstmt)
			{
				// 由RTTI判断不同的节点，来进行不同的操作
			}

			void visit(IfStatement* ifstmt)
			{
				// If the condition constant folds and can be elided, try to avoid emitting
				// the condition and the dead arm of the if/else
				bool CondConstant;
				/// \brief condAttr用于表示IfStmt的属性信息
				/// 1表示编译可推断且恒为真，0表示编译可推断且恒为假，-1表示编译不可推断
				short condAttr = ifstmt->CondCompileTimeDeduced();
				
				/// ConditionExpr恒为真，删除else分支（如果有的话）
				if (condAttr != -1)
				{
					const StatementAST *Executed = ifstmt->getThen();
					const StatementAST *Skipped = ifstmt->getElse();
					/// ConditionExpr恒为假，则删除true分支
					/// （如果没有else分支的话，删除整个IfStmt的生成）
					if (condAttr == 0)
						std::swap(Executed, Skipped);

					/// 注意在C/C++中存在一种情况需要注意，就是省略的block中有可能有goto的目标
					/// label，所以Clang需要检查block中是否有label。
					/// 但是moses没有goto，也就是说不可能存在上述情况。
					if (Executed)
						//EmitStmt();
						return;
				}

				

				// Cond->CodeGen()

				// 中间要穿插些跳转指令
				// 进行一些简单的优化判断，如果condition expr是定值，则删除假分支部分代码

				// Then->CodeGen()

				// Else->CodeGen()
			}

			void visit(WhileStatement* whilestmt)
			{
				// Cond->CodeGen()

				// 中间要穿插些跳转指令

				// 这中间生成的是BasicBlock

				// CompoumdStmt->CodeGen()
			}

			void visit(DeclStatement* declstmt)
			{
				// 根据具体的DeclStatement的类型来选择相应的IRBuilder
				// 例如：FunctionDecl、VarDecl、ClassDecl以及UnpackDecl
				if (const VarDecl* VD = dynamic_cast<VarDecl*>(declstmt))
				{
				}
				else if (const FunctionDecl* FD = dynamic_cast<FunctionDecl*>(declstmt))
				{

				}
				else if (const ClassDecl* CD = dynamic_cast<ClassDecl*>(declstmt))
				{

				}
				else
				{
					// IR error
				}
			}
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