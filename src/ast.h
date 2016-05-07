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
#include "SourceLocation.h"
#include "Type.h"

namespace compiler
{
	///---------------------------nonsense for coding------------------------------///
	/// syntax tree����parse���֮�󹹽��ģ�����moses��������ʵ��one-pass��compile
	/// Ҳ����˵�ڹ���syntax tree��ʱ��ֻ��ִ��һЩ�򵥵�������������洢���󡣹������
	/// syntax tree���ٱ���syntax tree��ִ�д������ɲ��֡�
	///---------------------------nonsense for coding------------------------------///

	namespace sema
	{
		class VariableSymbol;
	}
	
	namespace ast
	{
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

		/// \brief StatementAST - Base class for all statements.
		class StatementAST
		{
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
			SourceLocation getLocStart() const { return LocStart; }
			SourceLocation getLocEnd() const { return LocEnd; }
			void setLocStart(unsigned long line, unsigned long number) {}
			void setLocEnd(unsigned long line, unsigned long number) {}
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
		public:
			Expr() : ExprType(nullptr) {}
			Expr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, ExprValueKind vk) :
				StatementAST(start, end), ExprType(type), VK(vk) {}
			Expr(const Expr& expr) : 
				StatementAST(expr.LocStart, expr.LocEnd), ExprType(expr.ExprType) {}
			std::shared_ptr<Type> getType() const { return ExprType; }

			/// isLvalue - True if this expression is an "l-value" according to
			/// the rules of the current language. Like C/C++��moses give somewhat
			/// different rules for this concept, but in general, the result of 
			/// an l-value expression identifies a specific object whereas the
			/// result of an r-value expression is a value detached from any 
			/// specific storage.
			bool isLValue() const { return VK == ExprValueKind::VK_LValue; }
			bool isRValue() const { return VK == ExprValueKind::VK_RValue; }

			void setExprValueKind(ExprValueKind valueKind) { VK = valueKind; }

			virtual ~Expr() {}
		};

