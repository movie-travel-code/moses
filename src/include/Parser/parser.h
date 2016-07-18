//===--------------------------------parser.h-----------------------------===//
// 
// This file is used to implement the parser.
// 
//===---------------------------------------------------------------------===//
#ifndef PARSER_INCLUDE
#define PARSER_INCLUDE
#include <algorithm>
#include "../Lexer/scanner.h"
#include "ast.h"
#include "sema.h"
#include "ASTContext.h"
namespace compiler
{
	namespace parse
	{
		using namespace compiler::ast;
		using namespace compiler::sema;
		using namespace compiler::lex;

		namespace OperatorPrec
		{
			/// \brief PrecedenceLevels - These are precedences for the binary operators in moses's 
			/// grammar. Low precedences numbers bind more weakly than high numbers.
			enum Level
			{
				Unknown = 0,
				Comma = 1,	// ,
				Assignment = 2,	// = *= /= += -=
				LogicalOr = 3,	// ||
				LogicalAnd = 4,	// &&
				Equality = 5,	// ==, !=
				Relational = 6,	// >= <= > <
				Additive = 7,	// + -
				Multiplicative = 8,	// *, /
				PointerToMember = 9		// .
			};
		}

		/// \brief ParseContext - To achieve a good results, we set parse context.
		namespace ParseContext
		{
			enum class context : unsigned short
			{
				CompoundStatement,
				Statement,
				UnaryExpression,
				PostExpression,
				PrimaryExpression,
				ArgListExpression,
				ParmList,
				Expression,
				ParmDecl,
				ArgExpression,
				FunctionDefinition,
				ClassBody,
				ReturnType
			};
			using namespace tok;

			// 使用Follow集合进行语法错误恢复，由于语法分析的函数和文法没有强对应，所以有的文法并没有给出
			// SafeSymbols.
			// StmtSafeSymbols包含statement, if-statement, while-statement, break-statement,
			// continue-statement, return-statement, expression-statement, declaration-statement,
			// varaiable-declaration, class-declaration, constant-declaration
			static std::vector<TokenValue> StmtSafeSymbols = {
				TokenValue::PUNCTUATOR_Left_Brace, TokenValue::KEYWORD_while,
				TokenValue::KEYWORD_continue, TokenValue::KEYWORD_if,
				TokenValue::KEYWORD_return, TokenValue::KEYWORD_break,
				TokenValue::KEYWORD_const, TokenValue::KEYWORD_class, TokenValue::KEYWORD_var,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::BO_Sub,
				TokenValue::UO_Exclamatory, TokenValue::UO_Inc, TokenValue::UO_Dec,
				TokenValue::IDENTIFIER, TokenValue::PUNCTUATOR_Left_Paren,
				TokenValue::INTEGER_LITERAL, TokenValue::BOOL_FALSE, TokenValue::BOOL_TRUE,
				TokenValue::KEYWORD_func, TokenValue::PUNCTUATOR_Right_Brace };

			static std::vector<TokenValue> CompoundStmtSafeSymbols = {
				TokenValue::KEYWORD_else, TokenValue::PUNCTUATOR_Left_Brace,
				TokenValue::KEYWORD_while, TokenValue::KEYWORD_continue,
				TokenValue::KEYWORD_if, TokenValue::KEYWORD_return, TokenValue::KEYWORD_break,
				TokenValue::KEYWORD_const, TokenValue::KEYWORD_class, TokenValue::KEYWORD_var,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::UO_Exclamatory,
				TokenValue::UO_Inc, TokenValue::UO_Dec, TokenValue::IDENTIFIER,
				TokenValue::PUNCTUATOR_Left_Paren, TokenValue::INTEGER_LITERAL,
				TokenValue::BOOL_TRUE, TokenValue::BOOL_FALSE, TokenValue::KEYWORD_func,
				TokenValue::PUNCTUATOR_Right_Brace };

			// ExprSafeSymbols包含expression, assignment-expression, condition-or-expression
			static std::vector<TokenValue> ExprSafeSymbols = {
				TokenValue::PUNCTUATOR_Right_Paren, TokenValue::PUNCTUATOR_Semicolon,
				TokenValue::PUNCTUATOR_Left_Brace, TokenValue::PUNCTUATOR_Comma };

