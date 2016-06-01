//===------------------------------ast.cpp--------------------------------===//
//
// This file is used to implement class stmt, decl.
// To Do: The semantic analyzer use syntax-directed pattern.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/ast.h"

namespace compiler
{
	namespace ast
	{
		/// EvaluateAsRValue - Return true if this is a constant which we can fold
		/// using any crazy technique(that has nothing to do with language standards)
		/// we want to. If this funciton returns true, it returns the folded constant
		/// in Result. If this expresion is a gvalue, an lvalue-to-rvalue convension
		/// will be applied to the result.		-Clang
		/// To Do: 这是一个非常有意思的话题，以前以为能够在创建AST树能够做基本的常量折叠已经
		/// 非常不错了，这里给每个表达式提供可供在编译期evaluate的值
		//bool Expr::EvaluateAsRValue(EvalStatus &Result) const
		//{
		//	// 这里应该能够提供一个快速evaluate的版本
		//	// 例如进行简单的IntegerLiteral的检查或者简单的BoolLiteral检查
		//	bool IsConstant;
		//	if (FastEvaluateAsRValue(this, Result, IsConstant))
		//		return IsConstant;

		//	// 在这里进行真正地evaluate
		//}
		//
		//bool Expr::FastEvaluateAsRValue(const Expr *Exp, Expr::EvalStatus &Result,
		//	bool &IsConst)
		//{
		//	// Fast-path evaluations of integer literals, since we sometimes see
		//	// files containing vast quantities of these. -clang
		//	if (const NumberExpr *L = dynamic_cast<const NumberExpr*>(Exp))
		//	{
		//		Result.intVal = L->getVal();
		//		// 其实这里有点冗余，从Expr的type数据成员中直接可以看出类型信息
		//		Result.kind = Expr::EvalStatus::ValueKind::IntKind;
		//		IsConst = true;
		//		return true;
		//	}

		//	if (const BoolLiteral* L = dynamic_cast<const BoolLiteral*>(Exp))
		//	{
		//		Result.boolVal = L->getVal();
		//		Result.kind = Expr::EvalStatus::ValueKind::BoolKind;
		//		return true;
		//	}			
		//	// To Do: char literal
		//	return false;
		//}

		//bool Expr::EvalInfo::nextStep()
		//{
		//	if (!StepsLeft)
		//	{
		//		// 在有限步之内，没有evaluating出一个constant值
		//		return false;
		//	}
		//	--StepsLeft;
		//	return true;
		//}

		///// EvaluateAsRValue - Try to evaluate this expression, performing an
		///// implicit lvalue-to-rvalue cast if it is an lvalue.
		//bool Expr::EvaluateAsRValue(const Expr* E, EvalInfo &Info)
		//{
		//	if (!E->getType())
		//		return false;
		//	// 如果表达式的类型不可推导，也就没有推导的意义。直接返回。
		//	if (!E->CheckLiteralType(E))
		//		return false;
		//	if ()
		//		return false;
		//	return true;
		//}

		//static bool Evaluate(const Expr *E, Expr::EvalInfo &Info)
		//{
		//
		//}

		// CheckLiteralType - 检查Expr是否是LiteralType
		// 例如： a = num + 5; 其中num+5是literal type，说明可以对其进行evaluate.
		// func add() -> mytype {}
		// 用户自定义类型的返回，不可能是literaltype(但是解包声明的初始化表达式是可以推导的)
		/*bool Expr::CheckLiteralType(const Expr* E)
		{
			return E->canBeEvaluated();
		}

		bool Expr::EvaluateAsBooleanCondition(bool &Result) const
		{
			return true;
		}

		bool Expr::EvaluateAsInt(int &Result) const
		{
			return true;
		}

		bool Expr::HasSideEffects() const
		{
			return true;
		}*/

		void CompoundStmt::addSubStmt(StmtASTPtr stmt)
		{
			if (!stmt)
				SubStmts.push_back(std::move(stmt));
		}

		/// \brief 主要进行类型的检查和设置
		/// 例如： var {num, {lhs, rhs}} 和 type
		/// To Do: 代码设计有问题，指针暴露
		bool UnpackDecl::TypeCheckingAndTypeSetting(AnonymousType* type)
		{
			unsigned size = decls.size();
			// (1) 检查unpack decl的size与Anonymous type的子type是否相同
			if (type->getSubTypesNum() != size)
			{
				return false;
			}

			// (2) 递归检查其中每个element.
			// Note: 由于这个函数传递进来的是type，所以不能以type构建智能指针
			// 使用同一个原生指针初始化两个智能指针，有可能会出现悬空指针。
			for (int index = 0; index < size; index++)
			{
				if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
				{
					if (AnonymousType* anonyt = dynamic_cast<AnonymousType*>(type->getSubType(index).get()))
					{
						return unpackd->TypeCheckingAndTypeSetting(anonyt);
					}
					else
					{
						return false;
					}					
				}
				else
				{
					if (AnonymousType* anonyt = dynamic_cast<AnonymousType*>(type->getSubType(index).get()))
					{
						return false;
					}
				}
			}
			return true;
		}

		/// \brief 获取decl name
		std::vector<std::string> UnpackDecl::operator[](unsigned index) const
		{
			std::vector<std::string> names;
			if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
			{
				unpackd->getDeclNames(names);
			}
			
			if (VarDecl* var = dynamic_cast<VarDecl*>(decls[index].get()))
			{
				names.push_back(var->getName());
			}
			return names;
		}

		/// \brief 获取decl names
		void UnpackDecl::getDeclNames(std::vector<std::string>& names) const
		{
			unsigned size = decls.size();
			for (int index = 0; index < size; index++)
			{
				if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
				{
					unpackd->getDeclNames(names);
				}

				if (VarDecl* var = dynamic_cast<VarDecl*>(decls[index].get()))
				{
					names.push_back(var->getName());
				}
			}
		}

		void UnpackDecl::setCorrespondingType(std::shared_ptr<Type> type)
		{
			this->type = type;
		}
	}
}