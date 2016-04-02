//===--------------------------------parser.h-----------------------------===//
// 
// This file is used to implement the parser.
// 
//===---------------------------------------------------------------------===//
#include "Parser.h"

namespace compiler
{
	namespace parse
	{
		namespace OperatorPrec
		{
			/// \brief ��ȡ����������ȼ������ڽ�����Ԫ���ʽ
			Level getBinOpPrecedence(tok::TokenValue Kind)
			{
				switch (Kind)
				{
				case compiler::tok::TokenValue::PUNCTUATOR_Comma:
					return Level::Comma;
				case compiler::tok::TokenValue::BO_Mul:
				case compiler::tok::TokenValue::BO_Div:
					return Level::Multiplicative;
				case compiler::tok::TokenValue::BO_Add:
				case compiler::tok::TokenValue::BO_Sub:
					return Level::Additive;
				case compiler::tok::TokenValue::BO_LT:
				case compiler::tok::TokenValue::BO_GT:
				case compiler::tok::TokenValue::BO_LE:
				case compiler::tok::TokenValue::BO_GE:
					return Level::Relational;
				case compiler::tok::TokenValue::BO_EQ:
				case compiler::tok::TokenValue::BO_NE:
					return Level::Equality;
				case compiler::tok::TokenValue::BO_Assign:
				case compiler::tok::TokenValue::BO_MulAssign:
				case compiler::tok::TokenValue::BO_DivAssign:
				case compiler::tok::TokenValue::BO_RemAssign:
				case compiler::tok::TokenValue::BO_AddAssign:
				case compiler::tok::TokenValue::BO_SubAssign:
					return Level::Assignment;
				case compiler::tok::TokenValue::BO_And:
					return Level::LogicalAnd;
				case compiler::tok::TokenValue::BO_Or:
					return Level::LogicalOr;
				case compiler::tok::TokenValue::PUNCTUATOR_Member_Access:
					return Level::PointerToMember;
				default:
					return Level::Unknown;
				}
				return Level::Unknown;
			}
		}


		/// \brief Parser constructor.
		/// Get the first token and start parse	tokens.
		Parser::Parser(Scanner& scan, Sema& sema) : scan(scan), Actions(sema)
		{
			scan.getNextToken();
		}

		/// @brief �ú����ǽ��������ļ�����������moses��C/C++��ͬ��C/C++��top-level��һϵ��
		/// ��Decl����moses��top-level��һϵ�е�decl��stmt
		ASTPtr& Parser::parse()
		{
			// Define Sema

			if (scan.getToken().getValue() == tok::TokenValue::FILE_EOF)
			{
				AST.clear();
				return AST;
			}

			// Parser����ѭ��
			for (;;)
			{
				// ʹ�õݹ��½���Ԥ������������﷨������Ԥ�⼯��https://github.com/movie-travel-code/moses
				// ��switch�������handle�Ȳ��stmt��decl.
				switch (scan.getToken().getKind())
				{
					// Predict for { statement -> compound-statement }
					// Left Brace {, This represents the compound statement.
					// In Top-Level this is allowed.
				case tok::TokenValue::PUNCTUATOR_Left_Brace:
					AST.push_back(ParseCompoundStatement());
					break;
				case tok::TokenValue::KEYWORD_if:
					AST.push_back(ParseIfStatement());
					break;
				case tok::TokenValue::KEYWORD_while:
					AST.push_back(ParseWhileStatement());
					break;
					// Predict for { statement -> expression-statement }
					// ;, -, !, identifier, (, INT-LITERAL, BOOL-LITERAL
				case tok::TokenValue::PUNCTUATOR_Semicolon:
					scan.getNextToken();
					break;
				case tok::TokenValue::BO_Sub:
				case tok::TokenValue::IDENTIFIER:
				case tok::TokenValue::UO_Exclamatory:
				case tok::TokenValue::PUNCTUATOR_Left_Paren:
				case tok::TokenValue::INTEGER_LITERAL:
				case tok::TokenValue::BOOL_TRUE:
				case tok::TokenValue::BOOL_FALSE:
					AST.push_back(ParseExpressionStatement());
					break;
				case tok::TokenValue::KEYWORD_var:
				case tok::TokenValue::KEYWORD_const:
				case tok::TokenValue::KEYWORD_class:
					AST.push_back(ParseDeclStmt());
					break;
				case tok::TokenValue::KEYWORD_func:
					AST.push_back(ParseFunctionDefinition());
					break;
				case tok::TokenValue::FILE_EOF:
					std::cout << "done!" << std::endl;
					goto DONE;
				default:
					// Handle syntax error.
					// For example,break unlikely appear in Top-Level.
					errorReport("syntax error: ");
					syntaxErrorRecovery(ParseContext::context::Statement);
					break;
				}
			}
		DONE:
			return AST;
		}

		/// \brief ParseIfStatement - This function mainly used to parse if statement.
		/// sample code: 
		///		if flag-expression 
		///		{
		///			// then-part.
		///		} 
		///		else 
		///		{
		///			// else part.
		///		}
		StmtASTPtr Parser::ParseIfStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();