			// u-expression post-expression
			static std::vector<TokenValue> UnaryAndPostExprSafeSymbols = {
				TokenValue::BO_Mul, TokenValue::BO_Div, TokenValue::BO_Assign,
				TokenValue::BO_MulAssign, TokenValue::BO_DivAssign, TokenValue::BO_AddAssign,
				TokenValue::BO_SubAssign, TokenValue::BO_And, TokenValue::BO_AndAssign,
				TokenValue::BO_Add, TokenValue::BO_Sub, TokenValue::BO_LT, TokenValue::BO_LE,
				TokenValue::BO_GT, TokenValue::BO_GE, TokenValue::BO_EQ, TokenValue::BO_NE,
				TokenValue::BO_And, TokenValue::BO_Or, TokenValue::PUNCTUATOR_Right_Paren,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::PUNCTUATOR_Left_Brace,
				TokenValue::PUNCTUATOR_Comma };

			// primary-expression and arg-list
			static std::vector<TokenValue> PrimaryAndArgListExprSafeSymbols = {
				TokenValue::BO_Mul, TokenValue::BO_Div, TokenValue::BO_Assign,
				TokenValue::BO_MulAssign, TokenValue::BO_DivAssign, TokenValue::BO_AddAssign,
				TokenValue::BO_SubAssign, TokenValue::BO_And, TokenValue::BO_AndAssign,
				TokenValue::BO_Add, TokenValue::BO_Sub, TokenValue::BO_LT, TokenValue::BO_LE,
				TokenValue::BO_GT, TokenValue::BO_GE, TokenValue::BO_EQ, TokenValue::BO_NE,
				TokenValue::BO_And, TokenValue::BO_Or, TokenValue::PUNCTUATOR_Right_Paren,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::PUNCTUATOR_Left_Brace,
				TokenValue::PUNCTUATOR_Comma, TokenValue::PUNCTUATOR_Member_Access,
				TokenValue::UO_Inc, TokenValue::UO_Dec };

			// para-list
			static std::vector<TokenValue> ParaListSafeSymbols = { TokenValue::PUNCTUATOR_Left_Brace };

			// para-declaration
			static std::vector<TokenValue> ParaDeclSafeSymbols = { TokenValue::PUNCTUATOR_Comma,
				TokenValue::PUNCTUATOR_Right_Paren };

			// arg
			static std::vector<TokenValue> ArgSafeSymbols = { TokenValue::PUNCTUATOR_Comma,
				TokenValue::PUNCTUATOR_Right_Paren };

			// FunctionDefinition
			static std::vector<TokenValue> FuncDefSafeSymbols = {
				TokenValue::PUNCTUATOR_Left_Brace, TokenValue::KEYWORD_while,
				TokenValue::KEYWORD_continue, TokenValue::KEYWORD_if,
				TokenValue::KEYWORD_return, TokenValue::KEYWORD_break,
				TokenValue::KEYWORD_const, TokenValue::KEYWORD_class, TokenValue::KEYWORD_var,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::BO_Sub,
				TokenValue::UO_Exclamatory, TokenValue::UO_Inc, TokenValue::UO_Dec,
				TokenValue::IDENTIFIER, TokenValue::PUNCTUATOR_Left_Paren,
				TokenValue::INTEGER_LITERAL, TokenValue::BOOL_FALSE, TokenValue::BOOL_TRUE,
				TokenValue::KEYWORD_func };

			// class-body
			static std::vector<TokenValue> ClassBodySafeSymbols = { TokenValue::PUNCTUATOR_Semicolon };

			static std::vector<TokenValue> ReturnType = {TokenValue::PUNCTUATOR_Right_Brace,
				TokenValue::PUNCTUATOR_Left_Brace, TokenValue::KEYWORD_while,
				TokenValue::KEYWORD_continue, TokenValue::KEYWORD_if,
				TokenValue::KEYWORD_return, TokenValue::KEYWORD_break,
				TokenValue::KEYWORD_const, TokenValue::KEYWORD_class, TokenValue::KEYWORD_var,
				TokenValue::PUNCTUATOR_Semicolon, TokenValue::BO_Sub,
				TokenValue::UO_Exclamatory, TokenValue::UO_Inc, TokenValue::UO_Dec,
				TokenValue::IDENTIFIER, TokenValue::PUNCTUATOR_Left_Paren,
				TokenValue::INTEGER_LITERAL, TokenValue::BOOL_FALSE, TokenValue::BOOL_TRUE,
				TokenValue::KEYWORD_func};
		}

