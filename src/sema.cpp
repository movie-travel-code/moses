//===------------------------------sema.cpp--------------------------------===//
//
// This file is used to implement sema.
// To Do: Implement action routines.
// 
//===---------------------------------------------------------------------===//
#include "sema.h"
#include "error.h"
namespace compiler
{
	namespace sema
	{
		std::shared_ptr<Symbol> Scope::Resolve(std::string name) const
		{
			// Look up the name in current scope.
			for (auto item : SymbolTable)
			{
				if (item->getLexem() == name)
				{
					return item;
				}
			}
			if (Parent)
			{
				return Parent->Resolve(name);
			}
			return nullptr;
		}

		/// \biref ���������Ҫ�������ļ�ͷ����Top-Level Scope.
		void Sema::ActOnTranslationUnitStart()
		{
			CurScope = std::make_shared<Scope>("##TranslationUnit", 0, nullptr, 
				Scope::ScopeKind::SK_TopLevel);
			ScopeStack.push_back(CurScope);
		}

		/// \brief ActOnFunctionDecl - Mainly for checking function name and recording function name.
		void Sema::ActOnFunctionDeclStart(std::string name)
		{
			// Check function name.
			// Note: moses doesn't support function overload now.
			if (CurScope->Resolve(name))
			{
				errorSema("Function redefinition.");
			}

			// Create new scope for function.
			unsigned CurDepth = CurScope->getDepth();
			std::shared_ptr<Scope> funcScope = std::make_shared<Scope>(name, CurDepth + 1, 
				CurScope, Scope::ScopeKind::SK_Function);

			// Record this func symbol.
			std::shared_ptr<FunctionSymbol> funcsym= std::make_shared<FunctionSymbol>(name, nullptr, CurScope, funcScope);
			CurScope->addDef(funcsym);

			// Update CurScope.			
			CurScope = funcScope;
			ScopeStack.push_back(CurScope);
			FunctionStack.push_back(funcsym);
		}

		/// \brief ActOnFunctionDecl - Set return type and create new scope.
		void Sema::ActOnFunctionDecl(std::string name, std::shared_ptr<Type> returnType)
		{
			FunctionStack[FunctionStack.size() - 1]->setReturnType(returnType);

			// Create new scope for function body.
			CurScope = std::make_shared<Scope>("", CurScope->getDepth() + 1, CurScope,
				Scope::ScopeKind::SK_Block);
			ScopeStack.push_back(CurScope);
		}

		/// \brief Action routines about IfStatement.
		StmtASTPtr Sema::ActOnIfStmt(SourceLocation start, SourceLocation end, ExprASTPtr condition, StmtASTPtr ThenPart, StmtASTPtr ElsePart)
		{
			Expr* cond = condition.get();
			if (!cond)
			{
				// To Do: ����
			}
			return std::make_unique<IfStatement>(start, end, std::move(condition), 
				std::move(ThenPart), std::move(ElsePart));
		}

		/// \brief Action routines about CompoundStatement.
		void Sema::ActOnCompoundStmt()
		{
			CurScope = std::make_shared<Scope>("", CurScope->getDepth() + 1, CurScope, 
				Scope::ScopeKind::SK_Block);
			ScopeStack.push_back(CurScope);
		}

		StmtASTPtr Sema::ActOnBreakStmt()
		{
			return nullptr;
		}
		
		void Sema::ActOnParmDecl(std::string name, std::shared_ptr<Type> DeclType)
		{
			// Check redefinition.
			if (CurScope->LookupName(name))
			{
				errorSema("Parameter redefinition.");
			}
			// Note: moses��FunctionDefinition�е�const�βΣ�Ĭ�϶���ͨ�����������е�ʵ�γ�ʼ����
			std::shared_ptr<VariableSymbol> vsym = std::make_shared<VariableSymbol>
				(name, CurScope, DeclType, true);
			CurScope->addDef(vsym);
			getFunctionStackTop()->addParmVariableSymbol(vsym);
		}

