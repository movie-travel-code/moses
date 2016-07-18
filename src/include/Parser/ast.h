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
#include <cassert>
#include "../IR/Value.h"
#include "../Support/SourceLocation.h"
#include "Type.h"

namespace compiler
{
	///---------------------------nonsense for coding------------------------------///
	/// syntax tree是在parse完成之后构建的，其中moses并不打算实现one-pass的compile
	/// 也就是说在构建syntax tree的时候，只是执行一些简单的语义分析，并存储错误。构建完成
	/// syntax tree后，再遍历syntax tree并执行代码生成部分。
	///---------------------------nonsense for coding------------------------------///

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
		class ExprStatement; 
		class IfStatement;
		class WhileStatement;
		class ClassDecl;
		class ReturnStatement;
		class UnaryExpr;
		class MemberExpr;
		class BoolLiteral;
		class DeclRefExpr;
		class BreakStatement;
		class ContinueStatement;

		using ASTPtr = std::vector<std::shared_ptr<StatementAST>>;
		using StmtASTPtr = std::shared_ptr<StatementAST>;
		using NumberExprPtr = std::shared_ptr<NumberExpr>;
		using ExprASTPtr = std::shared_ptr<Expr>;
		using CallExprPtr = std::shared_ptr<CallExpr>;
		using DeclASTPtr = std::shared_ptr<DeclStatement>;
		using CompoundStmtPtr = std::shared_ptr<CompoundStmt>;
		using VarDeclPtr = std::shared_ptr<VarDecl>;
		using ParmDeclPtr = std::shared_ptr<ParameterDecl>;
		using UnpackDeclPtr = std::shared_ptr<UnpackDecl>;
		using FunctionDeclPtr = std::shared_ptr<FunctionDecl>;
		using BinaryPtr = std::shared_ptr<BinaryExpr>;
		using UnaryPtr = std::shared_ptr<UnaryExpr>;
		using DeclRefExprPtr = std::shared_ptr<DeclRefExpr>;
		using AnonTyPtr = std::shared_ptr<AnonymousType>;
		using UDTyPtr = std::shared_ptr<UserDefinedType>;
		using ReturnStmtPtr = std::shared_ptr<ReturnStatement>;
		using MemberExprPtr = std::shared_ptr<MemberExpr>;
		using BoolLiteralPtr = std::shared_ptr<BoolLiteral>;
		using BreakStmtPtr = std::shared_ptr<BreakStatement>;
		using ContStmtPtr = std::shared_ptr<ContinueStatement>;

		using IRValue = std::shared_ptr<IR::Value>;

		/// \brief StatementAST - Base class for all statements.
		//--------------------------nonsense for coding---------------------------
		// 这里使用std::shared_ptr来包裹AST树，有些鸡肋，因为IR生成的时候，直接将
		// AST析构掉（还不如直接在AST树中内置 CodeGen() 函数，一边遍历一边生成代码）。
		//--------------------------nonsense for coding---------------------------
		class StatementAST
		{
		public:
			// Visitor Pattern，使用override(虚函数) + overload(重载) 来做double dispatch.
			// 其中关于被访问体（也就是AST node）部分，通过override的 Accept()的方法来实现
			// 第一层dispatch，第一层dispatch通过override的方式拿到具体的节点类型。
			// 第二层dispatch使用重载实现（见IRbuilder）。
			// http://www.cs.wustl.edu/~cytron/cacweb/Tutorial/Visitor/
			// And 
			// http://programmers.stackexchange.com/questions/189476/implementing-the-visitor-pattern-for-an-abstract-syntax-tree
			template<typename RetTy, typename... Additional>
			class Visitor
			{
			public:
				virtual RetTy visit(const StatementAST* S, Additional... addi) = 0;
				virtual RetTy visit(const ExprStatement* ES, Additional... addi) = 0;
				virtual RetTy visit(const CompoundStmt* CS, Additional... addi) = 0;
				virtual RetTy visit(const IfStatement* IS, Additional... addi) = 0;
				virtual RetTy visit(const WhileStatement* WS, Additional... addi) = 0;
				virtual RetTy visit(const ReturnStatement* RS, Additional... addi) = 0;
				virtual RetTy visit(const DeclStatement* DS, Additional... addi) = 0;
				virtual RetTy visit(const BreakStatement* BS, Additional... addi) = 0;
				virtual RetTy visit(const ContinueStatement* CS, Additional... addi) = 0;
				virtual RetTy visit(const VarDecl* VD, Additional... addi) = 0;
				virtual RetTy visit(const ParameterDecl*, Additional... addi) = 0;
				virtual RetTy visit(const ClassDecl* CD, Additional... addi) = 0;
				virtual RetTy visit(const FunctionDecl* FD, Additional... addi) = 0;
				virtual RetTy visit(const UnpackDecl* UD, Additional... addi) = 0;
				virtual RetTy visit(const Expr* E, Additional... addi) = 0;
				virtual RetTy visit(const BinaryExpr* BE, Additional... addi) = 0;
				virtual RetTy visit(const CallExpr* CE, Additional... addi) = 0;
				virtual RetTy visit(const DeclRefExpr* DRE, Additional... addi) = 0;
				virtual RetTy visit(const BoolLiteral* BL, Additional... addi) = 0;
				virtual RetTy visit(const NumberExpr* NE, Additional... addi) = 0;
				virtual RetTy visit(const UnaryExpr* UE, Additional... addi) = 0;
				virtual RetTy visit(const MemberExpr* ME, Additional... addi) = 0;
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
			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
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
			// Expr都会有相应的Type，这个很重要，用于进行类型检查。
			// ------------------------nonsense for coding---------------------------
			std::shared_ptr<Type> ExprType;
			ExprValueKind VK;
			// 用于标识当前Expr是否是可推导出constant的
			// 例如： 
			// a = num * 5; /* num * 5 是可推导的 */
			// var num = {10, num}; /* {10, num} 是可推导的 */
			// 在moses中默认用户自定义类型是不可推导的。
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

