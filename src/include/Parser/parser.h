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
	Comma = 1,			// ,
	Assignment = 2,		// = *= /= += -=
	LogicalOr = 3,		// ||
	LogicalAnd = 4,		// &&
	Equality = 5,		// ==, !=
	Relational = 6,		// >= <= > <
	Additive = 7,		// + -
	Multiplicative = 8, // *, /
	PointerToMember = 9 // .
};
} // namespace OperatorPrec

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

// ʹ��Follow���Ͻ����﷨����ָ��������﷨�����ĺ������ķ�û��ǿ��Ӧ�������е��ķ���û�и���
// SafeSymbols.
// StmtSafeSymbols����statement, if-statement, while-statement, break-statement,
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
	TokenValue::KEYWORD_func, TokenValue::PUNCTUATOR_Right_Brace};

static std::vector<TokenValue> CompoundStmtSafeSymbols = {
	TokenValue::KEYWORD_else, TokenValue::PUNCTUATOR_Left_Brace,
	TokenValue::KEYWORD_while, TokenValue::KEYWORD_continue,
	TokenValue::KEYWORD_if, TokenValue::KEYWORD_return, TokenValue::KEYWORD_break,
	TokenValue::KEYWORD_const, TokenValue::KEYWORD_class, TokenValue::KEYWORD_var,
	TokenValue::PUNCTUATOR_Semicolon, TokenValue::UO_Exclamatory,
	TokenValue::UO_Inc, TokenValue::UO_Dec, TokenValue::IDENTIFIER,
	TokenValue::PUNCTUATOR_Left_Paren, TokenValue::INTEGER_LITERAL,
	TokenValue::BOOL_TRUE, TokenValue::BOOL_FALSE, TokenValue::KEYWORD_func,
	TokenValue::PUNCTUATOR_Right_Brace};

// ExprSafeSymbols����expression, assignment-expression, condition-or-expression
static std::vector<TokenValue> ExprSafeSymbols = {
	TokenValue::PUNCTUATOR_Right_Paren, TokenValue::PUNCTUATOR_Semicolon,
	TokenValue::PUNCTUATOR_Left_Brace, TokenValue::PUNCTUATOR_Comma};

// u-expression post-expression
static std::vector<TokenValue> UnaryAndPostExprSafeSymbols = {
	TokenValue::BO_Mul, TokenValue::BO_Div, TokenValue::BO_Assign,
	TokenValue::BO_MulAssign, TokenValue::BO_DivAssign, TokenValue::BO_AddAssign,
	TokenValue::BO_SubAssign, TokenValue::BO_And, TokenValue::BO_AndAssign,
	TokenValue::BO_Add, TokenValue::BO_Sub, TokenValue::BO_LT, TokenValue::BO_LE,
	TokenValue::BO_GT, TokenValue::BO_GE, TokenValue::BO_EQ, TokenValue::BO_NE,
	TokenValue::BO_And, TokenValue::BO_Or, TokenValue::PUNCTUATOR_Right_Paren,
	TokenValue::PUNCTUATOR_Semicolon, TokenValue::PUNCTUATOR_Left_Brace,
	TokenValue::PUNCTUATOR_Comma};

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
	TokenValue::UO_Inc, TokenValue::UO_Dec};

// para-list
static std::vector<TokenValue> ParaListSafeSymbols = {TokenValue::PUNCTUATOR_Left_Brace};

// para-declaration
static std::vector<TokenValue> ParaDeclSafeSymbols = {TokenValue::PUNCTUATOR_Comma,
													  TokenValue::PUNCTUATOR_Right_Paren};

// arg
static std::vector<TokenValue> ArgSafeSymbols = {TokenValue::PUNCTUATOR_Comma,
												 TokenValue::PUNCTUATOR_Right_Paren};

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
	TokenValue::KEYWORD_func};

// class-body
static std::vector<TokenValue> ClassBodySafeSymbols = {TokenValue::PUNCTUATOR_Semicolon};

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
} // namespace ParseContext

/// \brief Parser - Implenment a LL(1) parser.
/// Use syntax-directed semantic analysis.
/// --------------------------nonsense for coding-------------------------------
/// parse*()�������ܸ÷��ս���ļ̳����ԣ������ظ÷��ս�����ۺ�����
/// --------------------------nonsense for coding-------------------------------
class Parser
{

	Parser(const Parser &) = delete;
	void operator=(const Parser &) = delete;

  public:
	enum class ContextKind
	{
		TopLevel,
		Function,
		Class,
		While
	};

  private:
	Scanner &scan;
	ASTPtr AST;
	ASTContext &Ctx;
	Sema &Actions;
	ContextKind CurrentContext;

  public:
	Parser(Scanner &scan, Sema &Actions, ASTContext &Ctx);
	/// \brief parse - Parse the entire file specified.
	ASTPtr &parse();

	// �﷨�����ӳ���
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

	/// \brief moses�����Զ�������֧�֡��б��ʼ����
	/// ���磺
	/// var num : 10;
	/// var num = {num, {10, 10 * num}, false}
	/// num���������£�
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

	UnpackDeclPtr ParseUnpackDecl(bool isConst = false);

	ParmDeclPtr ParseParmDecl();

	StmtASTPtr ParseClassBody();

	DeclASTPtr ParseClassDecl();

	std::shared_ptr<AnonymousType> ParseAnony();

  public:
	std::shared_ptr<Scope> getCurScope() const { return Actions.getCurScope(); }
	std::vector<StmtASTPtr> getAST() const { return AST; }

  private:
	// Helper Functions.
	bool expectToken(tok::TokenValue value, const std::string &lexem, bool advanceToNextToken) const;
	bool validateToken(tok::TokenValue value) const;
	bool validateToken(tok::TokenValue value, bool advanceToNextToken) const;
	void errorReport(const std::string &msg) const;
	/// \brief Use "panic mode" to achieve syntax error recovery.
	void syntaxErrorRecovery(ParseContext::context context);
};
} // namespace parse
} // namespace compiler
#endif