		/// \brief NumberExpr - ������ʾnumeric literal, ����"1".
		class NumberExpr : public Expr
		{
			double Val;
		public:
			NumberExpr(SourceLocation start, SourceLocation end, double Val) :
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::INT, true), ExprValueKind::VK_RValue),
				Val(Val) {}
			virtual ~NumberExpr() {}
		};

		/// \brief CharExpr - ������ʾchar literal, ����"c"
		class CharExpr : public Expr
		{
			std::string C;
		public:
			/// To Do: �˴�ʹ��INT����ʾCharExpr��Ҳ�������߿�����Ӽ�
			CharExpr(SourceLocation start, SourceLocation end, std::string c) : 
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::INT, true), ExprValueKind::VK_RValue),
				C(c) {}
			virtual ~CharExpr() {}
		};

		/// \brief StringLiteral - ������ʾstring.
		class StringLiteral : public Expr
		{
			std::string str;
		public:
			StringLiteral(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
				std::string str) : Expr(start, end, type, ExprValueKind::VK_RValue), str(str) {}
			virtual ~StringLiteral() {}
		};

		/// \brief BoolLiteral - ������ʾbool����ֵ.
		class BoolLiteral : public Expr
		{		
			bool value;
		public:
			BoolLiteral(SourceLocation start, SourceLocation end, bool value) :
				Expr(start, end, std::make_shared<BuiltinType>(TypeKind::BOOL, true), ExprValueKind::VK_RValue),
				value(value) {}
		};

		/// @brief DeclRefExprAST - A reference to a declared variable, function, enum, etc.
		/// This encodes all the information about how a declaration is referenced within an 
		/// expression
		/// ------------------------------nonsence for coding------------------------
		/// ����AST�е�DeclRefExpr��������˵�Ƕ�һ�޶��ģ�����û�ж��忽�����캯����
		/// ����ʹ�ÿ������壬�������е��﷨���Ľڵ��ڶ���ֻ��һ�ݶ��洢��
		/// -------------------------------------------------------------------------
		class DeclRefExpr final : public Expr
		{
			std::string Name;
			std::unique_ptr<DeclStatement> decl;
		public:
			// Note: һ������£�DeclRefExpr������ֵ�ģ�������һ��������⣬���Ǻ������ֵ���
			// ���Ǻ������ö�Ӧ��expression��CallExpr������DeclRefExpr.
			// To Do: �п��ܻ���Ǳ�ڵ�bug
			DeclRefExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, std::string name,
				std::unique_ptr<DeclStatement> decl) : 
				Expr(start, end, type, ExprValueKind::VK_LValue), Name(name), decl(std::move(decl)) {}

			std::string getDeclName() const { return Name; }

			virtual ~DeclRefExpr() {}
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
			BinaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
				std::string Op, ExprASTPtr LHS, ExprASTPtr RHS) :
				Expr(start, end, type, ExprValueKind::VK_RValue), OpName(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

			virtual ~BinaryExpr() {}
		};

		/// \brief UnaryExpr - Expression class for a unary operator.
		class UnaryExpr : public Expr
		{
			std::string OpName;
			ExprASTPtr SubExpr;
		public:
			UnaryExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, std::string Op,
				ExprASTPtr subExpr, ExprValueKind vk) :
				Expr(start, end, type, vk), OpName(Op), SubExpr(std::move(SubExpr)) {}

			std::string getOpCodeName() { return OpName; }
			void setOpCodeName(std::string name) { OpName = name; }
			virtual ~UnaryExpr() {}
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
				ExprValueKind vk, const FunctionDecl* FD) : 
				Expr(start, end, type, vk), CalleeName(Callee), Args(std::move(Args)), FuncDecl(FD) {}

			virtual ~CallExpr() {}
		};

		/// \brief MemberExpr - Class members. X.F.
		class MemberExpr final : public Expr
		{
			/// Base - the expression for the base pointer or structure references. In
			/// X.F, this is "X".
			StmtASTPtr Base;

			/// name - member name
			std::string name;

			/// MemberLoc - This is the location of the member name.
			SourceLocation MemberLoc;

			/// This is the location of the -> or . in the expression.
			SourceLocation OperatorLoc;
		public:
			MemberExpr(SourceLocation start, SourceLocation end, std::shared_ptr<Type> type, 
				std::unique_ptr<Expr> base, SourceLocation operatorloc, std::string name) :
				Expr(start, end, type, ExprValueKind::VK_LValue), Base(std::move(base)), OperatorLoc(operatorloc),
				name(name) 
			{}

			void setBase(ExprASTPtr E) { Base = std::move(E); }
			ExprASTPtr getBase() const { return nullptr; }
		};


		/// \brief
		class AnonymousInitExpr final : public Expr
		{
			AnonymousInitExpr() = delete;
			AnonymousInitExpr(const AnonymousInitExpr&) = delete;
			std::vector<ExprASTPtr> InitExprs;
		public:
			AnonymousInitExpr(SourceLocation start, SourceLocation end, 
				std::vector<ExprASTPtr> initExprs, std::shared_ptr<Type> type) :
			Expr(start, end, type, Expr::ExprValueKind::VK_RValue), InitExprs(std::move(initExprs)) {}
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
			// The size of the compound statement(The number of sub-statements)
			unsigned size;
		public:
			CompoundStmt(SourceLocation start, SourceLocation end,
				std::vector<StmtASTPtr> subStmts) :
				StatementAST(start, end), SubStmts(std::move(subStmts)) {}
			virtual ~CompoundStmt() {}
			void addSubStmt(StmtASTPtr stmt);
			const StatementAST* getSubStmt(unsigned index);
			unsigned getSize() { return size; }
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
		public:
			VarDecl(SourceLocation start, SourceLocation end, std::string name, 
				std::shared_ptr<Type> type, bool isConst, ExprASTPtr init) :
				DeclStatement(start, end), name(name), declType(type), IsConst(isConst), 
				InitExpr(std::move(init)) {}
			std::string getName() { return name; }
			bool isClass();
			bool isConst() { return IsConst; }
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
		};
	}
}
#endif