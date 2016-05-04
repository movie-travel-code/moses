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

		/// \biref 这个函数主要用于在文件头创建Top-Level Scope.
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
				// To Do: 报错
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
			// Note: moses中FunctionDefinition中的const形参，默认都是通过函数调用中的实参初始化的
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
			// To Do: moses采用的是结构类型等价的结构，需要记录Class Type的子Type
			if (ClassStack.size() != 0)
			{
				// To Do: 代码冗余消除
				CurScope->getTheSymbolBelongTo()->addSubType(declType, name);
			}
		}

		/// \brief Act on declaration reference.
		/// Perferm name lookup and type checking.
		std::shared_ptr<Type> Sema::ActOnDeclRefExpr(std::string name)
		{
			if (VariableSymbol* sym = dynamic_cast<VariableSymbol*>(CurScope->Resolve(name).get()))
			{
				// 虽然std::shared_ptr中有使用某种类型指针作为参数的构造函数
				// 但是该构造函数是explicit标注的，所以需要使用static_cast<>转换一下
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
					// Note: 函数定义中的错误，此处不予报出，continue即可
					// 由于条件表达式的短路求值特性，不会出现(*FuncSym)[i]为空，同时
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
		/// Note: 在C/C++中，赋值语句返回的是所赋的值。
		/// 例如：'a = 10;'返回的值是10.
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
				// Shit code!其实指针不应该暴露出来的，但是C++标准只能通过指针类型实现多态
				// 以及相应的dynamic_cast转换，std::shared_ptr虽然属于标准库，但是仍然是一个栈上对象
				if (const DeclRefExpr* DRE = dynamic_cast<DeclRefExpr*>(lhs.get()))
				{
					if (DRE->getType()->isConst())
					{
						// 检查是否初始化
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

			// Note:现在moses只有两种类型，int和bool.
			// To Do: moses还没有实现类型转换（type cast）
			// To Do: moses暂时不允许自定义类型进行运算符重载
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

			// 如果当前运算符是算术运算，则BinaryOperator是int类型
			if (tok.isArithmeticOperator())
			{
				type = std::make_shared<BuiltinType>(TypeKind::INT, true);
			}

			// 如果当前运算符是逻辑运算符，则BinaryOperator是bool类型
			if (tok.isLogicalOperator())
			{
				type = std::make_shared<BuiltinType>(TypeKind::BOOL, true);
			}

			// Note: 为了简化设计，BinaryOperator默认是rvalue
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