//===---------------------------------ast.h---------------------------------===//
// 
// This file contains the declaration of the statement, decl, expr class.
// llvm source code farmatting:http://llvm.org/docs/CodingStandards.html
// 
//===-----------------------------------------------------------------------===//

#ifndef AST_INCLUDE
#define AST_INCLUDE
#include <string>
#include <vector>
#include <memory>
#include "../Support/SourceLocation.h"
#include "Type.h"

namespace compiler
{
	///---------------------------nonsense for coding------------------------------///
	/// syntax tree����parse���֮�󹹽��ģ�����moses��������ʵ��one-pass��compile
	/// Ҳ����˵�ڹ���syntax tree��ʱ��ֻ��ִ��һЩ�򵥵�������������洢���󡣹������
	/// syntax tree���ٱ���syntax tree��ִ�д������ɲ��֡�
	///---------------------------nonsense for coding------------------------------///

	//namespace visitor
	//{
	//	// The visitor pattern achieves a form of double dispath for languages
	//	// that offer only single dispatch. The visitor pattern enables phase- 
	//	// and node-specific code to be invoked clealy.
	//	// Every phase extends the Visitor class, so that it inherits the
	//	// visit(AbstractNode n) method.

	//	// Note: Wile the inclusion of the Accept method in every node class 
	//	// seems redundant, it cannot be factored into a common superclass, 
	//	// because the type of this must be specific to the visited node.

	//	// --------------------visitor code as below---------------------
	//	/*class Visitor
	//	procedure visit(AbstractNode n)
	//	n.accept()
	//	end
	//	end

	//	class TypeChecking extends Visitor
	//	procedure visit(IfNode n)
	//	end

	//	procedure visit(PlusNode n)
	//	end

	//	procedure visit(MinusNode m)
	//	end

	//	end

	//	class IFNode extends AbstractNode
	//	procedure accept(Visitor v)
	//	v.visit(this)
	//	end
	//	end

	//	class PlusNode externds AbstractNode
	//	procedure accept(Visitor v)
	//	v.visit(this)
	//	end
	//	...
	//	end

	//	class MinusNode extends AbstractNode
	//	procedure accept(Visitor v)
	//	v.visit(this)
	//	end
	//	...
	//	end
	//	*/
	//	// --------------------------------------------------------------
	//	class Visitor
	//	{
	//	public:
	//		virtual void visit(ast::StatementAST* root)
	//		{
	//			root->Accept(Visitor*(this));
	//		}

	//		void func() {}
	//	};
	//}

	namespace ast
	{
		using namespace compiler::lex;

		class Expr;
		class NumberExpr;
		class BinaryExpr;
		class CallExpr;
		class DeclStatement;
		class StatementAST;
		class ParameterDecl;
		class FunctionDecl;
		class CompoundStmt;
		class VarDecl;	
		class UnpackDecl;

		using ASTPtr = std::vector<std::unique_ptr<StatementAST>>;
		using StmtASTPtr = std::unique_ptr<StatementAST>;
		using ExprASTPtr = std::unique_ptr<Expr>;
		using CallExprPtr = std::unique_ptr<CallExpr>;
		using DeclASTPtr = std::unique_ptr<DeclStatement>;
		using CompoundStmtPtr = std::unique_ptr<CompoundStmt>;
		using ParmDeclPtr = std::unique_ptr<ParameterDecl>;
		using UnpackDeclPtr = std::unique_ptr<UnpackDecl>;
		using BinaryPtr = std::unique_ptr<BinaryExpr>;

		/// \brief StatementAST - Base class for all statements.
		//--------------------------nonsense for coding---------------------------
		// ����ʹ��std::unique_ptr������AST������Щ���ߣ���ΪIR���ɵ�ʱ��ֱ�ӽ�
		// AST��������������ֱ����AST�������� CodeGen() ������һ�߱���һ�����ɴ��룩��
		//--------------------------nonsense for coding---------------------------
		class StatementAST
		{
		public:
			// Visitor Pattern��ʹ��override(�麯��) + overload(����) ����double dispatch.
			// ���й��ڱ������壨Ҳ����AST node�����֣�ͨ��override�� Accept()�ķ�����ʵ��
			// ��һ��dispatch����һ��dispatchͨ��override�ķ�ʽ�õ�����Ľڵ����͡�
			// �ڶ���dispatchʹ������ʵ�֣���IRbuilder����
			class Visitor
			{
			public:
				virtual void visit(const ast::StatementAST* root)
				{
					root->Accept(this);
				}