		void Sema::ActOnClassDeclStart(std::string name)
		{
			// check class redefinition.
			if (CurScope->LookupName(name))
			{
				errorSema("Class redefinition.");
				return;
			}
			
			// Create new scope.
			std::shared_ptr<Scope> ClassBodyScope = std::make_shared<Scope>(name, 
				CurScope->getDepth() + 1, CurScope, Scope::ScopeKind::SK_Class);

			// Create class symbol.
			std::shared_ptr<ClassSymbol> CurSym = std::make_shared<ClassSymbol>(name, CurScope, ClassBodyScope);
			ClassBodyScope->setBelongToSymbolForClassScope(CurSym);
			CurScope->addDef(CurSym);
			ClassStack.push_back(CurSym);
			ScopeStack.push_back(ClassBodyScope);
			// Update CurScope.
			CurScope = ClassBodyScope;
		}

		/// \brief Create new variable symbol.
		void Sema::ActOnVarDecl(std::string name, std::shared_ptr<Type> declType, bool isInitial)
		{
			CurScope->addDef(std::make_shared<VariableSymbol>(name, CurScope, declType, isInitial));
			// To Do: moses���õ��ǽṹ���͵ȼ۵Ľṹ����Ҫ��¼Class Type����Type
			if (ClassStack.size() != 0)
			{
				// To Do: ������������
				CurScope->getTheSymbolBelongTo()->addSubType(declType, name);
			}
		}

		/// \brief Act on declaration reference.
		/// Perferm name lookup and type checking.
		std::shared_ptr<Type> Sema::ActOnDeclRefExpr(std::string name)
		{
			if (VariableSymbol* sym = dynamic_cast<VariableSymbol*>(CurScope->Resolve(name).get()))
			{
				// ��Ȼstd::shared_ptr����ʹ��ĳ������ָ����Ϊ�����Ĺ��캯��
				// ���Ǹù��캯����explicit��ע�ģ�������Ҫʹ��static_cast<>ת��һ��
				return sym->getType();
			}
			else
			{
				errorSema("Undefined variable.");
				return nullptr;
			}
		}


		/// \brief Act on Call Expr.
		/// Perform name lookup and parm type checking.
		bool Sema::ActOnCallExpr(std::string name, std::vector<std::shared_ptr<Type>> args)
		{
			std::shared_ptr<Type> ReturnType = nullptr;
			if (FunctionSymbol* FuncSym =
				dynamic_cast<FunctionSymbol*>(ScopeStack[0]->LookupName(name).get()))
			{
				// check args number and type.
				if (args.size() != FuncSym->getParmNum())
				{
					errorSema("Arguments number not match");
					return false;
				}

				unsigned size = args.size();
				for (int i = 0; i < size; i++)
				{
					if (!args[i])
					{
						errorSema("The argument of index " + std::to_string(i) + " is wrong.");
					}
					// Note: ���������еĴ��󣬴˴����豨����continue����
					// �����������ʽ�Ķ�·��ֵ���ԣ��������(*FuncSym)[i]Ϊ�գ�ͬʱ
					if (!(*FuncSym)[i] || !((*FuncSym)[i]->getType()))
					{
						continue;
					}

					if (args[i]->getTypeFingerPrintWithNoConst() == (*FuncSym)[i]->getType()->getTypeFingerPrintWithNoConst())
					{
						continue;
					}
					else
					{
						errorSema("Arguments type not match.");
						return false;
					}						
				}
				return true;
			}
			else
			{
				errorSema("Function undefined.");
				return false;
			}
			
		}

