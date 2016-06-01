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
		/// To Do: ����һ���ǳ�����˼�Ļ��⣬��ǰ��Ϊ�ܹ��ڴ���AST���ܹ��������ĳ����۵��Ѿ�
		/// �ǳ������ˣ������ÿ�����ʽ�ṩ�ɹ��ڱ�����evaluate��ֵ
		//bool Expr::EvaluateAsRValue(EvalStatus &Result) const
		//{
		//	// ����Ӧ���ܹ��ṩһ������evaluate�İ汾
		//	// ������м򵥵�IntegerLiteral�ļ����߼򵥵�BoolLiteral���
		//	bool IsConstant;
		//	if (FastEvaluateAsRValue(this, Result, IsConstant))
		//		return IsConstant;

		//	// ���������������evaluate
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
		//		// ��ʵ�����е����࣬��Expr��type���ݳ�Ա��ֱ�ӿ��Կ���������Ϣ
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
		//		// �����޲�֮�ڣ�û��evaluating��һ��constantֵ
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
		//	// ������ʽ�����Ͳ����Ƶ���Ҳ��û���Ƶ������塣ֱ�ӷ��ء�
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

		// CheckLiteralType - ���Expr�Ƿ���LiteralType
		// ���磺 a = num + 5; ����num+5��literal type��˵�����Զ������evaluate.
		// func add() -> mytype {}
		// �û��Զ������͵ķ��أ���������literaltype(���ǽ�������ĳ�ʼ�����ʽ�ǿ����Ƶ���)
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

		/// \brief ��Ҫ�������͵ļ�������
		/// ���磺 var {num, {lhs, rhs}} �� type
		/// To Do: ������������⣬ָ�뱩¶
		bool UnpackDecl::TypeCheckingAndTypeSetting(AnonymousType* type)
		{
			unsigned size = decls.size();
			// (1) ���unpack decl��size��Anonymous type����type�Ƿ���ͬ
			if (type->getSubTypesNum() != size)
			{
				return false;
			}

			// (2) �ݹ�������ÿ��element.
			// Note: ��������������ݽ�������type�����Բ�����type��������ָ��
			// ʹ��ͬһ��ԭ��ָ���ʼ����������ָ�룬�п��ܻ��������ָ�롣
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

		/// \brief ��ȡdecl name
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

		/// \brief ��ȡdecl names
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