				void func() {}
			};

		protected:
			SourceLocation LocStart;
			SourceLocation LocEnd;
		public:
			StatementAST() {}

			StatementAST(SourceLocation start, SourceLocation end) : LocStart(start), LocEnd(end)
			{}

			StatementAST(const StatementAST& stmt) : LocStart(stmt.LocStart),
				LocEnd(stmt.LocEnd) {}

			virtual ~StatementAST() {}

			// We use visitor pattern for IR build(Note: Because we use ad-hoc translation
			// , semantic analysis doesn't use vistor pattern).
			// When a node accepts a visitor, actions are performed that are appropriate to
			// both the visitor and the node at hand(double dispatch).
			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}

			SourceLocation getLocStart() const { return LocStart; }
			SourceLocation getLocEnd() const { return LocEnd; }
			void setLocStart(unsigned long line, unsigned long number) {}
			void setLocEnd(unsigned long line, unsigned long number) {}
			// virtual 
		};

		/// @brief ExprAST - Base class for all expression nodes.
		/// Note that Expr's are subclass of Stmt. This allows an expression
		/// to be transparently used any place a Stmt is required.
		class Expr : public StatementAST
		{
		public:
			/// \brief The categorization of expression values.
			enum class ExprValueKind
			{
				/// \brief An r-value expression produces a temporary value.
				/// Note: lvalue can convert to rvalue, so VK_LValue implication VK_RValue.
				VK_RValue,
				VK_LValue
			};
			
		private:
			// ------------------------nonsense for coding---------------------------
			// Expr��������Ӧ��Type���������Ҫ�����ڽ������ͼ�顣
			// ------------------------nonsense for coding---------------------------
			std::shared_ptr<Type> ExprType;
			ExprValueKind VK;
			// ���ڱ�ʶ��ǰExpr�Ƿ��ǿ��Ƶ���constant��
			// ���磺 
			// a = num * 5; /* num * 5 �ǿ��Ƶ��� */
			// var num = {10, num}; /* {10, num} �ǿ��Ƶ��� */
			// ��moses��Ĭ���û��Զ��������ǲ����Ƶ��ġ�
			bool CanBeEvaluated;
		public:
			Expr() : ExprType(nullptr), CanBeEvaluated(false) {}
			Expr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
				ExprValueKind vk, bool canDoEvaluate) :
				StatementAST(start, end), ExprType(type), VK(vk), 
				CanBeEvaluated(canDoEvaluate) 
			{}

			Expr(const Expr& expr) : 
				StatementAST(expr.LocStart, expr.LocEnd), ExprType(expr.ExprType), 
				CanBeEvaluated(expr.CanBeEvaluated) {}

			std::shared_ptr<Type> getType() const { return ExprType; }

			/// isLvalue - True if this expression is an "l-value" according to
			/// the rules of the current language. Like C/C++��moses give somewhat
			/// different rules for this concept, but in general, the result of 
			/// an l-value expression identifies a specific object whereas the
			/// result of an r-value expression is a value detached from any 
			/// specific storage.
			bool isLValue() const { return VK == ExprValueKind::VK_LValue; }
			bool isRValue() const { return VK == ExprValueKind::VK_RValue; }

			void setCanBeEvaluated() { CanBeEvaluated = true; }
			bool canBeEvaluated() const { return CanBeEvaluated; }			

			void setExprValueKind(ExprValueKind valueKind) { VK = valueKind; }

			virtual ~Expr() {}

