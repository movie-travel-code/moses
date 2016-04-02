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
			/// \brief 获取运算符的优先级，用于解析二元表达式
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

		/// @brief 该函数是解析整个文件的主函数，moses和C/C++不同，C/C++的top-level是一系列
		/// 的Decl，而moses的top-level是一系列的decl和stmt
		ASTPtr& Parser::parse()
		{
			// Define Sema

			if (scan.getToken().getValue() == tok::TokenValue::FILE_EOF)
			{
				AST.clear();
				return AST;
			}

			// Parser的主循环
			for (;;)
			{
				// 使用递归下降的预测分析法进行语法分析。预测集见https://github.com/movie-travel-code/moses
				// 该switch语句用于handle等层的stmt和decl.
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
			// 判断当前的token是否是"if"关键字，匹配到"if"并向前走
			if (!expectToken(tok::TokenValue::KEYWORD_if, "if", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
				return nullptr;
			}

			// 这里实行一种简单的恢复策略，假装'('存在
			// expectToken(tok::TokenValue::PUNCTUATOR_Left_Paren, "(", true);

			// Parse condition expression and go head.
			auto condition = ParseExpression();

			// 这里实行一种简单的恢复策略，假装')'存在
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
			// 判断当前token是否为"else"
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

		/// \brief ParseNumberExpr - 这个函数用于解析number literal.
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

		/// \brief ParseCharLiteral - 这个函数用于解析char literal.
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


		/// \brief ParseParenExpr - 该函数主要用于解析paren expr.
		/// 例如, "( num + 10 )".
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
			// 如果expr中某个子expr返回空指针的话，会一级一级的上传
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

		/// \brief ParseIdentifierExpr - 这个函数用于解析标识符，identifier.
		/// 如果当前的context是declaration statement，说明需要添加symbol info，如果当前
		/// context是普通场景，说明此处是变量引用，需要查询symbol table。
		/// 另外，如果是变量引用的话，也有可能是函数调用，例如add();
		/// 为了实现判断是普通变量引用还是函数调用，我们需要look-ahead。如果下一个token是'('
		/// 那么需要构建CallExpr，如果不是'('，那么构建DeclRefExpr.
		/// " identifierexpr 
		///		-> identifier
		///		-> identifier arg-list "
		ExprASTPtr Parser::ParseIdentifierExpr()
		{
			auto curTok = scan.getToken();
			auto locStart = curTok.getTokenLoc();
			std::string IdName = curTok.getLexem();
			// eat identifier
			// To Do: 执行语义分析
			scan.getNextToken();
			auto nextToken = scan.getToken();

			if (validateToken(tok::TokenValue::PUNCTUATOR_Left_Paren))
			{
				return ParseCallExpr(curTok);
			}
			else
			{
				auto locEnd = nextToken.getTokenLoc();
				// To Do: 语义分析，进行符号表的查找，并确定此次引用的具体Decl类型
				return std::make_unique<DeclRefExpr>(locStart, locEnd, scan.getToken().getLexem(),
					nullptr);
			}
		}

		/// \brief ParseDeclStmt - 解析DeclStmt.
		StmtASTPtr Parser::ParseDeclStmt()
		{
			StmtASTPtr curStmt = nullptr;
			// 根据TokenValue来进行进一步的分析
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
			// 实行简单的错误恢复策略，假装';'存在
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
			// To Do:执行语义分析，例如新建一个作用域
			return ParseCompoundStatementBody();
		}

		/// \brief ParseCompoundStatementBody - 该函数用于解析compound statement.
		StmtASTPtr Parser::ParseCompoundStatementBody()
		{
			auto locStart = scan.getToken().getTokenLoc();
			expectToken(tok::TokenValue::PUNCTUATOR_Left_Brace, "{", true);
			std::vector<StmtASTPtr> bodyStmts;
			for (;;)
			{
				StmtASTPtr currentASTPtr = nullptr;
				// 如果遇到'}'则直接退出
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
					// 现在moses并不支持闭包
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
			// 如何处理错误恢复
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Right_Brace, "}", true))
			{
				syntaxErrorRecovery(ParseContext::context::CompoundStatement);
			}
			return std::make_unique<CompoundStmt>(locStart, scan.getToken().getTokenLoc(),
				std::move(bodyStmts));
		}

		/// \brief ParseWhileStatement - 该函数用于解析while语句.
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

		/// \brief ParseBreakStatement - 解析break statement.
		/// ---------------------nonsense for coding----------------------
		/// break statement只能处于while和for里
		/// ---------------------nonsense for coding----------------------
		StmtASTPtr Parser::ParseBreakStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_break, "break", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// parse break statement最主要的是语义分析
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				// 作错误恢复处理
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<BreakStatement>(locStart, locEnd);
		}

		/// \brief ParseContinueStatement - 解析continue statement.
		/// --------------------nonsense for coding---------------------
		/// continue statement只能处于while和for里
		/// --------------------nonsense for coding---------------------
		StmtASTPtr Parser::ParseContinueStatement()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::KEYWORD_continue, "continue", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			// parse continue statement最主要的是语义分析
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			auto locEnd = scan.getToken().getTokenLoc();
			return std::make_unique<ContinueStatement>(locStart, locEnd);
		}

		/// \brief ParsereturnStatement() - 解析return statement.
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
			// 如果是"return;"，则直接返回
			if (validateToken(tok::TokenValue::PUNCTUATOR_Semicolon))
			{
				return std::make_unique<ReturnStatement>(locStart, scan.getToken().getTokenLoc(),
					nullptr);
			}
			// To Do: 语义分析
			auto returnExpr = ParseExpression();
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			return std::make_unique<ReturnStatement>(locStart, scan.getToken().getTokenLoc(),
				std::move(returnExpr));
		}

		/// \brief ParseBooleanExpression - 解析while语句的condition，解析If语句的condition
		ExprASTPtr Parser::ParseBoolenExpression()
		{
			auto locStart = scan.getToken().getTokenLoc();
			// To Do: 语义分析，进行类型检查，判断condition是否是bool expression
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
			// To Do: 语义分析
			// consume bool literal token
			scan.getNextToken();
			return std::make_unique<BoolLiteral>(locStart, scan.getToken().getTokenLoc(), isTrue);
		}

		/// \brief ParseExpression - Parse the expression.
		/// 当token为 '-' '!' 'identifier' '(' 'INTLITERAL' 和 'BOOLLITERAL'的时候才会进行到该函数。
		/// --------------------------nonsense for coding-------------------------------
		/// 表达式是一个可以递归的文法单元，所以如何处理其中的优先级和结合性是一个很重要的问题
		/// " expr1 + expr2 * expr3 + expr4 / (expr5 - 10) + true + !10 "
		/// 如何考虑到从左至右的结合性，并且照顾到优先级。
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
		/// primary expression应该包含哪些表达式，是否包括 ( expression ), -identifier, 
		/// !identifier等等。注意表达式中expression + - identifier，是可以被接受的，这里会将
		/// -identifier识别为一个单表达式。但是类似于这种 " expr1 -- expr2 "是不会被允许的，
		/// 因为这里会将--识别后增运算符，所以会出现这个问题。其实这些问题无需担心，因为这些
		/// 已经被识别成token。
		/// --------------------------nonsense for coding-------------------------------

		ExprASTPtr Parser::ParseExpression()
		{
			auto LHS = ParseWrappedUnaryExpression();
			if (!LHS)
			{
				syntaxErrorRecovery(ParseContext::context::Expression);
			}

			// 将优先级初始化为最低的 "Assignment"，直到遇到比它更低的优先级，即退出"expression"的解析
			// 例如：num0 * num1 - num2 / num3 - 5;
			// ';'就是最低的优先级，unknown
			// " num = num0 * num1 - num2 / num3 - 5;"
			// 开始通过ParseWarppedUnaryExpression()解析掉num0，然后优先级设置为 'Multiplicative' *，
			// 然后再解析掉num1，注意 '=' < '*'，所以将 'num0*num1' 合并成一个操作数继续ahead. 中间还
			// 掺杂着相关的语义分析，最终遇到了优先级最低的';'结束，语义分析也结束
			// ------------------------nonsense for coding--------------------------------
			// 整个过程有点儿像超级玛丽。
			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			//		遇到'*'，将最低的优先级提升_______（在这上面运行知道优先级降低）____
			// (优先级初始化为Assignmen)______|									   |__________
			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			// 优先级高的话，就跳上去运行一个新的ParseBinOpRHS()，直到遇到一个优先级更高的，或者更低的
			// 优先级，再调用一个新的ParseBinOpRHS()函数，进行解析（重点是语义分析），执行完之后
			// 将结果返回并降下来，将优先级高的一个子表达式作为一个新的RHS并和以前的合并为新的LHS，然后继续
			// 在原有的层级上解析。
			// ------------------------nonsense for coding--------------------------------
			return ParseBinOpRHS(OperatorPrec::Level::Assignment, std::move(LHS));
		}

		/// \brief ParseWrappedUnaryExpression - 用于parse UnaryExpression
		/// u-expression's Grammar as below.
		/// "u-expression -> - u-expression | ! u-expression | ++ u-expression 
		///			| -- u-expression | post-expression"
		/// The code like this "int num = - + + - + + num1;" is accepted.
		/// fxxk expression!!!
		/// Token is the key to understand Parser, not the character!!!
		/// -----------------nonsense for coding--------------------------
		/// 该函数用于解析简单的unary expression，也就是可以作为操作数的表达式
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

		/// \brief ParsePostfixExpression - 用于parse PostfixExpression
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

																  // 记录member名字
																  std::string memberName = scan.getToken().getLexem();
																  // To Do: 执行语义分析

																  // 例如获取base的type

																  // To Do: 执行语义分析
																  // Actions.ActOnStartCXXMemberRference()

																  return std::make_unique<MemberExpr>(locStart, scan.getToken().getTokenLoc(),
																	  std::move(LHS), locStart, nullptr, memberName);
				}
				case tok::TokenValue::UO_Inc: // postfix-expression: postfix-expression '++'
				case tok::TokenValue::UO_Dec: // postfix-expression: postfix-expression '--'
				{
												  // To Do: 执行语义分析				
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
			// 构造出ExpressionStatement AST
			// expect ;
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}
			return std::move(Expr);
		}

		/// \brief ParseBinOpRHS - Parse the expression-tail.
		/// Parser就是一个超级玛丽，不断跳上跳下的切换ParseBinOpRHS()，并只在同层级上执行语法解析
		/// 切换层级的时候（也就是最终返回往下跳级的时候），将结果返回，将优先级高的子表达式作为一个
		/// 整的操作数
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
				// 结合性只用于表达式中出现两个以上相同优先级的操作符的情况，用于消除歧义
				if (ThisPrec < NextTokPrec || (ThisPrec == NextTokPrec && isRightAssoc))
				{
					RHS = ParseBinOpRHS(static_cast<OperatorPrec::Level>(ThisPrec + !isRightAssoc),
						std::move(RHS));
					// 当从ParseBinOpRHS()返回的时候，下一个Op的优先级未知
					NextTokPrec = OperatorPrec::getBinOpPrecedence(scan.getToken().getKind());
				}
				// 如果没有递归调用ParseBinOpRHS()的话，NextTokPrec的优先级是固定的
				// To Do: 执行语义分析
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
					// To Do: 语义分析，检查是否是已定义的内置类型
				}
				else
				{

					// syntax error
					errorReport("expect 'int' ot 'bool', but find " + scan.getToken().getLexem());
					syntaxErrorRecovery(ParseContext::context::Statement);
					// ---------------------nonsense for coding-------------------------------
					// 为了便于从错误中恢复，遇到语法错误，程序不立即中止
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
			// To Do: 语义分析
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

			// To Do: 语义分析，记录函数名等操作

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
		/// 虽然function body和compound statement很相似，但是还有一些scope上
		/// 的区别。另外为了实现后面的函数闭包，这里将CompoundStatement和
		/// FunctionStatementBody的parse分开。
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
					// 解析完成
					break;
				}

				parms.push_back(ParseParmDecl());

				if (validateToken(tok::TokenValue::PUNCTUATOR_Right_Paren))
				{
					// 解析完成
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
			// To Do: 语义分析
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
				// 注意这个和 func identifier() '{' }相同
				syntaxErrorRecovery(ParseContext::context::ParmList);
			}

			CompoundStmtPtr classBody = nullptr;
			// 此时不需要错误恢复，直接使用简单的策略即可，即假装这里有'{'token
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
					// 错误恢复简直fuck 
					syntaxErrorRecovery(ParseContext::context::ClassBody);
					// 我们遇到';'，会停下错误恢复完成
					scan.getNextToken();
					break;
				}
				// To Do: 代码结构混乱
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
			// 根据当前的解析环境设置安全符号safe symbol
			switch (context)
			{
			case ParseContext::context::CompoundStatement:
				break;
			case ParseContext::context::Statement:
				// statement, if-statement, while-statement, break-statement, continue-statement
				// return-statement, expression-statement, declaration-statement, variable-statement,
				// class-declaration, constant-declaration
				// To Do: 设计有问题，提高效率
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
			// 如果找到SafeSymbol，则停止并返回
			while (find(curSafeSymbols.begin(), curSafeSymbols.end(), curKind) == curSafeSymbols.end())
			{
				scan.getNextToken();
				curKind = scan.getToken().getKind();
			}
		}
	}
}