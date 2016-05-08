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
				errorReport("Function redefinition.");
			}

			// Create new scope for function.
			unsigned CurDepth = CurScope->getDepth();
			std::shared_ptr<Scope> funcScope = std::make_shared<Scope>(name, CurDepth + 1,
				CurScope, Scope::ScopeKind::SK_Function);

			// Record this func symbol.
			std::shared_ptr<FunctionSymbol> funcsym = 
				std::make_shared<FunctionSymbol>(name, nullptr, CurScope, funcScope);

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
		StmtASTPtr Sema::ActOnIfStmt(SourceLocation start, SourceLocation end, 
			ExprASTPtr condition, StmtASTPtr ThenPart, StmtASTPtr ElsePart)
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

		/// \brief Actions routines about unpack decl.
		/// var {num, mem} = anony;
		/// Mainly check redefintion.
		bool Sema::ActOnUnpackDeclElement(std::string name)
		{
			// 在当前CurScope中发现同名变量
			if (CurScope->LookupName(name))
			{
				errorReport("Error occured in unpack decl.  Variable " + name + " redefinition.");
				return false;
			}
			return true;
		}

		/// \brief Mainly check whether current context is while context.
		bool Sema::ActOnBreakAndContinueStmt(bool whileContext)
		{
			if (whileContext)
			{
				errorReport("Current context is not loop-context.");
			}
			return whileContext;
		}

		/// \brief Mainly current whether identifier is user defined type.
		std::shared_ptr<Type> Sema::ActOnReturnType(const std::string& name) const
		{
			// Note: 在moses中，class暂时定义在Top-Level中
			if (ClassSymbol* sym = dynamic_cast<ClassSymbol*>(ScopeStack[0]->LookupName(name).get()))
			{
				return sym->getType();
			}
			else
			{
				errorReport("Current identifier isn't user defined type.");
				return nullptr;
			}
		}

		void Sema::ActOnParmDecl(std::string name, std::shared_ptr<Type> DeclType)
		{
			// Check redefinition.
			if (CurScope->LookupName(name))
			{
				errorReport("Parameter redefinition.");

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
				errorReport("Class redefinition.");
				return;
			}

			// Create new scope.
			std::shared_ptr<Scope> ClassBodyScope = std::make_shared<Scope>(name,
				CurScope->getDepth() + 1, CurScope, Scope::ScopeKind::SK_Class);

			// Create class symbol.
			std::shared_ptr<ClassSymbol> CurSym = 
				std::make_shared<ClassSymbol>(name, CurScope, ClassBodyScope);

			ClassBodyScope->setBelongToSymbolForClassScope(CurSym);
			CurScope->addDef(CurSym);
			ClassStack.push_back(CurSym);
			ScopeStack.push_back(ClassBodyScope);
			// Update CurScope.
			CurScope = ClassBodyScope;
		}

		/// \brief Create new variable symbol.
		ExprASTPtr Sema::ActOnVarDecl(std::string name, std::shared_ptr<Type> declType, ExprASTPtr InitExpr)
		{
			CurScope->addDef(std::make_shared<VariableSymbol>(name, CurScope,
				declType ? declType : InitExpr->getType(), declType ? false : true));

			// To Do: moses采用的是结构类型等价的结构，需要记录Class Type的子Type
			if (ClassStack.size() != 0)
			{
				// moses暂时在定义class中，不能给Member数据成员InitExpr.
				if (InitExpr)
				{
					errorReport("Member declaration can't have initial expression");
				}
				CurScope->getTheSymbolBelongTo()->addSubType(declType, name);
			}
			return std::move(InitExpr);
		}

		/// \brief 对reutrn anonymous的情况进行语义检查
		bool Sema::ActOnReturnAnonymous(std::shared_ptr<Type> type) const
		{
			if (!type)
			{
				return false;
			}
			// 无需报错，前面在解析function以及return type的时候出了问题，才会导致为空
			// 前面已经报错，无需重复报错
			if (!getFunctionStackTop() || !(getFunctionStackTop()->getReturnType()))
			{
				return false;
			}
			if (getFunctionStackTop()->getReturnType()->getKind() != TypeKind::ANONYMOUS)
			{
				errorReport("Current function's return type isn't anonymous.");
				return false;
			}

			if (type->getTypeFingerPrintWithNoConst() != 
				getFunctionStackTop()->getReturnType()->getTypeFingerPrintWithNoConst())
			{
				errorReport("Return type not match.");
				return false;
			}
			return true;
		}

		bool Sema::ActOnReturnStmt(std::shared_ptr<Type> type) const
		{
			// To Do: 代码效率较低，有的函数需要调用两次
			if (!getFunctionStackTop() || !(getFunctionStackTop()->getReturnType()))
			{
				return false;
			}
			// 判断类型是否相同
			// Note: moses暂时不支持const返回类型
			if (type->getTypeFingerPrintWithNoConst() 
				!= getFunctionStackTop()->getReturnType()->getTypeFingerPrintWithNoConst())
			{
				errorReport("Return type not match.");
				return false;
			}
			return true;
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
				errorReport("Undefined variable.");
				return nullptr;
			}
		}


		/// \brief Act on Call Expr.
		/// Perform name lookup and parm type checking.
		std::shared_ptr<Type> Sema::ActOnCallExpr(std::string name, 
			std::vector<std::shared_ptr<Type>> args)
		{
			std::shared_ptr<Type> ReturnType = nullptr;
			if (FunctionSymbol* FuncSym =
				dynamic_cast<FunctionSymbol*>(ScopeStack[0]->LookupName(name).get()))
			{
				ReturnType = FuncSym->getReturnType();
				// check args number and type.
				if (args.size() != FuncSym->getParmNum())
				{
					errorReport("Arguments number not match.");
				}

				unsigned size = args.size();
				for (int i = 0; i < size; i++)
				{
					if (!args[i])
					{
						errorReport("The argument of index " + std::to_string(i) + " is wrong.");
						continue;
					}
					// Note: 函数定义中的错误，此处不予报出，continue即可
					// 由于条件表达式的短路求值特性，不会出现(*FuncSym)[i]为空，同时
					if (!(*FuncSym)[i] || !((*FuncSym)[i]->getType()))
					{
						continue;
					}

					if (args[i]->getTypeFingerPrintWithNoConst() == 
						(*FuncSym)[i]->getType()->getTypeFingerPrintWithNoConst())
					{
						continue;
					}
					else
					{
						errorReport("Arguments type not match.");
					}
				}
			}
			else
			{
				errorReport("Function undefined.");
			}
			return ReturnType;
		}

		/// \brief Act on Binary Operator(Type checking and create new binary expr through lhs and rhs).
		/// Note: 在C/C++中，赋值语句返回的是所赋的值。
		/// 例如：'a = 10;'返回的值是10.
		ExprASTPtr Sema::ActOnBinaryOperator(ExprASTPtr lhs, Token tok, ExprASTPtr rhs)
		{
			if (!lhs || !rhs)
				return nullptr;
			if (!(lhs->getType()))
			{
				errorReport("Left hand expression' type wrong.");
				return nullptr;
			}

			if (!(rhs->getType()))
			{
				errorReport("Right hand expression' type wrong.");
				return nullptr;
			}

			// Check expression's value kind.
			if (tok.isAssign())
			{
				if (!lhs->isLValue())
					errorReport("Left hand expression must be lvalue.");
				if (lhs->getType()->getTypeFingerPrintWithNoConst() != 
					rhs->getType()->getTypeFingerPrintWithNoConst())
					errorReport("Type on the left and type on the right must be the same.");
				// Shit code!其实指针不应该暴露出来的，但是C++标准只能通过指针类型实现多态
				// 以及相应的dynamic_cast转换，std::shared_ptr虽然属于标准库，但是仍然是一个栈上对象
				if (const DeclRefExpr* DRE = dynamic_cast<DeclRefExpr*>(lhs.get()))
				{
					if (DRE->getType()->isConst())
					{
						// 检查是否初始化
						if (VariableSymbol* sym = 
							dynamic_cast<VariableSymbol*>(CurScope->Resolve(DRE->getDeclName()).get()))
						{
							if (sym->isInitial())
							{
								errorReport("Can not assign to const type.");
							}
							else
							{
								sym->setInitial(true);
							}
						}
						else
						{
							errorReport("Undefined symbol.");
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
				if (lhs->getType()->getKind() != TypeKind::INT || 
					rhs->getType()->getKind() != TypeKind::INT)
				{
					errorReport("Left hand expression and right hand expression must be int type.");
				}
			}

			if (tok.isBoolOperator())
			{
				if (lhs->getType()->getKind() != TypeKind::BOOL || 
					rhs->getType()->getKind() != TypeKind::BOOL)
				{
					errorReport("Left hand expression and right hand expression must be int type.");
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
			return std::make_unique<BinaryExpr>(lhs->getLocStart(), rhs->getLocEnd(), type, tok.getLexem(), 
				std::move(lhs), std::move(rhs));
		}

		/// \brief Perform simple semantic analysis for member access expr.
		/// Like:
		///		class A
		///		{
		///			var num : int;
		///		}
		///		var base : A;
		///		base.num = 10;
		ExprASTPtr Sema::ActOnMemberAccessExpr(ExprASTPtr lhs, Token tok)
		{
			/// (1) Check LHS
			/// LHS表示的是对象部分，对象部分大部分情况下都是DeclRefExpr
			/// 但也有很大情况不是DeclRefExpr，也有可能是函数调用表达式。
			/// 例如：
			/// func add() -> A {}
			/// 由于moses中自定义类型采用的是引用类型，也就是说自定义类型不是在栈上分配的
			/// 而是使用moses自己的内存管理机制进行统一管理的，然后适时地进行垃圾回收。
			/// 所以只需要检查LHS部分的类型是否为用户自定义类型即可。
			if (!(lhs->getType()) || lhs->getType()->getKind() != TypeKind::USERDEFIED)
			{
				errorReport("Type error. Expect user defined type.");
			}

			std::shared_ptr<Type> memberType = nullptr;
			/// (2) Check Member name.
			/// 这里需要检查该用户自定义类型是否有这个成员名，存在的话，同时获取该数据成员的类型
			/// Note: 这里将UserDefinedType*指针暴露出来是不符合规范的，但是只有在原生态指针的
			/// 情况下才能进行多态的转换。
			if (UserDefinedType* BaseType = dynamic_cast<UserDefinedType*>(lhs->getType().get()))
			{
				if (!(BaseType->HaveMember(tok.getLexem())))
				{
					errorReport("Type " + BaseType->getTypeName() + " have no member " + tok.getLexem());
					return nullptr;
				}
				memberType = BaseType->getMemberType(tok.getLexem());
			}
			return std::make_unique<MemberExpr>(lhs->getLocStart(), lhs->getLocEnd(), memberType,
				std::move(lhs), tok.getTokenLoc(), tok.getLexem());
		}

		ExprASTPtr Sema::ActOnDecOrIncExpr(ExprASTPtr rhs)
		{
			// (1) rhs 必须是int类型
			if (rhs->getType()->getKind() != TypeKind::INT)
			{
				errorReport("Operator '++' '--' need operand is int type.");
			}

			// (2) rhs 必须是左值
			// Note: moses中只有左值和右值之分，没有什么xrvalue和prvalue之分。
			if (rhs->isRValue())
			{
				errorReport("Operator '++' '--' need operand is lvalue");
			}
			return std::move(rhs);
		}

		ExprASTPtr Sema::ActOnUnaryExclamatoryExpr(ExprASTPtr rhs)
		{
			// 由于moses中用户自定义类型不能重载运算符，所以取非运算只能用于bool类型
			if (rhs->getType()->getKind() != TypeKind::BOOL)
			{
				errorReport("Operator '!' need operand is bool type.");
			}
			return std::move(rhs);
		}

		/// \brief 用于对单目求负值运算进行检查
		/// To Do: 注意这里int值的边界并没有限定，暂时与C语言中的int型相同16位(但是现在其实都是32位数的)
		ExprASTPtr Sema::ActOnUnarySubExpr(ExprASTPtr rhs)
		{
			if (rhs->getType()->getKind() != TypeKind::INT)
			{
				errorReport("Operator '-' need operand is int type");
			}
			return std::move(rhs);
		}

		/// \brief 对unpack decl进行语义分析，主要是类型检查
		/// 例如： var {num, {flag, lhs, rhs}} = anony;
		/// Shit code!
		UnpackDeclPtr Sema::ActOnUnpackDecl(UnpackDeclPtr unpackDecl, std::shared_ptr<Type> type)
		{
			if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(unpackDecl.get()))
			{
				if (!type)
				{
					errorReport("Unapck declaration's initial expression type error.");
				}
				if (type->getKind() != TypeKind::ANONYMOUS)
				{
					errorReport("Unpack declaration's initial expression must be anonymous type.");
					return nullptr;
				}

				return unpackDeclTypeChecking(std::move(unpackDecl), type);
			}
			errorReport("Left hand expression isn's unpack declaration.");
			// To Do: 此处直接返回nullptr太过激进，需要更合适的处理方式。
			return nullptr;
		}

		/// \brief 
		BinaryPtr Sema::ActOnAnonymousTypeVariableAssignment(ExprASTPtr lhs, ExprASTPtr rhs) const
		{
			if (DeclRefExpr* DRE = dynamic_cast<DeclRefExpr*>(lhs.get()))
			{
				// Type Checking.
				if (DRE->getType()->getTypeFingerPrintWithNoConst() != 
					rhs->getType()->getTypeFingerPrintWithNoConst())
				{
					errorReport("Type error occured in anonymous type variable assigning.");
				}
			}
			else
			{
				errorReport("Error occured in anonymous type variable assigning.");
			}
			return std::make_unique<BinaryExpr>(lhs->getLocStart(), rhs->getLocEnd(), 
				lhs->getType(), "=", std::move(lhs), std::move(rhs));
		}

		bool Sema::ActOnConditionExpr(std::shared_ptr<Type> type) const
		{
			if (!type || type->getKind() != TypeKind::BOOL)
			{
				errorReport("Conditional expression is not a boolean type.");
				return false;
			}
			return true;
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
				errorReport("Now in top-level scope and we can't get current function symbol.");
			}
			return FunctionStack[FunctionStack.size() - 1];
		}

		std::shared_ptr<ClassSymbol> Sema::getClassStackTop() const
		{
			if (ClassStack.size() == 0)
			{
				errorReport("Now in top-level scope and we can't get current class symbol");
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

		std::shared_ptr<Scope> Sema::getScopeStackTop() const
		{
			if (ScopeStack.size() == 0)
			{
				errorReport("Now in top-level scope and we can't get current scope.");
				return nullptr;
			}
			return ScopeStack[ScopeStack.size() - 1];
		}

		/// \brief 该方法主要用于进行类型检查，并设置其类型
		UnpackDeclPtr Sema::unpackDeclTypeChecking(UnpackDeclPtr decl, std::shared_ptr<Type> initType) const
		{
			if (AnonymousType* anonyt = dynamic_cast<AnonymousType*>(initType.get()))
			{
				// (1) 检查其中的每个类型是否相容
				if (!(decl->TypeCheckingAndTypeSetting(anonyt)))
				{
					errorReport("Unpack declaration type error.");
					return nullptr;
				}

				// Note: 其实不需要type设置，因为最重要的是symbol table.
				decl->setCorrespondingType(initType);

				// (2) 创建symbol
				// Note: 现在这个操作主要是将unpack decl分拆开来
				// 例如： {{start, end}, {lhs, rhs}} = num;
				// 拆出来后是start, end, lhs, rhs
				// 1: 收集decl names
				std::vector<std::string> unpackDeclNames;
				decl->getDeclNames(unpackDeclNames);

				// 2: 收集types
				std::vector<std::shared_ptr<Type>> types;
				anonyt->getTypes(types);

				// 3: check
				if (unpackDeclNames.size() != types.size())
				{
					errorReport("Unpack declaration type error.");
				}
				// 3: 创建symbol
				unsigned size = unpackDeclNames.size();
				for (int index = 0; index < size; index++)
				{
					// (1): 创建variable symbol并插入当前scope
					CurScope->addDef(std::make_shared<VariableSymbol>(unpackDeclNames[index], 
						CurScope, types[index], true));
				}
			}
			else
			{
				errorReport("Unpack declaration type error.");
				return nullptr;
			}
			return std::move(decl);
		}
		/// \brief 用于sema的报错
		void Sema::errorReport(const std::string& msg) const
		{
			errorSema(scan->getLastToken().getTokenLoc().toString() + " --- " + msg);
		}
	}
}