			// ---------------------nonsense for coding-------------------------
			// ע������accept�븸�����Ȳ��Ƕ��࣬��ͨ��this������������ʵ�ֶ�̬��
			// -----------------------------------------------------------------
			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief NumberExpr - ������ʾnumeric literal, ����"1".
		class NumberExpr : public Expr
		{
			double Val;
		public:
			NumberExpr(SourceLocation start, SourceLocation end, double Val) :
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::INT, true), 
				ExprValueKind::VK_RValue, true), Val(Val) {}
			virtual ~NumberExpr() {}

			double getVal() const { return Val; }

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief CharExpr - ������ʾchar literal, ����"c"
		class CharExpr : public Expr
		{
			std::string C;
		public:
			/// To Do: �˴�ʹ��INT����ʾCharExpr��Ҳ�������߿�����Ӽ�
			CharExpr(SourceLocation start, SourceLocation end, std::string c) : 
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::INT, true), 
				ExprValueKind::VK_RValue, true), C(c) 
			{}

			virtual ~CharExpr() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief StringLiteral - ������ʾstring.
		class StringLiteral : public Expr
		{
			std::string str;
		public:
			StringLiteral(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
						std::string str) : 
				Expr(start, end, type, ExprValueKind::VK_RValue, true), str(str) {}
			virtual ~StringLiteral() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief BoolLiteral - ������ʾbool����ֵ.
		class BoolLiteral : public Expr
		{		
			bool value;
		public:
			BoolLiteral(SourceLocation start, SourceLocation end, bool value) :
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::BOOL, true), 
				ExprValueKind::VK_RValue, true), value(value) {}

			bool getVal() const
			{
				return value;
			}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief DeclRefExprAST - A reference to a declared variable, function, enum, etc.
		/// This encodes all the information about how a declaration is referenced within an 
		/// expression.
		///-------------------------------nonsense for coding-------------------------------
		/// moses IR��һ�����㣨Ӧ��˵��bug�������﷨����û��ֱ�����ֳ�����DeclRefExpr���Ҳ��
		/// DeclRefExpr��������Clang dump�����﷨������Ϊ��ֵ���ʽ��DeclRefExpr�ᱻһ����ֵת
		/// ��ֵ����ʽת�����Ͱ�����
		///---------------------------------------------------------------------------------
		class DeclRefExpr final : public Expr
		{
			std::string Name;
			const VarDecl* VD;
		public:
			// Note: һ������£�DeclRefExpr������ֵ�ģ�������һ��������⣬���Ǻ������ֵ���
			// ���Ǻ������ö�Ӧ��expression��CallExpr������DeclRefExpr.
			// To Do: �п��ܻ���Ǳ�ڵ�bug
			DeclRefExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
					std::string name, const VarDecl* vd) : 
				Expr(start, end, type, ExprValueKind::VK_LValue, true), Name(name), VD(vd) 
			{}

			std::string getDeclName() const { return Name; }

			const VarDecl* getDecl() const { return VD; }

			virtual ~DeclRefExpr() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief BinaryExpr - Expression class for a binary operator.
		class BinaryExpr : public Expr
		{
			std::string OpName;
			ExprASTPtr LHS, RHS;
		public:
			// Note: ��moses�в�����ָ�����ͣ����Բ�����binaryΪlvalue�����
			// ����: 'int* p = &num;'
			// 'p + 1'�Ϳ�����Ϊ��ֵ
			// Note: BinaryExpr���ǿ��Խ���evaluate��
			BinaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
						std::string Op, ExprASTPtr LHS, ExprASTPtr RHS) :
				Expr(start, end, type, ExprValueKind::VK_RValue, true), OpName(Op), 
				LHS(std::move(LHS)), RHS(std::move(RHS)) {}

			const Expr* getLHS() const { return LHS.get(); }

			const Expr* getRHS() const { return RHS.get(); }

			std::string getOpcode() const { return OpName; }

			virtual ~BinaryExpr() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief UnaryExpr - Expression class for a unary operator.
		class UnaryExpr : public Expr
		{
			std::string OpName;
			ExprASTPtr SubExpr;
		public:
			UnaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, std::string Op,
				ExprASTPtr subExpr, ExprValueKind vk) :
				Expr(start, end, type, vk, true), OpName(Op), SubExpr(std::move(SubExpr)) {}

			const Expr* getSubExpr() const { return SubExpr.get(); }

			std::string getOpcode() const { return OpName; }

			void setOpcode(std::string name) { OpName = name; }

			virtual ~UnaryExpr() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief CallExprAST - Expression class for function calls.
		class CallExpr : public Expr
		{
			std::string CalleeName;
			// To Do: FunctionDecl*
			const FunctionDecl* FuncDecl;
			std::vector<std::unique_ptr<Expr> > Args;
		public:
			CallExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
					const std::string& Callee, std::vector<std::unique_ptr<Expr>> Args, 
					ExprValueKind vk, const FunctionDecl* FD, bool canDoEvaluate) : 
				Expr(start, end, type, vk, canDoEvaluate), CalleeName(Callee), Args(std::move(Args)), 
				FuncDecl(FD) 
			{}

			unsigned getArgsNum() const { return Args.size(); }

			const FunctionDecl* getFuncDecl() const { return FuncDecl; }

			// Note: ��������û�жԴ����index���м��
			const Expr* getArg(unsigned index) const { return Args[index].get(); }

			virtual ~CallExpr() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief MemberExpr - Class members. X.F.
		class MemberExpr final : public Expr
		{
			/// Base - the expression for the base pointer or structure references. In
			/// X.F, this is "X". ��DeclRefExpr
			StmtASTPtr Base;

			/// name - member name
			std::string name;

			/// MemberLoc - This is the location of the member name.
			SourceLocation MemberLoc;

			/// This is the location of the -> or . in the expression.
			SourceLocation OperatorLoc;
		public:
			MemberExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
					std::unique_ptr<Expr> base, SourceLocation operatorloc, std::string name, 
					bool canDoEvaluate) :
				Expr(start, end, type, ExprValueKind::VK_LValue, canDoEvaluate), 
				Base(std::move(base)), OperatorLoc(operatorloc), name(name) 
			{}

			void setBase(ExprASTPtr E) { Base = std::move(E); }

			std::string getMemberName() const { return name; }

			ExprASTPtr getBase() const { return nullptr; }

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};


		/// \brief �������ͳ�ʼ�����ʽ��
		///	var start = 0;
		/// var end = 1;
		/// ���磺 var num = {start, end};
		/// 
		/// �����������û��Զ������͵ĳ�ʼ������������ķ�ʽ���и�ֵ��
		class AnonymousInitExpr final : public Expr
		{
			AnonymousInitExpr() = delete;
			AnonymousInitExpr(const AnonymousInitExpr&) = delete;
			std::vector<ExprASTPtr> InitExprs;
		public:
			AnonymousInitExpr(SourceLocation start, SourceLocation end, 
					std::vector<ExprASTPtr> initExprs, std::shared_ptr<Type> type):
				Expr(start, end, type, Expr::ExprValueKind::VK_RValue, true), 
				InitExprs(std::move(initExprs)) 
			{}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief CompoundStatement - This class represents a Compound Statement
		/// ------------------------------------------
		/// Compound Statement's Grammar as below.
		/// compound-statement -> "{" statement* "}"
		/// ------------------------------------------
		class CompoundStmt : public StatementAST
		{
			// The sub-statements of the compound statement.
			std::vector<StmtASTPtr> SubStmts;
		public:
			CompoundStmt(SourceLocation start, SourceLocation end,
				std::vector<StmtASTPtr> subStmts) :
				StatementAST(start, end), SubStmts(std::move(subStmts)) {}

			virtual ~CompoundStmt() {}

			void addSubStmt(StmtASTPtr stmt);

			const StatementAST* getSubStmt(unsigned index) const;

			const StatementAST* operator[](unsigned index) const;

			unsigned getSize() const 
			{
				return SubStmts.size();
			}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief IfStatementAST - This class represents a If Statement
		/// ----------------------------------------------------------------------------
		/// If Statement's Grammar as below.
		/// if-statement -> "if" expression compound-statement "else" compound-statement
		/// ----------------------------------------------------------------------------
		class IfStatement : public StatementAST
		{
			std::unique_ptr<Expr> Condition;
			std::unique_ptr<StatementAST> Then;
			std::unique_ptr<StatementAST> Else;
		public:
			IfStatement(SourceLocation start, SourceLocation end,
				std::unique_ptr<Expr> Condition,
				std::unique_ptr<StatementAST> Then,
				std::unique_ptr<StatementAST> Else) : StatementAST(start, end),
				Condition(std::move(Condition)), Then(std::move(Then)), Else(std::move(Else)) {}

			virtual ~IfStatement() {}

			// To Do: �������ǲ����ʵģ�Ӧ��ʱ�̶�ʹ��unique_ptr�����ݸ����ڵ�
			// �����������Ļ���̫���ڸ���
			const StatementAST* getThen() const
			{
				return Then.get();
			}

			const StatementAST* getElse() const
			{
				return Else.get();
			}

			const Expr* getCondition() const
			{
				return Condition.get();
			}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief WhileStatementAST - This class represents a While Statement
		/// ---------------------------------------------------------
		/// While Statement's Grammar as below.
		/// while-statement -> "while" expression compound-statement
		/// ---------------------------------------------------------
		class WhileStatement : public StatementAST
		{
			std::unique_ptr<Expr> Condition;
			std::unique_ptr<StatementAST> WhileBody;
		public:
			WhileStatement(SourceLocation start, SourceLocation end,
				std::unique_ptr<Expr> condition,
				std::unique_ptr<StatementAST> body) :StatementAST(start, end),
				Condition(std::move(condition)), WhileBody(std::move(body)) {}
			virtual ~WhileStatement() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief BreakStatementAST - This class represents a Break Statement
		/// ---------------------------------------
		/// Break Statement's Grammar as below.
		/// break-statement -> "break" ";"
		/// ---------------------------------------
		class BreakStatement : public StatementAST
		{
		public:
			BreakStatement(SourceLocation start, SourceLocation end) : StatementAST(start, end) {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief ContinueStatementAST - This class represents a Continue Statement
		/// ----------------------------------------
		/// Continue Statement's Grammar as below.
		/// continue-statement -> "continue" ";"
		/// ----------------------------------------
		class ContinueStatement : public StatementAST
		{
		public:
			ContinueStatement(SourceLocation start, SourceLocation end) :
				StatementAST(start, end) {}
			virtual ~ContinueStatement() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief ReturnStatementAST - This class represents a Return Statement
		/// ---------------------------------------------
		/// Return Statement's Grammar as below.
		/// return-statement -> "return" expression ";"
		/// return-statement -> "return" ";"
		/// return-statement -> "return" anonymous-initial ";"
		/// ---------------------------------------------
		class ReturnStatement : public StatementAST
		{
			std::unique_ptr<Expr> ReturnExpr;
		public:
			ReturnStatement(SourceLocation start, SourceLocation end,
				std::unique_ptr<Expr> returnExpr) : StatementAST(start, end),
				ReturnExpr(std::move(ReturnExpr)) {}

			virtual ~ReturnStatement() {}

			const Expr* getSubExpr() const { return ReturnExpr.get(); }

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief ExprStatementAST - This class represents a statement of expressions.
		/// ----------------------------------------
		/// ExprStatement's Grammar as below.
		/// expression-statement -> expression? ";"
		/// ----------------------------------------
		class ExprStatement : public StatementAST
		{
			std::unique_ptr<Expr> expr;
		public:
			ExprStatement(SourceLocation start, SourceLocation end,
				std::unique_ptr<Expr> expr) : StatementAST(start, end), expr(std::move(expr)) {}

			virtual ~ExprStatement() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief DeclStatementAST - This class represents a Declaration Statement
		/// -----------------------------------------------
		/// Declaration Statement's Grammar as below.
		/// declaration-statement -> function-declaration
		///					| constant-declaration
		///					| variable-declaration
		///					| class-declaration
		/// -----------------------------------------------
		class DeclStatement : public StatementAST
		{
		public:
			DeclStatement(SourceLocation start, SourceLocation end) : StatementAST(start, end) {}
			virtual ~DeclStatement() {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief ParameterDecl - This class represents a ParameterDecl
		/// ParameterDecl's Grammar as below:
		/// para-declaration -> type identifier | const type identifier
		///
		/// --------------------nonsense for coding-------------------------
		/// AST�����Ʋ��ع��ھ������ķ�
		/// --------------------nonsense for coding-------------------------
		class ParameterDecl final : public DeclStatement
		{
		private:
			std::string ParaName;
			std::shared_ptr<Type> type;
		public:
			ParameterDecl(SourceLocation start, SourceLocation end, std::string name, 
				std::shared_ptr<Type> type) : 
				DeclStatement(start, end), ParaName(name),  type(type) {}

			std::string getParmName() const { return ParaName; }

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// \brief UnpackDecl - This class represents a UnpackDecl.
		/// ����moses֧���������ͣ��������͵ı����������ش��ݡ������������͵ı����������Ҫ
		/// ��ȡ���е�ֵ������Ҫ���н����ע�����õ��ı���Ĭ�϶���const�ģ�Ҳ����˵��������
		/// ��������ֻ�������ݴ��ݣ�������C++�е���ʱֵ������˵����ֵ����
		/// Note: UnpackDeclֻ���ڽ����û�����֡�
		class UnpackDecl final : public DeclStatement
		{
		private:
			std::vector<DeclASTPtr> decls;
			std::shared_ptr<Type> type;
		public:
			UnpackDecl(SourceLocation start, SourceLocation end, std::vector<DeclASTPtr> decls)
				: DeclStatement(start, end), decls(std::move(decls)) {}
			bool TypeCheckingAndTypeSetting(AnonymousType* type);
			// To Do: Shit code!
			void setCorrespondingType(std::shared_ptr<Type> type);
			unsigned getDeclNumber() const { return decls.size(); };
			std::vector<std::string> operator[](unsigned index) const;
			void getDeclNames(std::vector<std::string>& names) const;

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief FunctionDecl - This class represents a Function Declaration
		/// which capture its name, and its argument names(thus implicitly the 
		/// number of arguments the function takes).
		/// Forward declarations are not supported.
		/// ----------------------------------------------------------------
		/// Function Declaration's Grammar as below.
		/// function-declaration -> func identifier para-list compound-statement
		/// ----------------------------------------------------------------
		/// To Do: ����Ʋ��Ǻܺ�������FunctionDecl��Ӧ����DeclStatement������
		class FunctionDecl : public DeclStatement
		{
			std::string FDName;
			std::vector<ParmDeclPtr> parameters;
			unsigned paraNum;
			StmtASTPtr funcBody;
			std::shared_ptr<Type> returnType;
			
		public:
			FunctionDecl(SourceLocation start, SourceLocation end, const std::string& name,
				std::vector<ParmDeclPtr> Args, StmtASTPtr body, std::shared_ptr<Type> returnType) :
				DeclStatement(start, end), FDName(name), parameters(std::move(Args)),
				funcBody(std::move(body)), paraNum(parameters.size()), returnType(returnType) {}

			virtual ~FunctionDecl() {}

			unsigned getParaNum() { return paraNum; }

			std::string getParmName(unsigned index) const { return parameters[index].get()->getParmName(); }

			// ���ڱ�ʶ�ú����ܷ�constant-evaluator. 
			// ���ں�����˵���ܽ���constant-evaluator�ı�׼��ֻ����һ��return��䣬�ҷ�����������������.
			// ���磺
			//	func add() -> int  { return 10; }
			const StatementAST* isEvalCandiateAndGetReturnStmt() const;

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief VarDecl - This class represents a Variable Declaration or a Const Variable 
		/// Declaration
		/// --------------------------------------------------------------
		/// Variable Declaration's Grammar as below.
		/// variable-declaration -> "var" identifier initializer ";"
		///				| "var" identifier type-annotation ";"
		/// and
		/// constanr-declaration -> "const" identifier initializer ";"
		///				| "const" identifier type-annotation ";"
		/// --------------------------------------------------------------
		class VarDecl : public DeclStatement
		{
			bool IsConst;
			std::string name;
			std::shared_ptr<Type> declType;
			ExprASTPtr InitExpr;

			// ��ª�Ĵ���ܹ�������moses��˵��const�����ɳ�ʼ����Ҳ�ɲ���ʼ�����༴ͨ����Ԫ��ֵ���г�ʼ����
			//						VarDecl
			//					   /       \
			//					name	 InitExpr
			//					/			  \
			//				  num			nullptr
			//
			//						   BE
			//						/  |  \
			//					  lhs  =   rhs
			//					 /            \
			//					num			  10
			// ����ֱ��ʹ��InitExr -> ָ��rhs����Ϊmoses ASTʹ�� std::unique_ptr<> �����Բ�����ͬʱ��AST
			// ���������ط�ָ��ͬһ��unique_ptr�������double free

			Expr* BEInit;
		public:
			VarDecl(SourceLocation start, SourceLocation end, std::string name, 
				std::shared_ptr<Type> type, bool isConst, ExprASTPtr init) :
				DeclStatement(start, end), name(name), declType(type), IsConst(isConst), 
				InitExpr(std::move(init)) {}
			std::string getName() { return name; }

			std::shared_ptr<Type> getDeclType() const { return declType; }

			void setInitExpr(Expr* B) { BEInit = B; }

			const Expr* getInitExpr() const { return InitExpr.get(); }

			bool isClass() const { return declType->getKind() == TypeKind::USERDEFIED; }

			bool isConst() const { return IsConst; }

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};

		/// @brief ClassDecl - This class represents a Class Declaration
		/// ---------------------------------------------------------
		/// Class Declaration's Grammar as below.
		/// class-declarartion -> "class" identifier class-body ";"
		/// ----------------------------------------------------------
		///
		/// ----------------------------------------------------------
		/// Class Body's Grammar as below.
		/// class-body -> "{" variable-declaration* "}"
		/// ----------------------------------------------------------
		/// To Do: classͨ�����ݳ�Ա�������Գ�������ڴ�ռ䡣
		class ClassDecl : public DeclStatement
		{
			// std::unique_ptr<UserDefinedType> classType;
			std::string ClassName;
			StmtASTPtr Body;
		public:
			ClassDecl(SourceLocation start, SourceLocation end, std::string name, StmtASTPtr body) :
				DeclStatement(start, end), ClassName(name), Body(std::move(body)) {}

			virtual void Accept(Visitor* v) const
			{
				v->visit(this);
			}
		};
	}	
}
#endif