		/// \brief Act on Binary Operator(Type checking and create new binary expr through lhs and rhs).
		/// Note: ��C/C++�У���ֵ��䷵�ص���������ֵ��
		/// ���磺'a = 10;'���ص�ֵ��10.
		ExprASTPtr Sema::ActOnBinaryOperator(SourceLocation start, SourceLocation end, 
			ExprASTPtr lhs, Token tok, ExprASTPtr rhs)
		{
			if (!lhs || !rhs)
				return nullptr;
			if (!(lhs->getType()))
			{
				errorSema("Left hand expression' type wrong.");
				return nullptr;
			}

			if (!(rhs->getType()))
			{
				errorSema("Right hand expression' type wrong.");
				return nullptr;
			}
				
			// Check expression's value kind.
			if (tok.isAssign())
			{
				if (!lhs->isLValue())
					errorSema("Left hand expression must be lvalue.");
				if (lhs->getType()->getTypeFingerPrint() != rhs->getType()->getTypeFingerPrint())
					errorSema("Type on the left and type on the right must be the same.");
				if (lhs->getType()->isConst())
					errorSema("Left hand expression is const.");
				// Shit code!��ʵָ�벻Ӧ�ñ�¶�����ģ�����C++��׼ֻ��ͨ��ָ������ʵ�ֶ�̬
				// �Լ���Ӧ��dynamic_castת����std::shared_ptr��Ȼ���ڱ�׼�⣬������Ȼ��һ��ջ�϶���
				if (const DeclRefExpr* DRE = dynamic_cast<DeclRefExpr*>(lhs.get()))
				{
					if (DRE->getType()->isConst())
					{
						// ����Ƿ��ʼ��
						if (VariableSymbol* sym = dynamic_cast<VariableSymbol*>(CurScope->Resolve(DRE->getDeclName()).get()))
						{
							if (sym->isInitial())
							{
								errorSema("Can not assign to const type.");
							}
							else
							{
								sym->setInitial(true);
							}
						}
						else
						{
							errorSema("Undefined symbol.");
						}
					}
				}
			}

			tok::TokenValue tokKind = tok.getKind();
			std::shared_ptr<Type> type = nullptr;

			// Note:����mosesֻ���������ͣ�int��bool.
			// To Do: moses��û��ʵ������ת����type cast��
			// To Do: moses��ʱ�������Զ������ͽ������������
			// Type checking.
			if (tok.isIntOperator())
			{
				if (lhs->getType()->getKind() != TypeKind::INT || rhs->getType()->getKind() != TypeKind::INT)
				{
					errorSema("Left hand expression and right hand expression must be int type.");
				}
			}

			if (tok.isBoolOperator())
			{
				if (lhs->getType()->getKind() != TypeKind::BOOL || rhs->getType()->getKind() != TypeKind::BOOL)
				{					
					errorSema("Left hand expression and right hand expression must be int type.");
				}
			}

			// �����ǰ��������������㣬��BinaryOperator��int����
			if (tok.isArithmeticOperator())
			{
				type = std::make_shared<BuiltinType>(TypeKind::INT, true);
			}

			// �����ǰ��������߼����������BinaryOperator��bool����
			if (tok.isLogicalOperator())
			{
				type = std::make_shared<BuiltinType>(TypeKind::BOOL, true);
			}

			// Note: Ϊ�˼���ƣ�BinaryOperatorĬ����rvalue
			return std::make_unique<BinaryExpr>(start, end, type, tok.getLexem(), std::move(lhs), std::move(rhs));
		}

		void Sema::PopClassStack()
		{
			ClassStack.pop_back();
		}

		void Sema::PopFunctionStack()
		{
			FunctionStack.pop_back();
		}
		
		std::shared_ptr<FunctionSymbol> Sema::getFunctionStackTop() const
		{
			if (FunctionStack.size() == 0)
			{
				errorSema("Now in top-level scope and we can't get current function symbol.");
			}
			return FunctionStack[FunctionStack.size() - 1];
		}

		std::shared_ptr<ClassSymbol> Sema::getClassStackTop() const
		{
			if (ClassStack.size() == 0)
			{
				errorSema("Now in top-level scope and we can't get current class symbol");
			}
			return ClassStack[ClassStack.size() - 1];
		}

		void Sema::PopScope()
		{
			// Pop Scope and update info.
			ScopeStack.pop_back();
			CurScope = getScopeStackTop();
		}

		/// \brief Look up name for current scope.
		std::shared_ptr<Symbol> Scope::LookupName(std::string name) 
		{

			for (auto item : SymbolTable)
			{
				if (item->getLexem() == name)
				{
					return item;
				}
			}
			return nullptr;
		}
	}
}