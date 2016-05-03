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
			if (scan.getToken().getValue() == tok::TokenValue::FILE_EOF)
			{
				AST.clear();
				return AST;
			}

			// 在TranslationUnit开始时，创建Top-Level scope.
			Actions.ActOnTranslationUnitStart();

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
					errorReport("syntax error: ", ErrorKind::PARSER_ERROR);
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

			// To Do: Perform simple semantic analysis
			// (Check whether the type of the conditional expression is a Boolean type).
			if (condition->getType()->getKind() != TypeKind::BOOL)
			{
				errorReport("Conditional expression is not a boolean type.", ErrorKind::SEMA_error);
			}

			// 这里实行一种简单的恢复策略，假装')'存在
			// expectToken(tok::TokenValue::PUNCTUATOR_Right_Paren, ")", true);

			// Perform simple semantic analysis for then part(Create new scope).
			Actions.ActOnCompoundStmt();
			// parse then part and expect then statement.
			auto thenPart = ParseCompoundStatement();

			// To Do: 代码结构优化，Actions.PopScope()和PushScope()应该成对出现
			// 由于PushScope在创建新的Scope时会立刻调用PushScope(),而PopScope()
			// 只能放在此处。
			Actions.PopScope();

			if (!thenPart)
			{
				errorReport("then statement is not valid.", ErrorKind::PARSER_ERROR);
				syntaxErrorRecovery(ParseContext::context::Statement);
				return nullptr;
			}

			StmtASTPtr elsePart = nullptr;
			// 判断当前token是否为"else"
			if (validateToken(tok::TokenValue::KEYWORD_else))
			{
				// Perform simple semantic analysis for else part(Create new scope).
				Actions.ActOnCompoundStmt();

				elsePart = ParseCompoundStatement();
				if (!elsePart)
				{
					errorReport("else statement is not valid.", ErrorKind::PARSER_ERROR);
					syntaxErrorRecovery(ParseContext::context::Statement);
					return nullptr;
				}
				// To Do: 代码结构优化
				Actions.PopScope();
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

		// Note: 此处我们并没有在AST树上显示构造出ParenExpr节点，不像Clang在AST上hold住
		// 所有的信息，moses的AST只是为了进行后续的语义和代码生成而构建的，所以简洁为主。
		ExprASTPtr Parser::ParseParenExpr()
		{
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Left_Paren, "(", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			auto expr = ParseExpression();

			if (!expr)
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			// Next token is whether or not ")".
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Right_Paren, ")", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			return std::move(expr);
		}

		/// \brief ParseIdentifierExpr - 这个函数用于解析标识符，identifier.
		/// 有可能是变量引用，也有可能是函数调用，例如add();
		/// 为了实现判断是普通变量引用还是函数调用，我们需要look-ahead。如果下一个token是'('
		/// 那么需要构建CallExpr，如果不是'('，那么构建DeclRefExpr.
		/// " identifierexpr 
		///		-> identifier
		///		-> identifier arg-list "
		ExprASTPtr Parser::ParseIdentifierExpr()
		{
			// To Do: 区分开IdentifierExpr和DeclRefExpr
			// IdentifierExpr只要是标识符都可以，但是DeclRefExpr必须是变量引用，例如'class Base{};'
			// 中的'var b : Base', 是VarDecl，但是'Base'是IdentifierExpr，不是DeclRefExpr.
			auto curTok = scan.getToken();
			auto locStart = curTok.getTokenLoc();
			std::string IdName = curTok.getLexem();

			// eat identifier
			scan.getNextToken();

			if (validateToken(tok::TokenValue::PUNCTUATOR_Left_Paren))
			{
				return ParseCallExpr(curTok);
			}
			else
			{
				// Semantic analysis.(Identifier只能是VariableSymbol)
				std::shared_ptr<Type> type = Actions.ActOnDeclRefExpr(IdName);
				auto locEnd = scan.getToken().getTokenLoc();
				return std::make_unique<DeclRefExpr>(locStart, locEnd, type, IdName, nullptr);
			}
		}

		/// \brief ParseDeclStmt - 解析DeclStmt.
		StmtASTPtr Parser::ParseDeclStmt()
		{
			StmtASTPtr curStmt = nullptr;
			switch (scan.getToken().getValue())
			{
				// To Do: 需要记录const类型信息
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
					// Create new scope.
					Actions.ActOnCompoundStmt();
					bodyStmts.push_back(ParseCompoundStatement());
					// To Do: 优化代码结构
					Actions.PopScope();
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
				case tok::TokenValue::IDENTIFIER:
				case tok::TokenValue::PUNCTUATOR_Left_Paren:
					bodyStmts.push_back(ParseExpression());
				case tok::TokenValue::UO_Exclamatory:
				case tok::TokenValue::BO_Sub:
				case tok::TokenValue::INTEGER_LITERAL:
				case tok::TokenValue::BOOL_TRUE:
				case tok::TokenValue::BOOL_FALSE:
					// Note: 在CompoundStmt中单独存在的'!' '-' 'IntegerLiteral' 'true' 'false'
					// 没有任何意义. 虽然不会报错，但是不会为其构建AST节点
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
					// Check whether current scope is function scope.
					if (FunctionSymbol* funcSym = dynamic_cast<FunctionSymbol*>(Actions.getFunctionStackTop().get()))
					{
						bodyStmts.push_back(ParsereturnStatement(funcSym));
					}
					else
					{
						errorReport("Current context isn't function context.", ErrorKind::SEMA_error);
					}
					break;
				case tok::TokenValue::KEYWORD_break:
					// 检查当前环境是否是While-Context
					if (CurrentContext == ContextKind::While)
					{
						errorReport("Current context is not loop-context.", ErrorKind::SEMA_error);
						break;
					}
					bodyStmts.push_back(ParseBreakStatement());
				case tok::TokenValue::KEYWORD_continue:
					// 检查当前环境是否是While-Context
					if (CurrentContext == ContextKind::While)
					{
						errorReport("Current context is not loop-context.", ErrorKind::SEMA_error);
						break;
					}
					bodyStmts.push_back(ParseContinueStatement());
				default:
					// Handle syntax error.
					// For example, break unlikely appear in Top-Level.
					errorReport("syntax error: ", ErrorKind::PARSER_ERROR);
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

			// Perform semantic analysis.
			// (Check whether the type of the conditional expression is a boolean type).
			if (condition->getType()->getKind() != TypeKind::BOOL)
			{
				errorReport("Conditional expression is not a boolean type.", ErrorKind::SEMA_error);
			}

			// Perform semantic analysis.
			// (Create new scope for CompoundStmt.)
			Actions.ActOnCompoundStmt();

			auto compoundStmt = ParseCompoundStatement();

			// To Do: Perform semantic analysis.
			// Pop Scope.
			Actions.PopScope();
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
		/// \parm funcSym - Current function context thar parser dealing with.
		/// -------------------------------------------------------------
		/// Return Statement's Grammar as below:
		/// return-statement -> 'return' expression ? ;
		/// -------------------------------------------------------------
		StmtASTPtr Parser::ParsereturnStatement(FunctionSymbol* funcSym)
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
			auto returnExpr = ParseExpression();
			if (!expectToken(tok::TokenValue::PUNCTUATOR_Semicolon, ";", true))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}

			// Simple semantic analysis(Get return-expr type and check).
			auto retType = funcSym->getReturnType();
			if (retType->getKind() == TypeKind::VOID && !returnExpr)
			{
				errorReport("Function return type is void.", ErrorKind::SEMA_error);
			}

			// 判断returnexpr是否有效，类型是否匹配
			if (returnExpr)
			{
				if (retType->getTypeFingerPrintWithNoConst() != returnExpr->getType()->getTypeFingerPrintWithNoConst())
				{
					errorReport("Return Type and function return types do not match.", ErrorKind::SEMA_error);
				}
			}

			// To Do: 由于moses暂时不支持返回多个值，所以需要检查return expr的类型
			// 例如 a + b, b + c;
			// 或者说是return expr必须能够作为 单 右值 出现。

			return std::make_unique<ReturnStatement>(locStart, scan.getToken().getTokenLoc(),
				std::move(returnExpr));
		}

		/// \brief ParseBooleanExpression - 解析while语句的condition，解析If语句的condition
		ExprASTPtr Parser::ParseBoolenExpression()
		{
			auto locStart = scan.getToken().getTokenLoc();
			return nullptr;
		}

		/// \brief ParseStringLiteral - parse string literal.
		ExprASTPtr Parser::ParseStringLiteral()
		{
			auto locStart = scan.getToken().getTokenLoc();
			if (!expectToken(tok::TokenValue::STRING_LITERAL, "string", true))
			{
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
			}
			return std::make_unique<StringLiteral>(locStart, scan.getToken().getTokenLoc(), nullptr,
				scan.getToken().getLexem());
		}

		/// \brief ParseBoolLiteral - parse bool literal.
		ExprASTPtr Parser::ParseBoolLiteral(bool isTrue)
		{
			auto locStart = scan.getToken().getTokenLoc();
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
			auto tok = scan.getToken();
			tok::TokenValue tokKind = tok.getKind();
			auto locStart = scan.getToken().getTokenLoc();
			ExprASTPtr LHS = nullptr;
			// 代码有问题，创建表达式节点，例如： ++ ++ ++num;
			// 需要将前半部分创建好的表达式不停地传递给后面的表达式。
			if (tokKind == tok::TokenValue::BO_Sub ||
				tokKind == tok::TokenValue::UO_Exclamatory ||
				tokKind == tok::TokenValue::UO_Dec ||
				tokKind == tok::TokenValue::UO_Inc)
			{
				// consume '-' '!' '++' '--'
				// To Do: How to transfer '-' '!' '++' '--' to the expression.
				// -num + flag;
				// To Do: 其实这里是错误的，应该要创建UnaryExpression
				// -----------------------nonsense for coding-------------------
				// 设计一个好的文法可以简化Parse中的很多问题.
				// -----------------------nonsense for coding-------------------
				scan.getNextToken();
				LHS = ParseWrappedUnaryExpression();

				// To Do: 需要执行很多很多的语义分析

				// To Do: UserDefinedType暂时不能进行代码运算符重载，所以如果发现Type是用户自定义的，则报错
				if (LHS->getType()->getKind() != TypeKind::USERDEFIED)
				{
					errorReport("Type error.", ErrorKind::SEMA_error);
				}

				// '++' '--'要求LHS是左值
				if (LHS->isRValue() && (tokKind == tok::TokenValue::UO_Dec || tokKind == tok::TokenValue::UO_Inc))
				{
					errorReport("There need an lvalue.", ErrorKind::SEMA_error);
				}

				if (tokKind == tok::TokenValue::UO_Exclamatory && ( LHS->getType()->getKind() != TypeKind::BOOL ))
				{
					errorReport("Type error.", ErrorKind::SEMA_error);
				}
				if ((tokKind == tok::TokenValue::BO_Sub || 
					tokKind == tok::TokenValue::UO_Dec || 
					tokKind == tok::TokenValue::UO_Inc) && 
					(LHS->getType()->getKind() != TypeKind::INT))
				{
					errorReport("Type error", ErrorKind::SEMA_error);
				}

				// !epxr, -expr是rvalue
				// ++expr, --expr是lvalue
				Expr::ExprValueKind valueKind;
				if (tokKind == tok::TokenValue::UO_Exclamatory || tokKind== tok::TokenValue::BO_Sub)
				{
					valueKind = Expr::ExprValueKind::VK_RValue;
				}
				if (tokKind == tok::TokenValue::UO_Inc || tokKind == tok::TokenValue::UO_Dec)
				{
					valueKind = Expr::ExprValueKind::VK_LValue;
				}

				LHS = std::make_unique<UnaryExpr>(locStart, scan.getToken().getTokenLoc(), 
					LHS->getType(), tok.getLexem(), std::move(LHS), valueKind);
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

		/// ----------------------nonsense for coding-----------------
		/// There is a bug need to fix.
		/// 例如：num++返回的是rvalue( 相对应的 ++num返回的是左值)，
		/// 所以num++ ++这种形式是违法的。文法有误，'.'可以递归取，而'++'
		/// 不能递归使用。
		/// ----------------------nonsense for coding-----------------
		/// 其实'num++ ++'不能算是语法错误，应该是语义错误，因为右值不能
		/// 被赋值，所以在语法分析的过程中需要记录左右值信息。
		/// https://code.woboq.org/llvm/clang/include/clang/AST/Expr.h.html 247
		/// ----------------------nonsense for coding-----------------		
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
			std::shared_ptr<Type> type = nullptr;

			// To Do: Shit code!
			std::string MemberName;
			std::string OpName;

			// Now that the primary-expression piece of the postfix-expression has been
			// parsed, See if there are any postfix-expresison pieces here.
			// For example, B.member.mem.num;
			while (1)
			{
				switch (scan.getToken().getKind())
				{
				case tok::TokenValue::PUNCTUATOR_Member_Access:
					// postfix-expression: p-e '.'
					// Note:in moses, have no pointer
					// Eat the '.' token

					// Perform simple semantic analysis(Check Type).
					if (LHS->getType()->getKind() != TypeKind::USERDEFIED)
					{
						errorReport("Type error.", ErrorKind::SEMA_error);
					}
					scan.getNextToken();
					// Eat the identifier
					if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", false))
					{
						syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
					}

					MemberName = scan.getToken().getLexem();
					scan.getNextToken();
					// Perform simple semantic analysis(Check subtype).
					if (UserDefinedType* BaseType = dynamic_cast<UserDefinedType*>(LHS->getType().get()))
					{
						type = BaseType->getMemberType(MemberName);
					}

					LHS = std::make_unique<MemberExpr>(locStart, scan.getToken().getTokenLoc(), type,
						std::move(LHS), locStart, nullptr, MemberName);
					break;
				case tok::TokenValue::UO_Inc: // postfix-expression: postfix-expression '++'
				case tok::TokenValue::UO_Dec: // postfix-expression: postfix-expression '--'
					// To Do: 执行语义分析				
					OpName = scan.getToken().getLexem();
					// Consume the '++' or '--'
					scan.getNextToken();
					// To Do: Handle Type
					return std::make_unique<UnaryExpr>(locStart, scan.getToken().getTokenLoc(), nullptr,
						OpName, std::move(LHS), Expr::ExprValueKind::VK_RValue);
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

				if (!OpToken.isBinaryOp())
				{
					errorReport("Must be binary operator!", ErrorKind::SEMA_error);
				}

				// Semantic analysis(Type checking.)

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
				// Perform semantic and combine the LHS and RHS into LHS (e.g. build AST)
				lhs = Actions.ActOnBinaryOperator(locStart, scan.getToken().getTokenLoc(), 
					std::move(lhs), OpToken, std::move(RHS));
				
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
				errorReport("parse primary expression!", ErrorKind::PARSER_ERROR);
				syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
				break;
			}
			return nullptr;
		}

		/// \brief ParseCallExpr - Parse the call expr.
		ExprASTPtr Parser::ParseCallExpr(Token tok)
		{
			auto startLoc = tok.getTokenLoc();
			std::string funcName = tok.getLexem();

			std::vector<ExprASTPtr> Args;
			std::vector<std::shared_ptr<Type>> ParmTyps;
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
				if (tmpArg)
				{
					ParmTyps.push_back(tmpArg->getType());
					Args.push_back(std::move(tmpArg));					
				}
				else
				{
					return nullptr;
				}
			}

			// Perform simple semantic analysis
			// (Check whether the function is defined or parameter types match).
			Actions.ActOnCallExpr(funcName, ParmTyps);

			auto endLoc = scan.getToken().getTokenLoc();
			// Note: 关于设置CallExpr左右值类型有两点需要注意，（1）如果函数返回void，则为rvalue
			// （2）否则根据return-type来设置valueKind，如果return-type是内置类型则为rvalue，
			// To Do: moses拟采用内置类型值语义，用户自定义类型引用语义。
			// 但是目前全部采用值语义（类似于C/C++）
			return std::make_unique<CallExpr>(startLoc, endLoc, nullptr, tok.getLexem(),
				std::move(Args), Expr::ExprValueKind::VK_RValue, nullptr);
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

			// 'isConst' and 'isInitial' for const
			// 在moses中，const变量不需要像C++中的那样，必须初始化
			// const num : int;
			// num = 10;
			// 这两行代码在moses中也是允许的，所以对const的赋值不能全部报错
			// 如果const变量没有进行过初始化，那么对其的首次赋值就是允许的（亦即初始化），否则报错.
			// 所以需要记录const变量是否初始化的信息.
			bool isConst = false;
			bool isInitial = false;
			if (validateToken(tok::TokenValue::KEYWORD_const, false))
			{
				isConst = true;
			}
			// consume 'const' or 'var' token
			scan.getNextToken();
			if (!expectToken(tok::TokenValue::IDENTIFIER, "identifier", false))
			{
				syntaxErrorRecovery(ParseContext::context::Statement);
			}

			Token curTok = scan.getToken();
			scan.getNextToken();
			std::shared_ptr<Type> DeclType = nullptr;

			// type-annotation ':'
			if (validateToken(tok::TokenValue::PUNCTUATOR_Colon))
			{
				// type-annotation
				// check whether the built-in type.
				if (validateToken(tok::TokenValue::KEYWORD_int))
				{
					DeclType = std::make_shared<BuiltinType>(TypeKind::INT, isConst);
				}
				else if (validateToken(tok::TokenValue::KEYWORD_bool))
				{
					DeclType = std::make_shared<BuiltinType>(TypeKind::BOOL, isConst);
				}
				else if (validateToken(tok::TokenValue::IDENTIFIER, false))
				{
					// Simple semantic analysis. Check whether the user-defined type
					// 由于func定义和class定义只存在于Top-Level中，所以只需要在Top-Level Scope
					// 中进行name lookup就可以了。
					if (const ClassSymbol* classSym = dynamic_cast<ClassSymbol*>(
						Actions.getTopLevelScope()->LookupName(scan.getToken().getLexem()).get()))
					{
						DeclType = classSym->getType();
					}
					scan.getNextToken();
				}
				else
				{
					// syntax error
					errorReport("expect 'int' ot 'bool', but find " + scan.getToken().getLexem(), ErrorKind::PARSER_ERROR);
					syntaxErrorRecovery(ParseContext::context::Statement);
					// ---------------------nonsense for coding-------------------------------
					// 为了便于从错误中恢复，遇到语法错误，程序不立即中止
					// ---------------------nonsense for coding-------------------------------
					scan.getNextToken();
				}
			}
			else if (validateToken(tok::TokenValue::BO_Assign))
			{
				// 在moses中，对于普通变量记录是否初始化的信息，暂时无用
				if (isConst)
				{
					isInitial = true;
				}				
				// Parse init-expression

				// To Do: Simple semantic analysis(Type inference).
				ParseExpression();
			}
			else
			{
				// syntax error
				errorReport("expect ':' or '=', but find " + scan.getToken().getLexem(), ErrorKind::PARSER_ERROR);
				syntaxErrorRecovery(ParseContext::context::Statement);
			}

			// Perform simple semantic analysis(Create New Symbol
			// Note: DeclType包含const属性.
			Actions.ActOnVarDecl(curTok.getLexem(), DeclType, isInitial);

			return std::make_unique<VarDecl>(locStart, scan.getToken().getTokenLoc(),
				curTok.getLexem(), DeclType, isConst);
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

			CurrentContext = ContextKind::Function;

			// now we get 'func identifier' Parse identifier
			// Simple semantic analysis(Check redefinition and create new scope).
			Actions.ActOnFunctionDeclStart(name);

			// parse parameters
			auto parm = ParseParameterList();

			if (!expectToken(tok::TokenValue::PUNCTUATOR_Arrow, "->", true))
			{
				syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
			}

			std::shared_ptr<Type> returnType = nullptr;
			// expect type info
			if (validateToken(tok::TokenValue::KEYWORD_int))
			{
				returnType = std::make_shared<BuiltinType>(TypeKind::INT, false);
			}
			else if (validateToken(tok::TokenValue::KEYWORD_bool))
			{
				returnType = std::make_shared<BuiltinType>(TypeKind::BOOL, false);
			}
			else if (validateToken(tok::TokenValue::KEYWORD_void))
			{
				returnType = std::make_shared<BuiltinType>(TypeKind::VOID, false);
			}
			else if (validateToken(tok::TokenValue::IDENTIFIER, false))
			{
				// Check Userdefined type.
				if (ClassSymbol* sym = dynamic_cast<ClassSymbol*>(Actions.getCurScope()->Resolve(scan.getToken().getLexem()).get()))
				{
					returnType = sym->getType();
				}
				else
				{
					errorReport("Undefined type.", ErrorKind::SEMA_error);
				}
			}

			// To Do: 这里简单的记录到Funcition Symbol，
			// Record return type and create new scope.
			Actions.ActOnFunctionDecl(name, returnType);

			auto body = ParseFunctionStatementBody();

			// Pop parm scope.
			Actions.PopScope();
			CurrentContext = ContextKind::TopLevel;
			return std::make_unique<FunctionDecl>(locStart, scan.getToken().getTokenLoc(), name,
				std::move(parm), std::move(body), returnType);
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
			auto body = ParseCompoundStatement();

			// Pop function stack
			Actions.PopFunctionStack();

			// Pop function body'			
			Actions.PopScope();

			return std::move(body);
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
					errorReport("expect ')' or ',' but find " + scan.getToken().getLexem(), ErrorKind::PARSER_ERROR);
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
		/// To Do: handle const keyword.
		/// Note: 函数定义参照swift，parm声明时不需要'var'关键字.
		ParmDeclPtr Parser::ParseParmDecl()
		{
			auto locStart = scan.getToken().getTokenLoc();
			// Get parm name.
			std::string name = scan.getToken().getLexem();
			bool isConst = false;
			
			if (validateToken(tok::TokenValue::KEYWORD_const))
			{
				isConst = true;
			}

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

			std::shared_ptr<Type> DeclType = nullptr;
			// Handle decl type.
			if (validateToken(tok::TokenValue::KEYWORD_int, false))
			{
				DeclType = std::make_shared<BuiltinType>(TypeKind::INT, isConst);
			}
			else if (validateToken(tok::TokenValue::KEYWORD_bool, false))
			{
				DeclType = std::make_shared<BuiltinType>(TypeKind::BOOL, isConst);
			}
			else if (validateToken(tok::TokenValue::IDENTIFIER, false))
			{
				// User-defined type.
				// Type checking.
				std::string TypeLexem = scan.getToken().getLexem();
				std::shared_ptr<Symbol> result = Actions.getCurScope()->Resolve(TypeLexem);
				if (ClassSymbol* CS = dynamic_cast<ClassSymbol*>(Actions.getCurScope()->Resolve(TypeLexem).get()))
				{
					DeclType = CS->getType();
					DeclType->setConst(isConst);
				}
				else
				{
					errorReport("Undefined type.", ErrorKind::SEMA_error);
				}
			}
			else
			{
				errorParser("variable declaration error.");
			}
			// simple semantic analysis.
			Actions.ActOnParmDecl(name, DeclType);

			// consume type token - int, bool or userdefined type token.
			scan.getNextToken();
			return std::make_unique<ParameterDecl>(locStart, scan.getToken().getTokenLoc(), name, DeclType);
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

			// Set Current Context Kind.
			CurrentContext = ContextKind::Class;

			// Simple semantic analysis.
			// Check Type Redefinition and Create New Scope.
			Actions.ActOnClassDeclStart(className);

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

			// Pop Class Stack.
			Actions.PopClassStack();
			// Pop class scope.
			Actions.PopScope();
			CurrentContext = ContextKind::TopLevel;
			return std::make_unique<ClassDecl>(locStart, scan.getToken().getTokenLoc(), className,
				std::move(classBody));
		}

		// Helper Functions.
		bool Parser::expectToken(tok::TokenValue value, const std::string& tokenName,
			bool advanceToNextToken) const
		{
			if (scan.getToken().getValue() != value)
			{
				errorReport("Expected ' " + tokenName + " ', but find " + scan.getToken().getLexem(), ErrorKind::PARSER_ERROR);
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

		bool Parser::errorReport(const std::string& msg, ErrorKind kind) const
		{
			if (kind == ErrorKind::PARSER_ERROR)
				errorParser(scan.getToken().getTokenLoc().toString() + " --- " + msg);
			else if (kind == ErrorKind::SEMA_error)
				errorSema(scan.getToken().getTokenLoc().toString() + " --- " + msg);
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