			// Whether the current token is the "if", match "if" and go ahead.
			// �жϵ�ǰ��token�Ƿ���"if"�ؼ��֣�ƥ�䵽"if"����ǰ��
			if (!expectToken(tok::TokenValue::KEYWORD_if, "if", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
				return nullptr;
			}

			// ����ʵ��һ�ּ򵥵Ļָ����ԣ���װ'('����
			// expectToken(tok::TokenValue::PUNCTUATOR_Left_Paren, "(", true);

			// Parse condition expression and go head.
			auto condition = ParseExpression();

			// ����ʵ��һ�ּ򵥵Ļָ����ԣ���װ')'����
			// expectToken(tok::TokenValue::PUNCTUATOR_Right_Paren, ")", true);

			// parse then part and expect then statement.
			auto thenPart = ParseCompoundStatement();
			if (!thenPart)
			{
				errorReport("then statement is not valid.");
				syntaxErrorRecovery(ParseContext::context::Statement);
				return nullptr;
			}

			StmtASTPtr elsePart = nullptr;
			// �жϵ�ǰtoken�Ƿ�Ϊ"else"
			if (validateToken(tok::TokenValue::KEYWORD_else))
			{
				elsePart = ParseCompoundStatement();
				if (!elsePart)
				{
					errorReport("else statement is not valid.");
					syntaxErrorRecovery(ParseContext::context::Statement);
					return nullptr;
				}
			}
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<IfStatement>(locStart, locEnd, std::move(condition),
				std::move(thenPart), std::move(elsePart));
		}

		/// \brief ParseNumberExpr - ����������ڽ���number literal.
		ExprASTPtr Parser::ParseNumberExpr()
		{
			auto curTok = scan.getToken();
			if (!expectToken(tok::TokenValue::INTEGER_LITERAL, "integer literal", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			auto locStart = curTok.getTokenLoc();
			// Get the number value.
			double numVal = strtod(curTok.getLexem().c_str(), nullptr);
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<NumberExpr>(locStart, locEnd, numVal);
		}

		/// \brief ParseCharLiteral - ����������ڽ���char literal.
		ExprASTPtr Parser::ParseCharLiteral()
		{
			auto curTok = scan.getToken();
			if (!expectToken(tok::TokenValue::CHAR_LITERAL, "char literal", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			auto locStart = curTok.getTokenLoc();
			// Get the number value.
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<CharExpr>(locStart, locEnd, curTok.getLexem());
		}


		/// \brief ParseParenExpr - �ú�����Ҫ���ڽ���paren expr.
		/// ����, "( num + 10 )".
		/// -------------------------nonsense for coding---------------------------
		/// This function will consume the all tokens of "( expression )".	
		/// As you can see, ParseParenExpr() will call ParseExpression() and Parse-
		/// expression() possible call ParseParenExpr() too. This recursive way
		/// allows us to handle recursive grammars, and keeps each production very
		/// simple.
		/// -------------------------nonsense for coding---------------------------
		/// Note that parenthese do not cause construction of AST nodes themselves.
		/// While we could do it this way, the most important role of parentheses 
		/// are to guide the Parser and provide grouping. Once the Parser constructs
		/// the AST, parentheses are not used.
		/// " primary-expression -> identifier | identifier arg-list | ( expression )
		///					| INT-LITERAL | BOOL-LITERAL "
		/// parenExpr -> ( expression )
		ExprASTPtr Parser::ParseParenExpr()
		{
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Left_Paren, "(", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			auto expr = ParseExpression();
			// ---------------------nonsence for coding--------------------
			// ���expr��ĳ����expr���ؿ�ָ��Ļ�����һ��һ�����ϴ�
			// ------------------------------------------------------------
			if (!expr)
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			// Next token is whether or not ")".
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Right_Paren, ")", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			// return std::move(expr);
			return std::move(expr);
		}

		/// \brief ParseIdentifierExpr - ����������ڽ�����ʶ����identifier.
		/// �����ǰ��context��declaration statement��˵����Ҫ���symbol info�������ǰ
		/// context����ͨ������˵���˴��Ǳ������ã���Ҫ��ѯsymbol table��
		/// ���⣬����Ǳ������õĻ���Ҳ�п����Ǻ������ã�����add();
		/// Ϊ��ʵ���ж�����ͨ�������û��Ǻ������ã�������Ҫlook-ahead�������һ��token��'('
		/// ��ô��Ҫ����CallExpr���������'('����ô����DeclRefExpr.
		/// " identifierexpr 
		///		-> identifier
		///		-> identifier arg-list "
		ExprASTPtr Parser::ParseIdentifierExpr()
		{
			auto curTok = scan.getToken();
			auto locStart = curTok.getTokenLoc();
			std::string IdName = curTok.getLexem();
			// eat identifier
			// To Do: ִ���������
			scan.getNextToken();
			auto nextToken = scan.getToken();

			if (validateToken(tok::TokenValue::PUNCTUATOR_Left_Paren))
			{
				return ParseCallExpr(curTok);
			}
			else
			{
				auto locEnd = nextToken.getTokenLoc();
				// To Do: ������������з��ű�Ĳ��ң���ȷ���˴����õľ���Decl����
				return std::make_unique<DeclRefExpr>(locStart, locEnd, scan.getToken().getLexem(),
					nullptr);
			}
		}

		/// \brief ParseDeclStmt - ����DeclStmt.
		StmtASTPtr Parser::ParseDeclStmt()
		{
			StmtASTPtr curStmt = nullptr;
			// ����TokenValue�����н�һ���ķ���
			switch (scan.getToken().getValue())
			{
			case tok::TokenValue::KEYWORD_var:
			case tok::TokenValue::KEYWORD_const:
				curStmt = ParseVarDecl();
				break;
			case tok::TokenValue::KEYWORD_class:
				curStmt = ParseClassDecl();
				break;
			case tok::TokenValue::KEYWORD_func:
				break;
			default:
				break;
			}
			// ʵ�м򵥵Ĵ���ָ����ԣ���װ';'����
			expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true);
			return std::move(curStmt);
		}

		/// \brief ParseCompoundStatement - Parse {} statement.
		/// For example, if () CompoundStatement else CompoundStatement.
		/// --------------------------------------------------------------
		/// compound statement's Grammar as below:
		///		compound-statement -> { statement* }
		/// --------------------------------------------------------------
		StmtASTPtr Parser::ParseCompoundStatement()
		{
			// To Do:ִ����������������½�һ��������
			return ParseCompoundStatementBody();
		}

		/// \brief ParseCompoundStatementBody - �ú������ڽ���compound statement.
		StmtASTPtr Parser::ParseCompoundStatementBody()
		{
			auto locStart = scan.getToken().getTokenLoc();
			expectToken(tok::TokenValue::PUNCTUATOR_Left_Brace, "{", true);
			std::vector<StmtASTPtr> bodyStmts;
			for (;;)
			{
				StmtASTPtr currentASTPtr = nullptr;
				// �������'}'��ֱ���˳�
				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Brace, false))
				{
					break;
				}
				// Use the Predicts Sets to decide which function to call.
				// This switch use to handle top level statements and definition.
				switch (scan.getToken().getKind())
				{
				case tok::TokenValue::PUNCTUATOR_Left_Brace:
					bodyStmts.push_back(ParseCompoundStatement());
					break;
				case tok::TokenValue::KEYWORD_if:
					bodyStmts.push_back(ParseIfStatement());
					break;
				case tok::TokenValue::KEYWORD_while:
					bodyStmts.push_back(ParseWhileStatement());
					break;
				case tok::TokenValue::PUNCTUATOR_Semicolon:
					scan.getNextToken();
					break;
				case tok::TokenValue::BO_Sub:
				case tok::TokenValue::IDENTIFIER:
				case tok::TokenValue::UO_Exclamatory:
				case tok::TokenValue::PUNCTUATOR_Left_Paren:
				case tok::TokenValue::INTEGER_LITERAL:
				case tok::TokenValue::BOOL_TRUE:
				case tok::TokenValue::BOOL_FALSE:
					bodyStmts.push_back(ParseExpression());
					break;
				case tok::TokenValue::KEYWORD_var:
				case tok::TokenValue::KEYWORD_const:
				case tok::TokenValue::KEYWORD_class:
					bodyStmts.push_back(ParseDeclStmt());
					// ����moses����֧�ֱհ�
					//case tok::TokenValue::KEYWORD_func:
					//	ParseFunctionDefinition();
					break;
				case tok::TokenValue::KEYWORD_return:
					bodyStmts.push_back(ParsereturnStatement());
					break;
				default:
					// Handle syntax error.
					// For example, break unlikely appear in Top-Level.
					errorReport("syntax error: ");
					syntaxErrorRecovery(ParseContext::context::CompoundStatement);
					break;
				}
			}
			// ��δ������ָ�
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Right_Brace, "}", true))
			{
				syntaxErrorRecovery(ParseContext::context::CompoundStatement);
			}
			return std::make_unique<CompoundStmt>(locStart, scan.getToken().getTokenLoc(),
				std::move(bodyStmts));
		}