			void setType(std::shared_ptr<Type> type) { ExprType = type; }

			/// isLvalue - True if this expression is an "l-value" according to
			/// the rules of the current language. Like C/C++，moses give somewhat
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

			// Silly!
			// If we have other visitors, we will get a "Accept" list!
			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief NumberExpr - 用来表示numeric literal, 例如"1".
		class NumberExpr : public Expr
		{
			double Val;
		public:
			NumberExpr(SourceLocation start, SourceLocation end, double Val) :
				Expr(start, end, nullptr, ExprValueKind::VK_RValue, true), Val(Val) {}
			virtual ~NumberExpr() {}

			void setIntType(std::shared_ptr<Type> type)
			{
				Expr::setType(type);
			}

			double getVal() const { return Val; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief CharExpr - 用来表示char literal, 例如"c"
		class CharExpr : public Expr
		{
			std::string C;
		public:
			/// To Do: 此处使用INT来表示CharExpr，也就是两者可以相加减
			CharExpr(SourceLocation start, SourceLocation end, std::string c) : 
				Expr(start, end, nullptr, ExprValueKind::VK_RValue, true), C(c) 
			{}

			void setCharType(std::shared_ptr<Type> type) 
			{
				Expr::setType(type);
			}

			virtual ~CharExpr() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief StringLiteral - 用来表示string.
		class StringLiteral : public Expr
		{
			std::string str;
		public:
			StringLiteral(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
						std::string str) : 
				Expr(start, end, type, ExprValueKind::VK_RValue, true), str(str) {}
			virtual ~StringLiteral() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief BoolLiteral - 用来表示bool字面值.
		class BoolLiteral : public Expr
		{		
			bool value;
		public:
			BoolLiteral(SourceLocation start, SourceLocation end, bool value) :
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::BOOL), 
				ExprValueKind::VK_RValue, true), value(value) {}

			void setBoolType(std::shared_ptr<Type> type)
			{
				Expr::setType(type);
			}

			bool getVal() const
			{
				return value;
			}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief DeclRefExprAST - A reference to a declared variable, function, enum, etc.
		/// This encodes all the information about how a declaration is referenced within an 
		/// expression.
		///-------------------------------nonsense for coding-------------------------------
		/// moses IR有一个不足（应该说是bug），在语法树中没有直接体现出左侧的DeclRefExpr与右侧的
		/// DeclRefExpr，类似于Clang dump出的语法树，作为右值表达式的DeclRefExpr会被一个左值转
		/// 右值的隐式转换类型包裹。
		///---------------------------------------------------------------------------------
		class DeclRefExpr final : public Expr
		{
			std::string Name;
			VarDeclPtr var;
		public:
			// Note: 一般情况下，DeclRefExpr都是左值的，但是有一种情况例外，就是函数名字调用
			// 但是函数调用对应的expression是CallExpr，不是DeclRefExpr.
			// To Do: 有可能会有潜在的bug
			DeclRefExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
					std::string name, VarDeclPtr var) : 
				Expr(start, end, type, ExprValueKind::VK_LValue, true), Name(name), var(var) 
			{}

			std::string getDeclName() const { return Name; }

			VarDeclPtr getDecl() const { return var; }

			virtual ~DeclRefExpr() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief BinaryExpr - Expression class for a binary operator.
		class BinaryExpr : public Expr
		{
			std::string OpName;
			ExprASTPtr LHS, RHS;
		public:
			// Note: 在moses中不存在指针类型，所以不存在binary为lvalue的情况
			// 例如: 'int* p = &num;'
			// 'p + 1'就可以作为左值
			// Note: BinaryExpr都是可以进行evaluate的
			BinaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
						std::string Op, ExprASTPtr LHS, ExprASTPtr RHS) :
				Expr(start, end, type, ExprValueKind::VK_RValue, true), OpName(Op), LHS(LHS), RHS(RHS) 
			{}

			ExprASTPtr getLHS() const { return LHS; }

			ExprASTPtr getRHS() const { return RHS; }

			std::string getOpcode() const { return OpName; }

			virtual ~BinaryExpr() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief UnaryExpr - Expression class for a unary operator.
		class UnaryExpr final : public Expr
		{
			std::string OpName;
			ExprASTPtr SubExpr;
		public:
			UnaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, std::string Op,
				ExprASTPtr subExpr, ExprValueKind vk) :
				Expr(start, end, type, vk, true), OpName(Op), SubExpr(SubExpr) {}

			ExprASTPtr getSubExpr() const { return SubExpr; }

			std::string getOpcode() const { return OpName; }

			void setOpcode(std::string name) { OpName = name; }

			virtual ~UnaryExpr() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief CallExprAST - Expression class for function calls.
		class CallExpr final : public Expr
		{
			std::string CalleeName;
			// To Do: FunctionDecl*
			FunctionDeclPtr FuncDecl;
			std::vector<std::shared_ptr<Expr> > Args;
		public:
			CallExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
					const std::string& Callee, std::vector<std::shared_ptr<Expr>> Args, 
					ExprValueKind vk, FunctionDeclPtr FD, bool canDoEvaluate) :
				Expr(start, end, type, vk, canDoEvaluate), CalleeName(Callee), Args(Args), 
				FuncDecl(FD) 
			{}

			unsigned getArgsNum() const { return Args.size(); }

			FunctionDeclPtr getFuncDecl() const { return FuncDecl; }

			// Note: 这里我们没有对传入的index进行检查
			ExprASTPtr getArg(unsigned index) const { return Args[index]; }

			const std::vector<ExprASTPtr> getArgs() const { return Args; }

			virtual ~CallExpr() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief MemberExpr - Class members. X.F.
		class MemberExpr final : public Expr
		{
			/// Base - the expression for the base pointer or structure references. In
			/// X.F, this is "X". 即DeclRefExpr
			StmtASTPtr Base;

			/// name - member name
			std::string name;

			/// MemberLoc - This is the location of the member name.
			SourceLocation MemberLoc;

			/// This is the location of the -> or . in the expression.
			SourceLocation OperatorLoc;
		public:
			MemberExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
				std::shared_ptr<Expr> base, SourceLocation operatorloc, std::string name, bool canDoEvaluate) :
				Expr(start, end, type, ExprValueKind::VK_LValue, canDoEvaluate), Base(base), 
				OperatorLoc(operatorloc), name(name) 
			{}

			void setBase(ExprASTPtr E) { Base = E; }

			std::string getMemberName() const { return name; }

			ExprASTPtr getBase() const { return nullptr; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};


		/// \brief 匿名类型初始化表达式。
		///	var start = 0;
		/// var end = 1;
		/// 例如： var num = {start, end};
		/// 
		/// 另外后面关于用户自定义类型的初始化拟采用这样的方式进行赋值。
		class AnonymousInitExpr final : public Expr
		{
			AnonymousInitExpr() = delete;
			AnonymousInitExpr(const AnonymousInitExpr&) = delete;
			std::vector<ExprASTPtr> InitExprs;
		public:
			AnonymousInitExpr(SourceLocation start, SourceLocation end, 
					std::vector<ExprASTPtr> initExprs, std::shared_ptr<Type> type):
				Expr(start, end, type, Expr::ExprValueKind::VK_RValue, true), InitExprs(initExprs) 
			{}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief CompoundStatement - This class represents a Compound Statement
		/// ------------------------------------------
		/// Compound Statement's Grammar as below.
		/// compound-statement -> "{" statement* "}"
		/// ------------------------------------------
		class CompoundStmt final : public StatementAST
		{
			// The sub-statements of the compound statement.
			std::vector<StmtASTPtr> SubStmts;
		public:
			CompoundStmt(SourceLocation start, SourceLocation end,
				std::vector<StmtASTPtr> subStmts) : StatementAST(start, end), SubStmts(subStmts) {}

			virtual ~CompoundStmt() {}

			void addSubStmt(StmtASTPtr stmt);

			StmtASTPtr getSubStmt(unsigned index) const;

			StmtASTPtr operator[](unsigned index) const;

			unsigned getSize() const 
			{
				return SubStmts.size();
			}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief IfStatementAST - This class represents a If Statement
		/// ----------------------------------------------------------------------------
		/// If Statement's Grammar as below.
		/// if-statement -> "if" expression compound-statement "else" compound-statement
		/// ----------------------------------------------------------------------------
		class IfStatement final : public StatementAST
		{
			std::shared_ptr<Expr> Condition;
			std::shared_ptr<StatementAST> Then;
			std::shared_ptr<StatementAST> Else;
		public:
			IfStatement(SourceLocation start, SourceLocation end, ExprASTPtr Condition, StmtASTPtr Then,
				StmtASTPtr Else) : 
				StatementAST(start, end), Condition(Condition), Then(Then), Else(Else) {}

			virtual ~IfStatement() {}

			// To Do: 这样做是不合适的，应该时刻都使用unique_ptr来传递各个节点
			// 但是这样做的话，太过于复杂
			StmtASTPtr getThen() const
			{
				return Then;
			}

			StmtASTPtr getElse() const
			{
				return Else;
			}

			ExprASTPtr getCondition() const
			{
				return Condition;
			}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief WhileStatementAST - This class represents a While Statement
		/// ---------------------------------------------------------
		/// While Statement's Grammar as below.
		/// while-statement -> "while" expression compound-statement
		/// ---------------------------------------------------------
		class WhileStatement final : public StatementAST
		{
			ExprASTPtr Condition;
			StmtASTPtr LoopBody;
		public:
			WhileStatement(SourceLocation start, SourceLocation end, ExprASTPtr condition, 
					StmtASTPtr body) : 
					StatementAST(start, end), Condition(condition), LoopBody(body)
			{}

			ExprASTPtr getCondition() const { return Condition; }
			StmtASTPtr getLoopBody() const { return LoopBody; }
			virtual ~WhileStatement() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief BreakStatementAST - This class represents a Break Statement
		/// ---------------------------------------
		/// Break Statement's Grammar as below.
		/// break-statement -> "break" ";"
		/// ---------------------------------------
		class BreakStatement final : public StatementAST
		{
		public:
			BreakStatement(SourceLocation start, SourceLocation end) : StatementAST(start, end) {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief ContinueStatementAST - This class represents a Continue Statement
		/// ----------------------------------------
		/// Continue Statement's Grammar as below.
		/// continue-statement -> "continue" ";"
		/// ----------------------------------------
		class ContinueStatement final : public StatementAST
		{
		public:
			ContinueStatement(SourceLocation start, SourceLocation end) :
				StatementAST(start, end) {}
			virtual ~ContinueStatement() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief ReturnStatementAST - This class represents a Return Statement
		/// ---------------------------------------------
		/// Return Statement's Grammar as below.
		/// return-statement -> "return" expression ";"
		/// return-statement -> "return" ";"
		/// return-statement -> "return" anonymous-initial ";"
		/// ---------------------------------------------
		class ReturnStatement final : public StatementAST
		{
			std::shared_ptr<Expr> ReturnExpr;
		public:
			ReturnStatement(SourceLocation start, SourceLocation end, ExprASTPtr returnExpr) : 
				StatementAST(start, end), ReturnExpr(returnExpr) 
			{}

			virtual ~ReturnStatement() {}

			ExprASTPtr getSubExpr() const { return ReturnExpr; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief ExprStatementAST - This class represents a statement of expressions.
		/// ----------------------------------------
		/// ExprStatement's Grammar as below.
		/// expression-statement -> expression? ";"
		/// ----------------------------------------
		class ExprStatement : public StatementAST
		{
			std::shared_ptr<Expr> expr;
		public:
			ExprStatement(SourceLocation start, SourceLocation end, ExprASTPtr expr) : 
				StatementAST(start, end), expr(expr) {}

			virtual ~ExprStatement() {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
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
		protected:
			std::shared_ptr<Type> declType;
		public:
			DeclStatement(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type) :
				StatementAST(start, end), declType(type) {}

			virtual ~DeclStatement() {}

			std::shared_ptr<Type> getDeclType() const { return declType; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
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
			ExprASTPtr InitExpr;
		public:
			VarDecl(SourceLocation start, SourceLocation end, std::string name,
				std::shared_ptr<Type> type, bool isConst, ExprASTPtr init) :
				DeclStatement(start, end, type), name(name), IsConst(isConst),
				InitExpr(init) {}
			std::string getName() const { return name; }

			std::shared_ptr<Type> getDeclType() const { return declType; }

			void setInitExpr(ExprASTPtr B) { InitExpr = B; }

			ExprASTPtr getInitExpr() const { return InitExpr; }

			bool isClass() const { return declType->getKind() == TypeKind::USERDEFIED; }

			bool isConst() const { return IsConst; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// @brief ParameterDecl - This class represents a ParameterDecl
		/// Note: 暂时还没有实现paramdecl - const 和 init expr
		class ParameterDecl final : public VarDecl
		{
		public:
			ParameterDecl(SourceLocation start, SourceLocation end, std::string name, bool isConst,
				std::shared_ptr<Type> type) : 
				VarDecl(start, end, name, type, isConst, nullptr) {}

			std::string getParmName() const { return VarDecl::getName(); }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};

		/// \brief UnpackDecl - This class represents a UnpackDecl.
		/// 由于moses支持匿名类型，匿名类型的变量可以来回传递。对于匿名类型的变量，如果想要
		/// 获取其中的值，就需要进行解包，注意解包得到的变量默认都是const的，也就是说匿名类型
		/// 变量仅仅只用于数据传递，类似于C++中的临时值（或者说是右值）。
		/// Note: UnpackDecl只用于解包，没有名字。
		class UnpackDecl final : public DeclStatement
		{
		private:
			std::vector<DeclASTPtr> decls;
		public:
			UnpackDecl(SourceLocation start, SourceLocation end, std::vector<DeclASTPtr> decls)
				: DeclStatement(start, end, nullptr), decls(decls) {}

			bool TypeCheckingAndTypeSetting(AnonTyPtr type);

			// To Do: Shit code!
			void setCorrespondingType(std::shared_ptr<Type> type);

			unsigned getDeclNumber() const { return decls.size(); };

			std::vector<std::string> operator[](unsigned index) const;

			void getDeclNames(std::vector<std::string>& names) const;

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
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
		/// To Do: 类设计不是很合理，例如FunctionDecl不应该是DeclStatement的子类
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
				DeclStatement(start, end, nullptr), FDName(name), parameters(Args), funcBody(body), 
				paraNum(parameters.size()), returnType(returnType) 
			{}

			virtual ~FunctionDecl() {}
			unsigned getParaNum() const { return paraNum; }
			std::vector<ParmDeclPtr> getParms() const { return parameters; }
			std::string getParmName(unsigned index) const { return parameters[index].get()->getParmName(); }
			std::shared_ptr<Type> getReturnType() const { return returnType; }
			std::string getFDName() const { return FDName; }
			ParmDeclPtr getParmDecl(unsigned index) const { return (*this)[index]; }
			ParmDeclPtr operator[](unsigned index) const 
			{
				assert(index <= paraNum - 1 && "Index out of range when we get specified ParmDecl.");
				return parameters[index];
			}
			// 用于标识该函数能否constant-evaluator. 
			// 对于函数来说，能进行constant-evaluator的标准是只能有一条return语句，且返回类型是内置类型.
			// 例如：
			//	func add() -> int  { return 10; }
			// To Do: 这里我们强制要求，能够进行constant-evaluate的函数只能有一条return语句
			// 如果后面需要加强推导能力，就需要返回这个函数体了。
			ReturnStmtPtr isEvalCandiateAndGetReturnStmt() const;

			/// Determine whether the function F ends with a return stmt.
			bool endsWithReturn() const;

			StmtASTPtr getCompoundBody() const { return funcBody; }

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
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
		/// To Do: class通过数据成员的重排以充分利用内存空间。
		class ClassDecl final : public DeclStatement
		{
			// std::shared_ptr<UserDefinedType> classType;
			std::string ClassName;
			StmtASTPtr Body;
		public:
			ClassDecl(SourceLocation start, SourceLocation end, std::string name, StmtASTPtr body) :
				DeclStatement(start, end, nullptr), ClassName(name), Body(body) {}

			virtual IRValue Accept(Visitor<IRValue>* v) const
			{
				return v->visit(this);
			}
		};
	}	
}
#endif