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
			void VisitChildren(std::vector<std::unique_ptr<StatementAST>> AST)
			{
				unsigned ASTSize = AST.size();
				for (int i = 0; i < ASTSize; i++)
				{
					// �ڱ���AST�Ĺ����У������AST�ľ���ڵ���ѡ���Ӧ��Accept������
					// ���磺��������� "IfStmt" �Ļ�����ѡ��IfStmt��Accept()����
					AST[i].get()->Accept(this);
				}
			}

			//===----------------------------------------------------------------------===//
			// �����һϵ�е�visit()����ͨ������ʵ�ֵڶ����dispatch
			//===---------------------------------------------------------------------===//
			
			/// \brief IR gen for NumberExpr
			void visit(ExprStatement* exprstmt)
			{
				// ���� virtual void Accept() ��ԭ��AST�������ͽڵ㶼�ж�Ӧ��RTTI��Ϣ��
				// ���ǿ��Ը��ݾ����type info�����в�ͬ�Ĳ�����
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
				// ��RTTI�жϲ�ͬ�Ľڵ㣬�����в�ͬ�Ĳ���
			}

			void visit(IfStatement* ifstmt)
			{
				// If the condition constant folds and can be elided, try to avoid emitting
				// the condition and the dead arm of the if/else
				bool CondConstant;
				/// \brief condAttr���ڱ�ʾIfStmt��������Ϣ
				/// 1��ʾ������ƶ��Һ�Ϊ�棬0��ʾ������ƶ��Һ�Ϊ�٣�-1��ʾ���벻���ƶ�
				short condAttr = ifstmt->CondCompileTimeDeduced();
				
				/// ConditionExpr��Ϊ�棬ɾ��else��֧������еĻ���
				if (condAttr != -1)
				{
					const StatementAST *Executed = ifstmt->getThen();
					const StatementAST *Skipped = ifstmt->getElse();
					/// ConditionExpr��Ϊ�٣���ɾ��true��֧
					/// �����û��else��֧�Ļ���ɾ������IfStmt�����ɣ�
					if (condAttr == 0)
						std::swap(Executed, Skipped);

					/// ע����C/C++�д���һ�������Ҫע�⣬����ʡ�Ե�block���п�����goto��Ŀ��
					/// label������Clang��Ҫ���block���Ƿ���label��
					/// ����mosesû��goto��Ҳ����˵�����ܴ������������
					if (Executed)
						//EmitStmt();
						return;
				}

				

				// Cond->CodeGen()

				// �м�Ҫ����Щ��תָ��
				// ����һЩ�򵥵��Ż��жϣ����condition expr�Ƕ�ֵ����ɾ���ٷ�֧���ִ���

				// Then->CodeGen()

				// Else->CodeGen()
			}

			void visit(WhileStatement* whilestmt)
			{
				// Cond->CodeGen()

				// �м�Ҫ����Щ��תָ��

				// ���м����ɵ���BasicBlock

				// CompoumdStmt->CodeGen()
			}

			void visit(DeclStatement* declstmt)
			{
				// ���ݾ����DeclStatement��������ѡ����Ӧ��IRBuilder
				// ���磺FunctionDecl��VarDecl��ClassDecl�Լ�UnpackDecl
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