		/// \brief ParseWhileStatement - �ú������ڽ���while���.
		StmtASTPtr Parser::ParseWhileStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_while, "while", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			auto condition = ParsePrimaryExpr();
			auto compoundStmt = ParseCompoundStatement();
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<WhileStatement>(locStart, locEnd, std::move(condition),
				std::move(compoundStmt));
		}

		/// \brief ParseBreakStatement - ����break statement.
		/// ---------------------nonsense for coding----------------------
		/// break statementֻ�ܴ���while��for��
		/// ---------------------nonsense for coding----------------------
		StmtASTPtr Parser::ParseBreakStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_break, "break", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// parse break statement����Ҫ�����������
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				// ������ָ�����
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<BreakStatement>(locStart, locEnd);
		}

		/// \brief ParseContinueStatement - ����continue statement.
		/// --------------------nonsense for coding---------------------
		/// continue statementֻ�ܴ���while��for��
		/// --------------------nonsense for coding---------------------
		StmtASTPtr Parser::ParseContinueStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_continue, "continue", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// parse continue statement����Ҫ�����������
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<ContinueStatement>(locStart, locEnd);
		}

		/// \brief ParsereturnStatement() - ����return statement.
		/// -------------------------------------------------------------
		/// Return Statement's Grammar as below:
		/// return-statement -> 'return' expression ? ;
		/// -------------------------------------------------------------
		StmtASTPtr Parser::ParsereturnStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_return, "return", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// �����"return;"����ֱ�ӷ���
			if (validateToken(tok::TokenValue::PUNCTUATOR_Semicolon))
			{
				return std::make_unique<ReturnStatement>(locStart, scan.getToken().getTokenLoc(),
					nullptr);
			}
			// To Do: �������
			auto returnExpr = ParseExpression();
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			return std::make_unique<ReturnStatement>(locStart, scan.getToken().getTokenLoc(),
				std::move(returnExpr));
		}

		/// \brief ParseBooleanExpression - ����while����condition������If����condition
		ExprASTPtr Parser::ParseBoolenExpression()
		{
			auto locStart = scan.getToken().getTokenLoc();
			// To Do: ����������������ͼ�飬�ж�condition�Ƿ���bool expression
			return nullptr;
		}

		/// \brief ParseStringLiteral - parse string literal.
		ExprASTPtr Parser::ParseStringLitreal()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::STRING_LITERAL, "string", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			return std::make_unique<StringLiteral>(locStart, scan.getToken().getTokenLoc(),
				scan.getToken().getLexem());
		}

		/// \brief ParseBoolLiteral - parse bool literal.
		ExprASTPtr Parser::ParseBoolLiteral(bool isTrue)
		{
			auto locStart = scan.getToken().getTokenLoc();
			// To Do: �������
			// consume bool literal token
			scan.getNextToken();
			return std::make_unique<BoolLiteral>(locStart, scan.getToken().getTokenLoc(), isTrue);
		}

		/// \brief ParseExpression - Parse the expression.
		/// ��tokenΪ '-' '!' 'identifier' '(' 'INTLITERAL' �� 'BOOLLITERAL'��ʱ��Ż���е��ú�����
		/// --------------------------nonsense for coding-------------------------------
		/// ���ʽ��һ�����Եݹ���ķ���Ԫ��������δ������е����ȼ��ͽ������һ������Ҫ������
		/// " expr1 + expr2 * expr3 + expr4 / (expr5 - 10) + true + !10 "
		/// ��ο��ǵ��������ҵĽ���ԣ������չ˵����ȼ���
		/// The basic idea of operator precedence parsing is to break down an expression
		/// with potentially ambigous binary operators into pieces. Consider, for exam-
		/// -ple, the expression "a + b + ( c + d ) * e * f + g". Operator precedence 
		/// parsing considiers this as a stream of primary expressions separated by bi-
		/// -nary opreators. As such it will first parse the leading primary expression
		/// "a", then it will see the pairs [+, b] [+, (c+d)], [*, e], [*, f] and [+, g].
		/// Note that because parenthese are primary expressions, the binary expression
		/// Parser doesn't need to worry about nested subexpressions like (c+d) at all.
		/// --------------------------nonsense for coding-------------------------------
		/// --------------------------nonsense for coding-------------------------------
		/// primary expressionӦ�ð�����Щ���ʽ���Ƿ���� ( expression ), -identifier, 
		/// !identifier�ȵȡ�ע����ʽ��expression + - identifier���ǿ��Ա����ܵģ�����Ὣ
		/// -identifierʶ��Ϊһ�������ʽ���������������� " expr1 -- expr2 "�ǲ��ᱻ����ģ�
		/// ��Ϊ����Ὣ--ʶ���������������Ի����������⡣��ʵ��Щ�������赣�ģ���Ϊ��Щ
		/// �Ѿ���ʶ���token��
		/// --------------------------nonsense for coding-------------------------------

		ExprASTPtr Parser::ParseExpression()
		{
			auto LHS = ParseWrappedUnaryExpression();
			if (!LHS)
			{
				syntaxErrorRecovery(ParseContext::context::Expression);
			}

			// �����ȼ���ʼ��Ϊ��͵� "Assignment"��ֱ�������������͵����ȼ������˳�"expression"�Ľ���
			// ���磺num0 * num1 - num2 / num3 - 5;
			// ';'������͵����ȼ���unknown
			// " num = num0 * num1 - num2 / num3 - 5;"
			// ��ʼͨ��ParseWarppedUnaryExpression()������num0��Ȼ�����ȼ�����Ϊ 'Multiplicative' *��
			// Ȼ���ٽ�����num1��ע�� '=' < '*'�����Խ� 'num0*num1' �ϲ���һ������������ahead. �м仹
			// ��������ص�����������������������ȼ���͵�';'�������������Ҳ����
			// ------------------------nonsense for coding--------------------------------
			// ���������е���񳬼�������
			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			//		����'*'������͵����ȼ�����_______��������������֪�����ȼ����ͣ�____
			// (���ȼ���ʼ��ΪAssignmen)______|									   |__________
			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			// ���ȼ��ߵĻ���������ȥ����һ���µ�ParseBinOpRHS()��ֱ������һ�����ȼ����ߵģ����߸��͵�
			// ���ȼ����ٵ���һ���µ�ParseBinOpRHS()���������н������ص��������������ִ����֮��
			// ��������ز��������������ȼ��ߵ�һ���ӱ��ʽ��Ϊһ���µ�RHS������ǰ�ĺϲ�Ϊ�µ�LHS��Ȼ�����
			// ��ԭ�еĲ㼶�Ͻ�����
			// ------------------------nonsense for coding--------------------------------
			return ParseBinOpRHS(OperatorPrec::Level::Assignment, std::move(LHS));
		}

		/// \brief ParseWrappedUnaryExpression - ����parse UnaryExpression
		/// u-expression's Grammar as below.
		/// "u-expression -> - u-expression | ! u-expression | ++ u-expression 
		///			| -- u-expression | post-expression"
		/// The code like this "int num = - + + - + + num1;" is accepted.
		/// fxxk expression!!!
		/// Token is the key to understand Parser, not the character!!!
		/// -----------------nonsense for coding--------------------------
		/// �ú������ڽ����򵥵�unary expression��Ҳ���ǿ�����Ϊ�������ı��ʽ
		/// -----------------nonsense for coding--------------------------
		ExprASTPtr Parser::ParseWrappedUnaryExpression()
		{
			tok::TokenValue tokKind = scan.getToken().getKind();
			ExprASTPtr LHS = nullptr;
			if (tokKind == tok::TokenValue::BO_Sub ||
				tokKind == tok::TokenValue::UO_Exclamatory ||
				tokKind == tok::TokenValue::UO_Dec ||
				tokKind == tok::TokenValue::UO_Inc)
			{
				scan.getNextToken();
				LHS = ParseWrappedUnaryExpression();
			}
			else
			{
				LHS = ParsePostfixExpression();
			}
			return std::move(LHS);
		}

		/// \brief ParsePostfixExpression - ����parse PostfixExpression
		/// for example - " ++num" or "point.mem"
		/// post-expression's Grammar as below.
		/// "post-expression -> primary-expression 
		///					| primary-expression post-expression-tail"
		/// "post-expression-tail -> . identifier post-expression-tail
		///					| ++ post-expression-tail
		///					| -- post-expression-tail
		///					| EPSILON"
		ExprASTPtr Parser::ParsePostfixExpression()
		{
			// First we parse the leading part of a postfix-expression.
			// Second we parse the suffix of the postfix-expression, for example '.' '++'
			ExprASTPtr LHS = ParsePrimaryExpr();

			// These can be followed by postfix-expr pieces.
			return ParsePostfixExpressionSuffix(std::move(LHS));
		}

		/// \brief Once the leading part of a postfix-expression is parsed, this method
		/// parses any suffixes that apply.
		/// moses post-expression's Grammar as below
		///	"post-expression -> primary-expression
		///			| post-expression.identifier
		///			| post-expression ++
		///			| post-expression --"
		/// post-expression is eimple for now.
		ExprASTPtr Parser::ParsePostfixExpressionSuffix(ExprASTPtr LHS)
		{
			auto locStart = scan.getToken().getTokenLoc();
			// Now that the primary-expression piece of the postfix-expression has been
			// parsed, See if there are any postfix-expresison pieces here.
			// For example, B.member.mem.num;
			while (1)
			{
				switch (scan.getToken().getKind())
				{
				case tok::TokenValue::PUNCTUATOR_Member_Access:
				{
																  // postfix-expression: p-e '.'
																  // Note:in moses, have no pointer
																  // Eat the '.' token
																  scan.getNextToken();
																  // Eat the identifier
																  if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", true))
																  {
																	  syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
																  }

																  // ��¼member����
																  std::string memberName = scan.getToken().getLexem();
																  // To Do: ִ���������

																  // �����ȡbase��type

																  // To Do: ִ���������
																  // Actions.ActOnStartCXXMemberRference()

																  return std::make_unique<MemberExpr>(locStart, scan.getToken().getTokenLoc(),
																	  std::move(LHS), locStart, nullptr, memberName);
				}
				case tok::TokenValue::UO_Inc: // postfix-expression: postfix-expression '++'
				case tok::TokenValue::UO_Dec: // postfix-expression: postfix-expression '--'
				{
												  // To Do: ִ���������				
												  std::string OpName = scan.getToken().getLexem();
												  // Consume the '++' or '--'
												  scan.getNextToken();
												  return std::make_unique<UnaryExpr>(locStart, scan.getToken().getTokenLoc(),
													  OpName, std::move(LHS));
				}
				default:	// Not a postfix-expression suffix.
					return std::move(LHS);
				}
			}
		}

		/// \brief ParseExpressionStatement() - This function is a transfer function from statement to
		/// expression.
		StmtASTPtr Parser::ParseExpressionStatement()
		{
			auto Expr = ParseExpression();
			// �����ExpressionStatement AST
			// expect ;
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			return std::move(Expr);
		}

		/// \brief ParseBinOpRHS - Parse the expression-tail.
		/// Parser����һ�����������������������µ��л�ParseBinOpRHS()����ֻ��ͬ�㼶��ִ���﷨����
		/// �л��㼶��ʱ��Ҳ�������շ�������������ʱ�򣩣���������أ������ȼ��ߵ��ӱ��ʽ��Ϊһ��
		/// ���Ĳ�����
		ExprASTPtr Parser::ParseBinOpRHS(OperatorPrec::Level MinPrec, ExprASTPtr lhs)
		{
			auto curTok = scan.getToken();
			auto locStart = curTok.getTokenLoc();
			OperatorPrec::Level NextTokPrec = OperatorPrec::getBinOpPrecedence(curTok.getKind());
			while (1)
			{
				// --------------------annotation from clang-------------------------------
				// If this token has a lower precedence than we are allowed to parse (e.g.
				// because we are called recursively, or because the token is not a binop),
				// then we are done!
				// --------------------annotation from clang-------------------------------
				// For example, we are parsing "num = num * 10 - max + num ;"
				// The ";" is the end character for parsing expression. And we set ";" the 
				// lowest precedence.
				// The initial precedence is "="
				if (NextTokPrec < MinPrec)
					return std::move(lhs);
				Token OpToken = curTok;
				// consume the operator
				scan.getNextToken();

				// Parse another leaf here for the RHS of the operator.
				// For example, "num1 * ++num2", '++num2' is a operand
				ExprASTPtr RHS = ParseWrappedUnaryExpression();

				// Update the information after consume the operand			
				// "num1* ++num2;"
				// At first NextTokPrec -> '*', after consume '++num2', 
				// ThisPrec = NextTokPrex -> '*', and NextTokPrex -> ';'
				// -------------------annotation from clang-------------------------
				// Remember the precedence of this operator and get the precedence of 
				// the operator immediatiely to the right of the RHS.
				// -------------------annotation from clang--------------------------
				OperatorPrec::Level ThisPrec = NextTokPrec;
				NextTokPrec = OperatorPrec::getBinOpPrecedence(scan.getToken().getKind());

				// Assignment exression are right-associative.
				bool isRightAssoc = ThisPrec == OperatorPrec::Level::Assignment;

				// -----------------------annotation from clang------------------------
				// Get the precedence of the operator to the right of the RHS. If it
				// binds more tightly with RHS than we do, evaluate it completely first.
				// -----------------------annotation from clang------------------------
				// For example, "num0 + num1 * num2", ThisPrec -> '+', NextTokPrec -> '*'
				// we will promote the precedence to '*',  and Call ParserHSOfBinaryExpression()
				// And if now the expression is "num0 = num1 = num2", right association, so 
				// "num0 = (num1 = num2)", num1 is binds more tightly with num2.
				// �����ֻ���ڱ��ʽ�г�������������ͬ���ȼ��Ĳ������������������������
				if (ThisPrec < NextTokPrec || (ThisPrec == NextTokPrec && isRightAssoc))
				{
					RHS = ParseBinOpRHS(static_cast<OperatorPrec::Level>(ThisPrec + !isRightAssoc),
						std::move(RHS));
					// ����ParseBinOpRHS()���ص�ʱ����һ��Op�����ȼ�δ֪
					NextTokPrec = OperatorPrec::getBinOpPrecedence(scan.getToken().getKind());
				}
				// ���û�еݹ����ParseBinOpRHS()�Ļ���NextTokPrec�����ȼ��ǹ̶���
				// To Do: ִ���������
				// Actions.ActOnBinOp()

				// Combine the LHS and RHS into LHS (e.g. build AST)
				lhs = std::make_unique<BinaryExpr>(locStart, scan.getToken().getTokenLoc(),
					OpToken.getLexem(), std::move(lhs), std::move(RHS));

			}
			return std::move(lhs);
		}

		/// \brief This function for parsing PrimaryExpr.
		/// primary-expression -> identifier | identifier arg-list | ( expression )
		///			| INT-LITERAL | BOOL-LITERAL
		ExprASTPtr Parser::ParsePrimaryExpr()
		{
			tok::TokenValue curTokVal = scan.getToken().getValue();
			switch (curTokVal)
			{
			case compiler::tok::TokenValue::IDENTIFIER:
				return ParseIdentifierExpr();
			case compiler::tok::TokenValue::INTEGER_LITERAL:
				return ParseNumberExpr();
				// now moses0.1 only have int and bool
				//case compiler::tok::TokenValue::REAL_LITERAL:
				//	return ParseNumberExpr();
				//case compiler::tok::TokenValue::CHAR_LITERAL:
				//	return ParseCharLiteral();
				//case compiler::tok::TokenValue::STRING_LITERAL:
				//	return ParseStringLitreal();
			case compiler::tok::TokenValue::BOOL_TRUE:
				return ParseBoolLiteral(true);
			case compiler::tok::TokenValue::BOOL_FALSE:
				return ParseBoolLiteral(false);
			case compiler::tok::TokenValue::PUNCTUATOR_Left_Paren:
				return ParseParenExpr();
			default:
				errorReport("parse primary expression!");
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
				break;
			}
			return nullptr;
		}

		/// \brief ParseCallExpr - Parse the call expr.
		ExprASTPtr Parser::ParseCallExpr(Token tok)
		{
			auto startLoc = tok.getTokenLoc();

			std::vector<std::unique_ptr<Expr>> Args;
			bool first = true;
			// [(] [param] [,parm] [,parm] [)]
			while (1)
			{
				// check ')'
				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Paren))
				{
					break;
				}

				if (first)
				{
					first = false;
				}
				else
				{
					if (!expectToken(tok::TokenValue::PUNCTUATOR_Comma, ",", true))
					{
						syntaxErrorRecovery(ParseContext::context::ParmDecl);
						continue;
					}
				}

				auto tmpArg = ParseExpression();
				/*if (tmpArg)
					Args.push_back(std::move(tmpArg));
					else
					return nullptr;*/
			}
			auto endLoc = scan.getToken().getTokenLoc();
			return std::make_unique<CallExpr>(startLoc, endLoc, tok.getLexem(), std::move(Args), nullptr);
		}

		/// \brief ParseVarDefinition - Parse variable declaration.
		/// ---------------------------------------------------------------
		/// variable declaration's Grammar as below:
		/// variable-declaration -> var identifier init-expression ;
		///				| var identifier type-annotation ;
		/// ---------------------------------------------------------------
		DeclASTPtr Parser::ParseVarDecl()
		{
			auto locStart = scan.getToken().getTokenLoc();
			bool isConst = false;
			if (validateToken(tok::TokenValue::KEYWORD_const, false))
			{
				isConst = true;
			}

			// consume 'const' or 'var' token
			scan.getNextToken();
			if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}

			Token curTok = scan.getToken();
			// type-annotation ':'
			if (validateToken(tok::TokenValue::PUNCTUATOR_Colon))
			{
				// type-annotation
				// check whether the built-in type.
				if (validateToken(tok::TokenValue::KEYWORD_int))
				{
					// record type info
				}
				else if (validateToken(tok::TokenValue::KEYWORD_bool))
				{
					// read type info
				}
				else if (validateToken(tok::TokenValue::IDENTIFIER))
				{
					// check whether the user-defined type
					// To Do: �������������Ƿ����Ѷ������������
				}
				else
				{

					// syntax error
					errorReport("expect 'int' ot 'bool', but find " + scan.getToken().getLexem());
					syntaxErrorRecovery(ParseContext::context::Statement);
					// ---------------------nonsense for coding-------------------------------
					// Ϊ�˱��ڴӴ����лָ��������﷨���󣬳���������ֹ
					// ---------------------nonsense for coding-------------------------------
					scan.getNextToken();
				}
			}
			else if (validateToken(tok::TokenValue::BO_Assign))
			{
				// Parse init-expression
				ParseExpression();
			}
			else
			{
				// syntax error
				errorReport("expect ':' or '=', but find " + scan.getToken().getLexem());
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// To Do: �������
			return std::make_unique<VarDecl>(locStart, scan.getToken().getTokenLoc(), isConst);
		}

		/// \brief ParseFunctionDecl - Parse function declaration.
		/// --------------------------------------------------------------------
		/// function declaration's Grammar as below:
		/// function-declaration -> func identifier para-list -> type compound-statement
		/// --------------------------------------------------------------------
		/// We parsed and verified that the specified Declarator is well formed.
		/// If this is a K&R function, read the parameters declaration-list, then
		/// start the compound-statement.
		StmtASTPtr Parser::ParseFunctionDefinition()
		{
			auto locStart = scan.getToken().getTokenLoc();
			// consume 'func' token
			if (!expectToken(tok::TokenValue::KEYWORD_func, "func", true))
			{
				syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
			}
			auto name = scan.getToken().getLexem();
			if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", true))
			{
				syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
			}
			// now we get 'func identifier' Parse identifier

			// To Do: �����������¼�������Ȳ���

			// parse parameters
			auto parm = ParseParameterList();

			if (!expectToken(tok::TokenValue::PUNCTUATOR_Arrow, "->", true))
			{
				syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
			}

			// expect type info
			if (validateToken(tok::TokenValue::KEYWORD_int))
			{
				// record type info
			}
			else if (validateToken(tok::TokenValue::KEYWORD_bool))
			{
				// record type info
			}

			auto body = ParseFunctionStatementBody();
			return std::make_unique<FunctionDecl>(locStart, scan.getToken().getTokenLoc(), name,
				std::move(parm), std::move(body));
		}


		/// \brief ParseFunctionStatementBody - Parse function body.
		/// ----------------------nonsense for coding-----------------------
		/// ��Ȼfunction body��compound statement�����ƣ����ǻ���һЩscope��
		/// ����������Ϊ��ʵ�ֺ���ĺ����հ������ｫCompoundStatement��
		/// FunctionStatementBody��parse�ֿ���
		/// ----------------------nonsense for coding-----------------------
		StmtASTPtr Parser::ParseFunctionStatementBody()
		{
			// Temporarily call 'ParseCompoundStatement()'
			return ParseCompoundStatement();
		}

		/// \brief ParseParameterList - Parse function parameter list.
		/// ------------------------------------------------------------
		/// para-list's Grammar as below:
		/// para-list -> ( proper-para-list? )
		/// proper-para-list -> para-declaration ( , para-declaration) *
		/// ------------------------------------------------------------
		std::vector<ParmDeclPtr> Parser::ParseParameterList()
		{
			std::vector<ParmDeclPtr> parms;
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Left_Paren, "(", true))
			{
				syntaxErrorRecovery(ParseContext::context::ParmList);
			}

			// Cycle parse of formal parameters.
			while (1)
			{
				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Paren))
				{
					// �������
					break;
				}

				parms.push_back(ParseParmDecl());

				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Paren))
				{
					// �������
					break;
				}
				else if (validateToken(tok::TokenValue::PUNCTUATOR_Comma))
				{
					continue;
				}
				else
				{
					errorReport("expect ')' or ',' but find " + scan.getToken().getLexem());
					syntaxErrorRecovery(ParseContext::context::ParmDecl);
				}
			}
			return std::move(parms);
		}

		/// \brief ParseParmDecl - Parse parameter declaration.
		/// parm decl's Grammar as below.
		/// para-declaration -> identifier type-annotation
		/// type-annotation -> : type
		/// type -> int | bool
		ParmDeclPtr Parser::ParseParmDecl()
		{
			auto locStart = scan.getToken().getTokenLoc();
			// Get parm name.
			std::string name = scan.getToken().getLexem();
			if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", true))
			{
				syntaxErrorRecovery(ParseContext::context::ParmDecl);
				return nullptr;
			}

			if (!expectToken(tok::TokenValue::PUNCTUATOR_Colon, ":", true))
			{
				syntaxErrorRecovery(ParseContext::context::ParmDecl);
				return nullptr;
			}

			if (!validateToken(tok::TokenValue::KEYWORD_int) &&
				!validateToken(tok::TokenValue::KEYWORD_bool))
			{
				syntaxErrorRecovery(ParseContext::context::ParmDecl);
				return nullptr;
			}
			// To Do: �������
			// consume type token - int, bool
			scan.getNextToken();
			// To Do: record type info
			return std::make_unique<ParameterDecl>(locStart, scan.getToken().getTokenLoc(), name);
		}

		/// \brief ParseClassDecl - Parse class declaration.
		// ---------------------------------------------------------
		// class decl's Grammar as below.
		// class-declaration -> class identifier class-body;
		// class-body -> { class-member }
		// class-member -> declaration-statement class-member | 
		//			function-definition class-member | EPSILON
		// ---------------------------------------------------------
		DeclASTPtr Parser::ParseClassDecl()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_class, "class", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}

			std::string className = scan.getToken().getLexem();
			if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", true))
			{
				// class identifier '{' }
				// ע������� func identifier() '{' }��ͬ
				syntaxErrorRecovery(ParseContext::context::ParmList);
			}

			CompoundStmtPtr classBody = nullptr;
			// ��ʱ����Ҫ����ָ���ֱ��ʹ�ü򵥵Ĳ��Լ��ɣ�����װ������'{'token
			expectToken(tok::TokenValue::PUNCTUATOR_Left_Brace, "{", true);

			for (;;)
			{
				// parse class member
				switch (scan.getToken().getKind())
				{
				case tok::TokenValue::KEYWORD_const:
				case tok::TokenValue::KEYWORD_class:
				case tok::TokenValue::KEYWORD_var:
					classBody->addSubStmt(ParseDeclStmt());
					break;
				case tok::TokenValue::KEYWORD_func:
					classBody->addSubStmt(ParseFunctionDefinition());
					break;
				case tok::TokenValue::PUNCTUATOR_Left_Brace:
					break;
				default:
					// ����ָ���ֱfuck 
					syntaxErrorRecovery(ParseContext::context::ClassBody);
					// ��������';'����ͣ�´���ָ����
					scan.getNextToken();
					break;
				}
				// To Do: ����ṹ����
				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Brace, false))
				{
					break;
				}
			}

			if (!expectToken(tok::TokenValue::PUNCTUATOR_Right_Brace, "}", true))
			{
				syntaxErrorRecovery(ParseContext::context::ClassBody);
			}
			return std::make_unique<ClassDecl>(locStart, scan.getToken().getTokenLoc(), className,
				std::move(classBody));
		}

		// Helper Functions.
		bool Parser::expectToken(tok::TokenValue value, const std::string& tokenName,
			bool advanceToNextToken) const
		{
			if (scan.getToken().getValue() != value)
			{
				errorReport("Expected ' " + tokenName + " ', but find " + scan.getToken().getLexem());
				return false;
			}

			if (advanceToNextToken)
			{
				scan.getNextToken();
			}

			return true;
		}

		bool Parser::validateToken(tok::TokenValue value) const
		{
			if (scan.getToken().getValue() != value)
			{
				return false;
			}
			scan.getNextToken();
			return true;
		}

		bool Parser::validateToken(tok::TokenValue value, bool advanceToNextToken) const
		{
			if (scan.getToken().getValue() != value)
			{
				return false;
			}
			if (advanceToNextToken)
			{
				scan.getNextToken();
			}
			return true;
		}

		bool Parser::errorReport(const std::string& msg) const
		{
			errorParser(scan.getToken().getTokenLoc().toString() + " --- " + msg);
			return true;
		}

		/// \brief syntaxErrorRecovery - achieve syntax error recovery.
		void Parser::syntaxErrorRecovery(ParseContext::context context)
		{
			std::vector<tok::TokenValue> curSafeSymbols;
			// ���ݵ�ǰ�Ľ����������ð�ȫ����safe symbol
			switch (context)
			{
			case ParseContext::context::CompoundStatement:
				break;
			case ParseContext::context::Statement:
				// statement, if-statement, while-statement, break-statement, continue-statement
				// return-statement, expression-statement, declaration-statement, variable-statement,
				// class-declaration, constant-declaration
				// To Do: ��������⣬���Ч��
				curSafeSymbols = ParseContext::StmtSafeSymbols;
				break;
			case ParseContext::context::Expression:
				// expression, assignmen-expression, condition-or-expression
				curSafeSymbols = ParseContext::ExprSafeSymbols;
				break;
			case ParseContext::context::UnaryExpression:
			case ParseContext::context::PostExpression:
				curSafeSymbols = ParseContext::UnaryAndPostExprSafeSymbols;
				break;
			case ParseContext::context::ArgListExpression:
			case ParseContext::context::PrimaryExpression:
				curSafeSymbols = ParseContext::PrimaryAndArgListExprSafeSymbols;
				break;
			case ParseContext::context::ParmList:
				curSafeSymbols = ParseContext::ParaListSafeSymbols;
				break;
			case ParseContext::context::ParmDecl:
				curSafeSymbols = ParseContext::ParaDeclSafeSymbols;
				break;
			case ParseContext::context::ArgExpression:
				curSafeSymbols = ParseContext::ArgSafeSymbols;
				break;
			case ParseContext::context::FunctionDefinition:
				curSafeSymbols = ParseContext::FuncDefSafeSymbols;
				break;
			case ParseContext::context::ClassBody:
				curSafeSymbols = ParseContext::ClassBodySafeSymbols;
				break;
			default:
				break;
			}
			auto curKind = scan.getToken().getKind();
			// ����ҵ�SafeSymbol����ֹͣ������
			while (find(curSafeSymbols.begin(), curSafeSymbols.end(), curKind) == curSafeSymbols.end())
			{
				scan.getNextToken();
				curKind = scan.getToken().getKind();
			}
		}
	}
}