		/// \brief Parser - Implenment a LL(1) parser.
		/// Use syntax-directed semantic analysis.
		/// --------------------------nonsense for coding-------------------------------
		/// parse*()函数接受该非终结符的继承属性，并返回该非终结符的综合属性
		/// --------------------------nonsense for coding-------------------------------
		class Parser
		{
			
			Parser(const Parser &) = delete;
			void operator=(const Parser&) = delete;
		public:
			enum class ContextKind { TopLevel, Function, Class, While };
		private:
			Scanner& scan;
			ASTPtr AST;
			ASTContext &Ctx;
			Sema& Actions;
			ContextKind CurrentContext;
		public:
			Parser(Scanner& scan, Sema& Actions, ASTContext& Ctx);
			/// \brief parse - Parse the entire file specified.
			ASTPtr& parse();

			// 语法解析子程序：
			// The most important one is that this routine eats all of the tokens 
			// that correspond to the production and returns the lexer buffer with 
			// the next token (which is not part of the grammar production) ready 
			// to go.

			ExprASTPtr ParseNumberExpr();
			ExprASTPtr ParseCharLiteral();
			ExprASTPtr ParseParenExpr();
			ExprASTPtr ParseIdentifierExpr();
			StmtASTPtr ParseCompoundStatement();

			StmtASTPtr ParseIfStatement();
			StmtASTPtr ParseWhileStatement();
			StmtASTPtr ParseBreakStatement();
			StmtASTPtr ParseContinueStatement();
			StmtASTPtr ParseReturnStatement();

			ExprASTPtr ParseStringLiteral();

			ExprASTPtr ParseBoolLiteral(bool isTrue);

			StmtASTPtr ParseExpressionStatement();

			StmtASTPtr ParseDeclStmt();

			/// \brief moses对于自定义类型支持“列表初始化”
			/// 例如： 
			/// var num : 10;
			/// var num = {num, {10, 10 * num}, false}
			/// num的类型如下：
			/// class AnonymousType
			///	{
			///		var : int;
			///		class 
			///		{
			///			var : int;
			///			var : int;
			///		}
			///		var : bool;
			/// }
			ExprASTPtr ParseAnonymousInitExpr();

			ExprASTPtr ParseExpression();

			ExprASTPtr ParseWrappedUnaryExpression();

			ExprASTPtr ParsePostfixExpression();

			ExprASTPtr ParsePostfixExpressionSuffix(ExprASTPtr);

			ExprASTPtr ParsePrimaryExpr();

			ExprASTPtr ParseBinOpRHS(OperatorPrec::Level MinPrec, ExprASTPtr lhs);

			ExprASTPtr ParseCallExpr(Token tok);

			DeclASTPtr ParseVarDecl();

			StmtASTPtr ParseFunctionDefinition();

			StmtASTPtr ParseFunctionStatementBody();

			StmtASTPtr ParseCompoundStatementBody();

			std::vector<ParmDeclPtr> ParseParameterList();

			UnpackDeclPtr ParseUnpackDecl();

			ParmDeclPtr ParseParmDecl();

			StmtASTPtr ParseClassBody();

			DeclASTPtr ParseClassDecl();

			std::shared_ptr<AnonymousType> ParseAnony();
			
		public:
			std::shared_ptr<Scope> getCurScope() const { Actions.getCurScope(); }
			std::vector<StmtASTPtr> getAST() const { return AST; }
		private:
			// Helper Functions.
			bool expectToken(tok::TokenValue value, const std::string& lexem, bool advanceToNextToken) const;
			bool validateToken(tok::TokenValue value) const;
			bool validateToken(tok::TokenValue value, bool advanceToNextToken) const;
			void errorReport(const std::string& msg) const;
			/// \brief Use "panic mode" to achieve syntax error recovery.
			void syntaxErrorRecovery(ParseContext::context context);
		};
	}
}
#endif