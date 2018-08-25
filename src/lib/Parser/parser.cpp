//===--------------------------------parser.h-----------------------------===//
//
// This file is used to implement the parser.
//
//===---------------------------------------------------------------------===//
#include "include/Parser/parser.h"
using namespace compiler::parse;
using namespace compiler::parse::OperatorPrec;
using namespace compiler::lex;
using namespace compiler::tok;

/// \brief
static Level getBinOpPrecedence(TokenValue Kind) {
  switch (Kind) {
  case TokenValue::PUNCTUATOR_Comma:
    return Level::Comma;
  case TokenValue::BO_Mul:
  case TokenValue::BO_Div:
    return Level::Multiplicative;
  case TokenValue::BO_Add:
  case TokenValue::BO_Sub:
    return Level::Additive;
  case TokenValue::BO_LT:
  case TokenValue::BO_GT:
  case TokenValue::BO_LE:
  case TokenValue::BO_GE:
    return Level::Relational;
  case TokenValue::BO_EQ:
  case TokenValue::BO_NE:
    return Level::Equality;
  case TokenValue::BO_Assign:
  case TokenValue::BO_MulAssign:
  case TokenValue::BO_DivAssign:
  case TokenValue::BO_RemAssign:
  case TokenValue::BO_AddAssign:
  case TokenValue::BO_SubAssign:
    return Level::Assignment;
  case TokenValue::BO_And:
    return Level::LogicalAnd;
  case TokenValue::BO_Or:
    return Level::LogicalOr;
  case TokenValue::PUNCTUATOR_Member_Access:
    return Level::PointerToMember;
  default:
    return Level::Unknown;
  }
  return Level::Unknown;
}

/// \brief Parser constructor.
/// Get the first token and start parse	tokens.
Parser::Parser(Scanner &scan, Sema &sema, ASTContext &Ctx)
    : scan(scan), Ctx(Ctx), Actions(sema) {
  scan.getNextToken();
  Actions.getScannerPointer(&(this->scan));
}

ASTPtr &Parser::parse() {
  if (scan.getToken().getValue() == TokenValue::FILE_EOF) {
    AST.clear();
    return AST;
  }

  Actions.ActOnTranslationUnitStart();

  // Parser
  for (;;) {
    switch (scan.getToken().getKind()) {
      // Predict for { statement -> compound-statement }
      // Left Brace {, This represents the compound statement.
      // In Top-Level this is allowed.
    case TokenValue::PUNCTUATOR_Left_Brace:
      AST.push_back(ParseCompoundStatement());
      break;
    case TokenValue::KEYWORD_if:
      AST.push_back(ParseIfStatement());
      break;
    case TokenValue::KEYWORD_while:
      AST.push_back(ParseWhileStatement());
      break;
      // Predict for { statement -> expression-statement }
      // ;, -, !, identifier, (, INT-LITERAL, BOOL-LITERAL
    case TokenValue::PUNCTUATOR_Semicolon:
      scan.getNextToken();
      break;
    case TokenValue::BO_Sub:
    case TokenValue::IDENTIFIER:
    case TokenValue::UO_Exclamatory:
    case TokenValue::UO_Dec:
    case TokenValue::UO_Inc:
    case TokenValue::PUNCTUATOR_Left_Paren:
    case TokenValue::INTEGER_LITERAL:
    case TokenValue::BOOL_TRUE:
    case TokenValue::BOOL_FALSE:
      AST.push_back(ParseExpressionStatement());
      break;
    case TokenValue::KEYWORD_var:
    case TokenValue::KEYWORD_const:
    case TokenValue::KEYWORD_class:
      AST.push_back(ParseDeclStmt());
      break;
    case TokenValue::KEYWORD_func:
      AST.push_back(ParseFunctionDefinition());
      break;
    case TokenValue::FILE_EOF:
      std::cout << "done!" << std::endl;
      goto DONE;
    default:
      // Handle syntax error.
      // For example,break unlikely appear in Top-Level.
      errorReport("Illegal token '" + scan.getToken().getLexem() + "'.");
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
StmtASTPtr Parser::ParseIfStatement() {
  auto locStart = scan.getToken().getTokenLoc();

  // Whether the current token is the "if", match "if" and go ahead.
  if (!expectToken(TokenValue::KEYWORD_if, "if", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
    return nullptr;
  }

  // expectToken(TokenValue::PUNCTUATOR_Left_Paren, "(", true);
  // Parse condition expression and go head.
  auto condition = ParseExpression();

  if (!condition) {
    errorReport("Error occured in condition expression.");
  } else {
    // To Do: Perform simple semantic analysis
    // (Check whether the type of the conditional expression is a Boolean type).
    Actions.ActOnConditionExpr(condition->getType());
  }

  // Perform simple semantic analysis for then part(Create new scope).
  Actions.ActOnCompoundStmt();
  // parse then part and expect then statement.
  auto thenPart = ParseCompoundStatement();

  Actions.PopScope();

  if (!thenPart) {
    errorReport("then statement is not valid.");
    syntaxErrorRecovery(ParseContext::context::Statement);
    return nullptr;
  }

  StmtASTPtr elsePart = nullptr;
  // "else"
  if (validateToken(TokenValue::KEYWORD_else)) {
    // Perform simple semantic analysis for else part(Create new scope).
    Actions.ActOnCompoundStmt();

    elsePart = ParseCompoundStatement();
    if (!elsePart) {
      errorReport("else statement is not valid.");
      syntaxErrorRecovery(ParseContext::context::Statement);
      return nullptr;
    }
    // To Do:
    Actions.PopScope();
  }
  auto locEnd = scan.getToken().getTokenLoc();

  return std::make_shared<IfStatement>(locStart, locEnd, condition, thenPart,
                                       elsePart);
}

/// \brief ParseNumberExpr - number literal.
ExprASTPtr Parser::ParseNumberExpr() {
  auto curTok = scan.getToken();
  if (!expectToken(TokenValue::INTEGER_LITERAL, "integer literal", true)) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  auto locStart = curTok.getTokenLoc();
  // Get the number value.
  double numVal = strtod(curTok.getLexem().c_str(), nullptr);
  auto locEnd = scan.getToken().getTokenLoc();
  auto NumE = std::make_shared<NumberExpr>(locStart, locEnd, numVal);
  NumE->setIntType(Ctx.Int);
  return NumE;
}

/// \brief ParseCharLiteral - char literal.
ExprASTPtr Parser::ParseCharLiteral() {
  auto curTok = scan.getToken();
  if (!expectToken(TokenValue::CHAR_LITERAL, "char literal", true)) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  auto locStart = curTok.getTokenLoc();
  // Get the number value.
  auto locEnd = scan.getToken().getTokenLoc();
  auto CharE = std::make_shared<CharExpr>(locStart, locEnd, curTok.getLexem());
  CharE->setCharType(Ctx.Int);
  return CharE;
}

/// \brief ParseParenExpr - paren expr.
/// "( num + 10 )".
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

ExprASTPtr Parser::ParseParenExpr() {
  if (!expectToken(TokenValue::PUNCTUATOR_Left_Paren, "(", true)) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  auto expr = ParseExpression();

  if (!expr) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  // Next token is whether or not ")".
  if (!expectToken(TokenValue::PUNCTUATOR_Right_Paren, ")", true)) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  return expr;
}

/// \brief ParseIdentifierExpr
/// " identifierexpr
///		-> identifier
///		-> identifier arg-list "
ExprASTPtr Parser::ParseIdentifierExpr() {
  auto curTok = scan.getToken();
  auto locStart = curTok.getTokenLoc();
  std::string IdName = curTok.getLexem();

  // eat identifier
  scan.getNextToken();

  if (validateToken(TokenValue::PUNCTUATOR_Left_Paren)) {
    return ParseCallExpr(curTok);
  } else {
    // Semantic analysis.
    VarDeclPtr var = Actions.ActOnDeclRefExpr(IdName);
    auto locEnd = scan.getToken().getTokenLoc();
    return std::make_shared<DeclRefExpr>(
        locStart, locEnd, var ? var->getDeclType() : nullptr, IdName, var);
  }
}

/// \brief ParseDeclStmt - DeclStmt.
StmtASTPtr Parser::ParseDeclStmt() {
  StmtASTPtr curStmt = nullptr;
  switch (scan.getToken().getValue()) {
  case TokenValue::KEYWORD_var:
  case TokenValue::KEYWORD_const:
    curStmt = ParseVarDecl();
    break;
  case TokenValue::KEYWORD_class:
    curStmt = ParseClassDecl();
    break;
  case TokenValue::KEYWORD_func:
    break;
  default:
    break;
  }
  expectToken(TokenValue::PUNCTUATOR_Semicolon, ";", true);
  return curStmt;
}

/// \brief ParseCompoundStatement - Parse {} statement.
/// For example, if () CompoundStatement else CompoundStatement.
/// --------------------------------------------------------------
/// compound statement's Grammar as below:
///		compound-statement -> { statement* }
/// --------------------------------------------------------------
StmtASTPtr Parser::ParseCompoundStatement() {
  return ParseCompoundStatementBody();
}

/// \brief ParseCompoundStatementBody.
StmtASTPtr Parser::ParseCompoundStatementBody() {
  auto locStart = scan.getToken().getTokenLoc();
  expectToken(TokenValue::PUNCTUATOR_Left_Brace, "{", true);
  std::vector<StmtASTPtr> bodyStmts;
  for (;;) {
    StmtASTPtr currentASTPtr = nullptr;
    if (validateToken(TokenValue::PUNCTUATOR_Right_Brace, false)) {
      break;
    }
    // Use the Predicts Sets to decide which function to call.
    // This switch use to handle top level statements and definition.
    switch (scan.getToken().getKind()) {
    case TokenValue::PUNCTUATOR_Left_Brace:
      // Create new scope.
      Actions.ActOnCompoundStmt();
      bodyStmts.push_back(ParseCompoundStatement());
      Actions.PopScope();
      break;
    case TokenValue::KEYWORD_if:
      bodyStmts.push_back(ParseIfStatement());
      break;
    case TokenValue::KEYWORD_while:
      bodyStmts.push_back(ParseWhileStatement());
      break;
    case TokenValue::PUNCTUATOR_Semicolon:
      scan.getNextToken();
      break;
    case TokenValue::IDENTIFIER:
    case TokenValue::PUNCTUATOR_Left_Paren:
    case TokenValue::UO_Exclamatory:
    case TokenValue::BO_Sub:
    case TokenValue::INTEGER_LITERAL:
    case TokenValue::BOOL_TRUE:
    case TokenValue::BOOL_FALSE:
      bodyStmts.push_back(ParseExpression());
      break;
    case TokenValue::KEYWORD_var:
    case TokenValue::KEYWORD_const:
    case TokenValue::KEYWORD_class:
      bodyStmts.push_back(ParseDeclStmt());
      // case TokenValue::KEYWORD_func:
      //	ParseFunctionDefinition();
      break;
    case TokenValue::KEYWORD_return:
      // Check whether current scope is function scope.
      bodyStmts.push_back(ParseReturnStatement());
      break;
    case TokenValue::KEYWORD_break:
      Actions.ActOnBreakAndContinueStmt(CurrentContext == ContextKind::While);
      bodyStmts.push_back(ParseBreakStatement());
      break;
    case TokenValue::KEYWORD_continue:
      Actions.ActOnBreakAndContinueStmt(CurrentContext == ContextKind::While);
      bodyStmts.push_back(ParseContinueStatement());
    default:
      // Handle syntax error.
      // For example, break unlikely appear in Top-Level.
      errorReport("Illegal token.");
      syntaxErrorRecovery(ParseContext::context::CompoundStatement);
      break;
    }
  }

  if (!expectToken(TokenValue::PUNCTUATOR_Right_Brace, "}", true)) {
    syntaxErrorRecovery(ParseContext::context::CompoundStatement);
  }
  return std::make_shared<CompoundStmt>(locStart, scan.getToken().getTokenLoc(),
                                        bodyStmts);
}

/// \brief ParseWhileStatement - while.
StmtASTPtr Parser::ParseWhileStatement() {
  auto locStart = scan.getToken().getTokenLoc();

  // Shit Code!
  auto oldContext = CurrentContext;
  CurrentContext = ContextKind::While;

  // consume 'while'.
  scan.getNextToken();
  auto condition = ParsePrimaryExpr();

  if (!condition || !(condition->getType())) {
    errorReport("Error occured in condition expression.");
  }
  // Perform semantic analysis.
  // (Check whether the type of the conditional expression is a boolean type).
  Actions.ActOnConditionExpr(condition->getType());

  // Perform semantic analysis.
  // (Create new scope for CompoundStmt.)
  Actions.ActOnCompoundStmt();

  auto compoundStmt = ParseCompoundStatement();

  // To Do: Perform semantic analysis.
  // Pop Scope.
  Actions.PopScope();

  // Shit code!
  CurrentContext = oldContext;

  auto locEnd = scan.getToken().getTokenLoc();
  return std::make_shared<WhileStatement>(locStart, locEnd, condition,
                                          compoundStmt);
}

/// \brief ParseBreakStatement - break statement.
StmtASTPtr Parser::ParseBreakStatement() {
  auto locStart = scan.getToken().getTokenLoc();
  scan.getNextToken();
  // parse break statement
  if (!expectToken(TokenValue::PUNCTUATOR_Semicolon, ";", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }
  auto locEnd = scan.getToken().getTokenLoc();
  return std::make_shared<BreakStatement>(locStart, locEnd);
}

/// \brief ParseContinueStatement - continue statement.
StmtASTPtr Parser::ParseContinueStatement() {
  auto locStart = scan.getToken().getTokenLoc();
  if (!expectToken(TokenValue::KEYWORD_continue, "continue", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }
  // parse continue statement
  if (!expectToken(TokenValue::PUNCTUATOR_Semicolon, ";", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }
  auto locEnd = scan.getToken().getTokenLoc();
  return std::make_shared<ContinueStatement>(locStart, locEnd);
}

/// \brief ParsereturnStatement() - return statement.
/// \parm funcSym - Current function context thar parser dealing with.
/// -------------------------------------------------------------
/// Return Statement's Grammar as below:
/// return-statement -> 'return' expression ? ;
/// return-statement -> 'return' anonymous-initial ? ;
/// -------------------------------------------------------------
StmtASTPtr Parser::ParseReturnStatement() {
  auto locStart = scan.getToken().getTokenLoc();
  if (!expectToken(TokenValue::KEYWORD_return, "return", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }

  if (validateToken(TokenValue::PUNCTUATOR_Semicolon)) {
    return std::make_shared<ReturnStatement>(
        locStart, scan.getToken().getTokenLoc(), nullptr);
  }

  ExprASTPtr returnExpr = nullptr;
  if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
    returnExpr = ParseAnonymousInitExpr();
    /// Perform simple semantic analysis(Mainly for type checking.)
    Actions.ActOnReturnAnonymous(returnExpr->getType());
  } else {
    returnExpr = ParseExpression();
    if (!returnExpr)
      return nullptr;
    /// Perform fimple semantic analysis(Mainly for type checking.)
    Actions.ActOnReturnStmt(returnExpr->getType());
  }

  if (!expectToken(TokenValue::PUNCTUATOR_Semicolon, ";", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }
  return std::make_shared<ReturnStatement>(
      locStart, scan.getToken().getTokenLoc(), returnExpr);
}

/// \brief ParseStringLiteral - parse string literal.
ExprASTPtr Parser::ParseStringLiteral() {
  auto locStart = scan.getToken().getTokenLoc();
  if (!expectToken(TokenValue::STRING_LITERAL, "string", true)) {
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
  }
  return std::make_shared<StringLiteral>(locStart,
                                         scan.getToken().getTokenLoc(), nullptr,
                                         scan.getToken().getLexem());
}

/// \brief ParseBoolLiteral - parse bool literal.
ExprASTPtr Parser::ParseBoolLiteral(bool isTrue) {
  auto locStart = scan.getToken().getTokenLoc();
  // consume bool literal token
  scan.getNextToken();
  auto BoolL = std::make_shared<BoolLiteral>(
      locStart, scan.getToken().getTokenLoc(), isTrue);
  BoolL->setBoolType(Ctx.Bool);
  return BoolL;
}

/// \brief ParseExpression - Parse the expression.
/// --------------------------nonsense for coding-------------------------------
/// " expr1 + expr2 * expr3 + expr4 / (expr5 - 10) + true + !10 "
/// The basic idea of operator precedence parsing is to break down an expression
/// with potentially ambigous binary operators into pieces. Consider, for exam-
/// -ple, the expression "a + b + ( c + d ) * e * f + g". Operator precedence
/// parsing considers this as a stream of primary expressions separated by bi-
/// -nary opreators. As such it will first parse the leading primary expression
/// "a", then it will see the pairs [+, b] [+, (c+d)], [*, e], [*, f] and [+,
/// g]. Note that because parenthese are primary expressions, the binary
/// expression Parser doesn't need to worry about nested subexpressions like
/// (c+d) at all.
/// --------------------------nonsense for coding-------------------------------

ExprASTPtr Parser::ParseExpression() {
  auto LHS = ParseWrappedUnaryExpression();
  if (!LHS) {
    syntaxErrorRecovery(ParseContext::context::Expression);
  }

  return ParseBinOpRHS(OperatorPrec::Level::Assignment, LHS);
}

/// \brief ParseWrappedUnaryExpression - parse UnaryExpression
/// u-expression's Grammar as below.
/// "u-expression -> - u-expression | ! u-expression | ++ u-expression
///			| -- u-expression | post-expression"
/// The code like this "int num = - + + - + + num1;" is accepted.
/// fxxk expression!!!
/// Token is the key to understand Parser, not the character!!!
ExprASTPtr Parser::ParseWrappedUnaryExpression() {
  auto tok = scan.getToken();
  TokenValue tokKind = tok.getKind();
  auto locStart = scan.getToken().getTokenLoc();
  ExprASTPtr RHS = nullptr;

  if (tokKind == TokenValue::BO_Sub) {
    scan.getNextToken();
    RHS = ParseWrappedUnaryExpression();
    if (!RHS || !(RHS->getType())) {
      errorReport("Error occured in left hand expression.");
    }
    RHS = Actions.ActOnUnarySubExpr(RHS);
    RHS = std::make_shared<UnaryExpr>(locStart, scan.getToken().getTokenLoc(),
                                      RHS->getType(), tok.getLexem(), RHS,
                                      Expr::ExprValueKind::VK_RValue);
  } else if (tokKind == TokenValue::UO_Exclamatory) {
    scan.getNextToken();
    RHS = ParseWrappedUnaryExpression();
    if (!RHS || !(RHS->getType())) {
      errorReport("Error occured in left hand expression.");
    }
    RHS = Actions.ActOnUnarySubExpr(RHS);
    RHS = std::make_shared<UnaryExpr>(locStart, scan.getToken().getTokenLoc(),
                                      RHS->getType(), tok.getLexem(), RHS,
                                      Expr::ExprValueKind::VK_RValue);
  } else if (tokKind == TokenValue::UO_Dec || tokKind == TokenValue::UO_Inc) {
    scan.getNextToken();
    RHS = ParseWrappedUnaryExpression();
    if (!RHS || !(RHS->getType())) {
      errorReport("Error occured in left hand expression.");
    }
    RHS = Actions.ActOnDecOrIncExpr(RHS);
    RHS = std::make_shared<UnaryExpr>(locStart, scan.getToken().getTokenLoc(),
                                      RHS->getType(), tok.getLexem(), RHS,
                                      Expr::ExprValueKind::VK_LValue);
  } else {
    RHS = ParsePostfixExpression();
  }
  return RHS;
}

/// \brief ParsePostfixExpression
/// for example - " ++num" or "point.mem"
/// post-expression's Grammar as below.
/// "post-expression -> primary-expression
///					| primary-expression
///post-expression-tail"
/// "post-expression-tail -> . identifier post-expression-tail
///					| ++ post-expression-tail
///					| -- post-expression-tail
///					| EPSILON"

/// ----------------------nonsense for coding-----------------
/// There is a bug need to fix.
/// https://code.woboq.org/llvm/clang/include/clang/AST/Expr.h.html 247
/// ----------------------nonsense for coding-----------------
ExprASTPtr Parser::ParsePostfixExpression() {
  // First we parse the leading part of a postfix-expression.
  // Second we parse the suffix of the postfix-expression, for example '.' '++'
  ExprASTPtr LHS = ParsePrimaryExpr();
  // These can be followed by postfix-expr pieces.
  return ParsePostfixExpressionSuffix(LHS);
}

/// \brief Once the leading part of a postfix-expression is parsed, this method
/// parses any suffixes that apply.
/// moses post-expression's Grammar as below
///	"post-expression -> primary-expression
///			| post-expression.identifier
///			| post-expression ++
///			| post-expression --"
/// post-expression is eimple for now.
ExprASTPtr Parser::ParsePostfixExpressionSuffix(ExprASTPtr LHS) {
  auto locStart = scan.getToken().getTokenLoc();
  std::shared_ptr<Type> type = nullptr;

  // To Do: Shit code!
  std::string OpName;

  // Now that the primary-expression piece of the postfix-expression has been
  // parsed, See if there are any postfix-expresison pieces here.
  // For example, B.member.mem.num;
  while (1) {
    switch (scan.getToken().getKind()) {
    case TokenValue::PUNCTUATOR_Member_Access:
      // postfix-expression: p-e '.'
      // Note:in moses, have no pointer
      // Note: If LHS is nullptr, it's meaningless to analyze the subsequent.
      if (!LHS)
        return nullptr;
      scan.getNextToken();
      // Eat the identifier
      if (!expectToken(TokenValue::IDENTIFIER, "identifier", false)) {
        syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
      }

      LHS = Actions.ActOnMemberAccessExpr(LHS, scan.getToken());
      scan.getNextToken();
      break;
    case TokenValue::UO_Inc: // postfix-expression: postfix-expression '++'
    case TokenValue::UO_Dec: // postfix-expression: postfix-expression '--'
      OpName = scan.getToken().getLexem();
      LHS = Actions.ActOnDecOrIncExpr(LHS);
      // Consume the '++' or '--'
      scan.getNextToken();
      return std::make_shared<UnaryExpr>(
          locStart, scan.getToken().getTokenLoc(), LHS->getType(), OpName, LHS,
          Expr::ExprValueKind::VK_RValue);
    default: // Not a postfix-expression suffix.
      return LHS;
    }
  }
}

/// \brief ParseExpressionStatement() - This function is a transfer function
/// from statement to expression.
StmtASTPtr Parser::ParseExpressionStatement() {
  auto Expr = ParseExpression();
  // ExpressionStatement AST
  // expect ;
  if (!expectToken(TokenValue::PUNCTUATOR_Semicolon, ";", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }
  return Expr;
}

/// \brief ParseBinOpRHS - Parse the expression-tail.
/// Note: Anonymous type need specially handled.
ExprASTPtr Parser::ParseBinOpRHS(OperatorPrec::Level MinPrec, ExprASTPtr lhs) {
  auto locStart = scan.getToken().getTokenLoc();
  OperatorPrec::Level NextTokPrec =
      getBinOpPrecedence(scan.getToken().getKind());
  while (1) {
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
      return lhs;
    Token OpToken = scan.getToken();

    if (!OpToken.isBinaryOp()) {
      errorReport("Must be binary operator!");
    }

    // Handled anonymous initexpr specially.
    // num = {expr1, expr2};
    if (OpToken.getLexem() == "=") {
      scan.getNextToken();
      // Anonymous initexpr.
      if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
        ExprASTPtr RHS = ParseAnonymousInitExpr();
        return Actions.ActOnAnonymousTypeVariableAssignment(lhs, RHS);
      }
    }

    // consume the operator
    if (OpToken.getLexem() != "=")
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
    NextTokPrec = getBinOpPrecedence(scan.getToken().getKind());

    // Assignment exression are right-associative.
    bool isRightAssoc = ThisPrec == OperatorPrec::Level::Assignment;

    // -----------------------annotation from clang------------------------
    // Get the precedence of the operator to the right of the RHS. If it
    // binds more tightly with RHS than we do, evaluate it completely first.
    // -----------------------annotation from clang------------------------
    // For example, "num0 + num1 * num2", ThisPrec -> '+', NextTokPrec -> '*'
    // we will promote the precedence to '*',  and Call
    // ParserHSOfBinaryExpression() And if now the expression is "num0 = num1 =
    // num2", right association, so "num0 = (num1 = num2)", num1 is binds more
    // tightly with num2.
    if (ThisPrec < NextTokPrec || (ThisPrec == NextTokPrec && isRightAssoc)) {
      RHS = ParseBinOpRHS(
          static_cast<OperatorPrec::Level>(ThisPrec + !isRightAssoc), RHS);
      NextTokPrec = getBinOpPrecedence(scan.getToken().getKind());
    }
    // Perform semantic and combine the LHS and RHS into LHS (e.g. build AST)
    lhs = Actions.ActOnBinaryOperator(lhs, OpToken, RHS);
  }
  return lhs;
}

/// \brief This function for parsing PrimaryExpr.
/// primary-expression -> identifier | identifier arg-list | ( expression )
///			| INT-LITERAL | BOOL-LITERAL
ExprASTPtr Parser::ParsePrimaryExpr() {
  TokenValue curTokVal = scan.getToken().getValue();
  switch (curTokVal) {
  case TokenValue::IDENTIFIER:
    return ParseIdentifierExpr();
  case TokenValue::INTEGER_LITERAL:
    return ParseNumberExpr();
    // now moses0.1 only have int and bool
    // case compiler::TokenValue::REAL_LITERAL:
    //	return ParseNumberExpr();
    // case compiler::TokenValue::CHAR_LITERAL:
    //	return ParseCharLiteral();
    // case compiler::TokenValue::STRING_LITERAL:
    //	return ParseStringLitreal();
  case TokenValue::BOOL_TRUE:
    return ParseBoolLiteral(true);
  case TokenValue::BOOL_FALSE:
    return ParseBoolLiteral(false);
  case TokenValue::PUNCTUATOR_Left_Paren:
    return ParseParenExpr();
  default:
    errorReport("Error occured when parsing primary expression! Illegal token");
    syntaxErrorRecovery(ParseContext::context::PrimaryExpression);
    break;
  }
  return nullptr;
}

/// \brief ParseCallExpr - Parse the call expr.
/// arg-list -> ( ) | ( proper-arg-list )
/// proper-arg-list->arg proper-arg - list - tail
/// proper-arg-list-tail ->, arg proper-arg-list-tail | EPSILON
/// arg->expression | anonymous - initial
ExprASTPtr Parser::ParseCallExpr(Token tok) {
  auto startLoc = tok.getTokenLoc();
  std::string funcName = tok.getLexem();
  std::vector<ExprASTPtr> Args;
  std::vector<std::shared_ptr<Type>> ParmTyps;
  bool first = true;
  // [(] [param] [,parm] [,parm] [)]
  // To Do: Shit code!
  while (1) {
    // check ')'
    if (validateToken(TokenValue::PUNCTUATOR_Right_Paren)) {
      break;
    }

    if (first) {
      first = false;
    } else {
      if (!expectToken(TokenValue::PUNCTUATOR_Comma, ",", true)) {
        syntaxErrorRecovery(ParseContext::context::ParmDecl);
        continue;
      }
    }

    ExprASTPtr arg = nullptr;
    if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
      arg = ParseAnonymousInitExpr();
    } else {
      arg = ParseExpression();
    }

    if (!arg) {
      return nullptr;
    }
    ParmTyps.push_back(arg->getType());
    Args.push_back(arg);
  }

  // Perform simple semantic analysis
  // (Check whether the function is defined or parameter types match).
  FunctionDeclPtr fd = nullptr;
  auto returnType = Actions.ActOnCallExpr(funcName, ParmTyps, fd);

  auto endLoc = scan.getToken().getTokenLoc();

  if (returnType) {
    return std::make_shared<CallExpr>(
        startLoc, endLoc, returnType, tok.getLexem(), Args,
        Expr::ExprValueKind::VK_RValue, fd,
        returnType->getKind() != TypeKind::USERDEFIED);
  }
  return nullptr;
}

/// \brief ParseVarDefinition - Parse variable declaration.
/// ---------------------------------------------------------------
/// variable declaration's Grammar as below:
/// variable-declaration -> var identifier init-expression ;
///				| var identifier type-annotation ;
/// ---------------------------------------------------------------
DeclASTPtr Parser::ParseVarDecl() {
  auto locStart = scan.getToken().getTokenLoc();

  // 'isConst' and 'isInitial' for const
  bool isConst = false;
  ExprASTPtr InitExpr = nullptr;

  if (validateToken(TokenValue::KEYWORD_const, false)) {
    isConst = true;
  }
  // consume 'const' or 'var' token
  scan.getNextToken();

  if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
    /// var {int, {int, int}} = num;
    if (isConst) {
      errorReport("Unpack declaration can't have const qualifier.");
    }

    auto unpackDecl = ParseUnpackDecl(isConst);
    // complicated actions routines.
    if (!(validateToken(TokenValue::BO_Assign))) {
      errorReport("Expected '=', but find " + scan.getToken().getLexem());
    }

    auto initexpr = ParseIdentifierExpr();

    /// (3)
    return Actions.ActOnUnpackDecl(unpackDecl, initexpr->getType());
  }

  if (!validateToken(TokenValue::IDENTIFIER, false)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }

  Token curTok = scan.getToken();
  scan.getNextToken();
  std::shared_ptr<Type> DeclType = nullptr;

  // type-annotation ':'
  if (validateToken(TokenValue::PUNCTUATOR_Colon)) {
    // type-annotation
    // check whether the built-in type.
    if (validateToken(TokenValue::KEYWORD_int)) {
      DeclType = Ctx.Int;
    } else if (validateToken(TokenValue::KEYWORD_bool)) {
      DeclType = Ctx.Bool;
    } else if (validateToken(TokenValue::IDENTIFIER, false)) {
      // Simple semantic analysis. Check whether the user-defined type
      DeclType = Actions.ActOnVarDeclUserDefinedType(scan.getToken());
      scan.getNextToken();
    } else if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
      DeclType = ParseAnony();
      scan.getNextToken();
    } else {
      // syntax error
      errorReport("expect 'int' ot 'bool', but find " +
                  scan.getToken().getLexem());
      syntaxErrorRecovery(ParseContext::context::Statement);
      scan.getNextToken();
    }
  } else if (validateToken(TokenValue::BO_Assign)) {
    // Parse init-expression
    if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
      // Class initial expression.
      InitExpr = ParseAnonymousInitExpr();
      DeclType = InitExpr->getType();
    } else {
      // Normal initial expression.
      InitExpr = ParseExpression();
      DeclType = InitExpr->getType();
    }
  } else {
    // syntax error
    errorReport("expect ':' or '=', but find " + scan.getToken().getLexem());
    syntaxErrorRecovery(ParseContext::context::Statement);
  }

  auto decl =
      std::make_shared<VarDecl>(locStart, scan.getToken().getTokenLoc(),
                                curTok.getLexem(), DeclType, isConst, InitExpr);
  Actions.ActOnVarDecl(curTok.getLexem(), decl);

  return decl;
}

/// \brief This function parse unpack decl.
/// var {num, {start, end}} = num;
UnpackDeclPtr Parser::ParseUnpackDecl(bool isConst) {
  // current token is '{'
  if (!(expectToken(TokenValue::PUNCTUATOR_Left_Brace, "{", true))) {
    errorReport("Unpack declaration list must be start with '{'.");
  }
  auto startloc = scan.getToken().getTokenLoc();
  std::vector<DeclASTPtr> decls;

  while (1) {
    if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
      decls.push_back(ParseUnpackDecl(isConst));
    } else if (validateToken(TokenValue::IDENTIFIER, false)) {
      Actions.ActOnUnpackDeclElement(scan.getToken().getLexem());

      decls.push_back(std::make_shared<VarDecl>(
          startloc, scan.getToken().getTokenLoc(), scan.getToken().getLexem(),
          nullptr, isConst, nullptr));
      scan.getNextToken();
    } else {
      errorReport("Error occured in unpack declaration.");
      syntaxErrorRecovery(ParseContext::context::Statement);
    }
    if (validateToken(TokenValue::PUNCTUATOR_Right_Brace)) {
      break;
    }
    expectToken(TokenValue::PUNCTUATOR_Comma, ",", true);
  }
  return std::make_shared<UnpackDecl>(startloc, scan.getToken().getTokenLoc(),
                                      decls);
}

ExprASTPtr Parser::ParseAnonymousInitExpr() {
  auto startloc = scan.getToken().getTokenLoc();
  // consume '{'
  scan.getNextToken();
  std::vector<ExprASTPtr> initExprs;
  std::vector<std::shared_ptr<Type>> initTypes;
  while (1) {
    switch (scan.getToken().getKind()) {
    case TokenValue::BO_Sub:
    case TokenValue::UO_Dec:
    case TokenValue::UO_Exclamatory:
    case TokenValue::UO_Inc:
    case TokenValue::IDENTIFIER:
    case TokenValue::PUNCTUATOR_Left_Paren:
    case TokenValue::INTEGER_LITERAL:
    case TokenValue::BOOL_FALSE:
    case TokenValue::BOOL_TRUE:
      initExprs.push_back(ParseExpression());
      break;
    case TokenValue::PUNCTUATOR_Left_Brace:
      initExprs.push_back(ParseAnonymousInitExpr());
      break;
    default:
      // Handle syntax error.
      // For example, break unlikely appear in Top-Level.
      errorReport("Error occured when parsing compound initial expression. ");
      syntaxErrorRecovery(ParseContext::context::Statement);
    }
    if (validateToken(TokenValue::PUNCTUATOR_Right_Brace)) {
      break;
    }
    expectToken(TokenValue::PUNCTUATOR_Comma, ",", true);
  }

  unsigned size = initExprs.size();
  for (unsigned i = 0; i < size; i++) {
    initTypes.push_back(initExprs[i]->getType());
  }
  // std::make_shared<AnonymousType>(initTypes);
  return std::make_shared<AnonymousInitExpr>(
      startloc, scan.getToken().getTokenLoc(), initExprs,
      std::make_shared<AnonymousType>(initTypes));
}

/// \brief ParseFunctionDecl - Parse function declaration.
/// --------------------------------------------------------------------
/// function declaration's Grammar as below:
/// function-declaration -> func identifier para-list -> type compound-statement
/// --------------------------------------------------------------------
/// We parsed and verified that the specified Declarator is well formed.
/// If this is a K&R function, read the parameters declaration-list, then
/// start the compound-statement.
StmtASTPtr Parser::ParseFunctionDefinition() {
  auto locStart = scan.getToken().getTokenLoc();
  // consume 'func' token
  if (!expectToken(TokenValue::KEYWORD_func, "func", true)) {
    syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
  }
  auto name = scan.getToken().getLexem();
  if (!expectToken(TokenValue::IDENTIFIER, "identifier", true)) {
    syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
  }

  CurrentContext = ContextKind::Function;

  // now we get 'func identifier' Parse identifier
  // Simple semantic analysis(Check redefinition and create new scope).
  Actions.ActOnFunctionDeclStart(name);

  // parse parameters
  auto parm = ParseParameterList();

  if (!expectToken(TokenValue::PUNCTUATOR_Arrow, "->", true)) {
    syntaxErrorRecovery(ParseContext::context::FunctionDefinition);
  }

  std::shared_ptr<Type> returnType = nullptr;

  switch (scan.getToken().getKind()) {
  case TokenValue::KEYWORD_int:
    returnType = Ctx.Int;
    scan.getNextToken();
    break;
  case TokenValue::KEYWORD_bool:
    returnType = Ctx.Bool;
    scan.getNextToken();
    break;
  case TokenValue::KEYWORD_void:
    returnType = Ctx.Void;
    scan.getNextToken();
    break;
  case TokenValue::IDENTIFIER:
    returnType = Actions.ActOnReturnType(scan.getToken().getLexem());
    scan.getNextToken();
    break;
  case TokenValue::PUNCTUATOR_Left_Brace:
    returnType = ParseAnony();
    scan.getNextToken();
    break;
  default:
    errorReport("Error occured. " + scan.getToken().getLexem() +
                " isn't type.");
    syntaxErrorRecovery(ParseContext::context::ReturnType);
    break;
  }

  // Record return type and create new scope.
  Actions.ActOnFunctionDecl(name, returnType);

  auto body = ParseFunctionStatementBody();

  CurrentContext = ContextKind::TopLevel;

  auto FuncDecl = std::make_shared<FunctionDecl>(
      locStart, scan.getToken().getTokenLoc(), name, parm, body, returnType);

  Actions.getFunctionStackTop()->setFunctionDeclPointer(FuncDecl);

  // Pop function stack
  Actions.PopFunctionStack();

  // Pop parm scope.
  Actions.PopScope();

  return FuncDecl;
}

/// \brief ParseFunctionStatementBody - Parse function body.
StmtASTPtr Parser::ParseFunctionStatementBody() {
  // Temporarily call 'ParseCompoundStatement()'
  auto body = ParseCompoundStatement();

  // Pop function body'
  Actions.PopScope();

  return body;
}

/// \brief ParseParameterList - Parse function parameter list.
/// ------------------------------------------------------------
/// para-list's Grammar as below:
/// para-list -> ( proper-para-list? )
/// proper-para-list -> para-declaration ( , para-declaration) *
/// ------------------------------------------------------------
std::vector<ParmDeclPtr> Parser::ParseParameterList() {
  std::vector<ParmDeclPtr> parms;
  if (!expectToken(TokenValue::PUNCTUATOR_Left_Paren, "(", true)) {
    syntaxErrorRecovery(ParseContext::context::ParmList);
  }

  // Cycle parse of formal parameters.
  while (1) {
    if (validateToken(TokenValue::PUNCTUATOR_Right_Paren)) {
      break;
    }

    parms.push_back(ParseParmDecl());

    if (validateToken(TokenValue::PUNCTUATOR_Right_Paren)) {
      break;
    } else if (validateToken(TokenValue::PUNCTUATOR_Comma)) {
      continue;
    } else {
      errorReport("expect ')' or ',' but find " + scan.getToken().getLexem());
      syntaxErrorRecovery(ParseContext::context::ParmDecl);
    }
  }
  return parms;
}

/// \brief ParseParmDecl - Parse parameter declaration.
ParmDeclPtr Parser::ParseParmDecl() {
  auto locStart = scan.getToken().getTokenLoc();
  // Get parm name.
  std::string name = scan.getToken().getLexem();
  bool isConst = false;

  if (validateToken(TokenValue::KEYWORD_const)) {
    isConst = true;
  }

  if (!expectToken(TokenValue::IDENTIFIER, "identifier", true)) {
    syntaxErrorRecovery(ParseContext::context::ParmDecl);
    return nullptr;
  }

  if (!expectToken(TokenValue::PUNCTUATOR_Colon, ":", true)) {
    syntaxErrorRecovery(ParseContext::context::ParmDecl);
    return nullptr;
  }

  std::shared_ptr<Type> DeclType = nullptr;
  // Handle decl type.
  if (validateToken(TokenValue::KEYWORD_int)) {
    DeclType = Ctx.Int;
  } else if (validateToken(TokenValue::KEYWORD_bool)) {
    DeclType = Ctx.Bool;
  } else if (validateToken(TokenValue::IDENTIFIER, false)) {
    // Type checking.
    DeclType = Actions.ActOnParmDeclUserDefinedType(scan.getToken());
    // DeclType->setConst(isConst);
    // consume user defined type(identifier).
    scan.getNextToken();
  } else if (validateToken(TokenValue::PUNCTUATOR_Left_Brace, false)) {
    DeclType = ParseAnony();
    scan.getNextToken();
  } else {
    errorReport("variable declaration error.");
  }

  auto parm = std::make_shared<ParameterDecl>(
      locStart, scan.getToken().getTokenLoc(), name, isConst, DeclType);
  // simple semantic analysis.
  Actions.ActOnParmDecl(name, parm);

  return parm;
}

/// \brief ParseClassDecl - Parse class declaration.
/// ---------------------------------------------------------
/// class decl's Grammar as below.
/// class-declaration -> class identifier class-body;
/// class-body -> { class-member }
/// class-member -> declaration-statement class-member |
///			function-definition class-member | EPSILON
/// ---------------------------------------------------------
DeclASTPtr Parser::ParseClassDecl() {
  auto locStart = scan.getToken().getTokenLoc();
  if (!expectToken(TokenValue::KEYWORD_class, "class", true)) {
    syntaxErrorRecovery(ParseContext::context::Statement);
  }

  std::string className = scan.getToken().getLexem();
  if (!expectToken(TokenValue::IDENTIFIER, "identifier", true)) {
    // class identifier '{' }
    syntaxErrorRecovery(ParseContext::context::ParmList);
  }

  // Set Current Context Kind.
  CurrentContext = ContextKind::Class;

  // Simple semantic analysis.
  // Check Type Redefinition and Create New Scope.
  Actions.ActOnClassDeclStart(className);

  std::vector<StmtASTPtr> classBody;
  auto classBodyStart = scan.getToken().getTokenLoc();
  expectToken(TokenValue::PUNCTUATOR_Left_Brace, "{", true);

  for (;;) {
    // parse class member
    switch (scan.getToken().getKind()) {
    case TokenValue::KEYWORD_const:
    case TokenValue::KEYWORD_class:
    case TokenValue::KEYWORD_var:
      classBody.push_back(ParseDeclStmt());
      break;
    case TokenValue::PUNCTUATOR_Semicolon:
      scan.getNextToken();
      break;
    default:
      syntaxErrorRecovery(ParseContext::context::Statement);
      scan.getNextToken();
      break;
    }
    if (validateToken(TokenValue::PUNCTUATOR_Right_Brace, false)) {
      break;
    }
  }

  if (!expectToken(TokenValue::PUNCTUATOR_Right_Brace, "}", true)) {
    syntaxErrorRecovery(ParseContext::context::ClassBody);
  }

  // (1) get Class Stack Top.
  auto ClassSym = Actions.getClassStackTop();

  // (2) Pop Class Stack.
  Actions.PopClassStack();

  // Pop class scope.
  Actions.PopScope();
  CurrentContext = ContextKind::TopLevel;
  auto ClassD = std::make_shared<ClassDecl>(
      locStart, scan.getToken().getTokenLoc(), className,
      std::make_shared<CompoundStmt>(classBodyStart,
                                     scan.getToken().getTokenLoc(), classBody));
  Ctx.UDTypes.insert(
      std::dynamic_pointer_cast<UserDefinedType>(ClassSym->getType()));
  return ClassD;
}

std::shared_ptr<AnonymousType> Parser::ParseAnony() {
  std::vector<std::shared_ptr<Type>> types;
  auto startloc = scan.getToken().getTokenLoc();
  scan.getNextToken();
  while (1) {
    switch (scan.getToken().getKind()) {
    case TokenValue::KEYWORD_int:
      types.push_back(Ctx.Int);
      break;
    case TokenValue::KEYWORD_bool:
      types.push_back(Ctx.Bool);
      break;
    case TokenValue::PUNCTUATOR_Left_Brace:
      types.push_back(ParseAnony());
      break;
    default:
      errorReport("Parameter declaration anonymous type error.");
    }
    scan.getNextToken();
    if (validateToken(TokenValue::PUNCTUATOR_Right_Brace, false)) {
      break;
    }

    if (!(validateToken(TokenValue::PUNCTUATOR_Comma))) {
      errorReport("Expected ','.");
    }
  }
  auto anony = std::make_shared<AnonymousType>(types);
  Ctx.AnonTypes.insert(anony);
  return anony;
}

// Helper Functions.
bool Parser::expectToken(TokenValue value, const std::string &tokenName,
                         bool advanceToNextToken) const {
  if (scan.getToken().getValue() != value) {
    errorReport("Expected ' " + tokenName + " ', but find " +
                scan.getToken().getLexem());
    return false;
  }

  if (advanceToNextToken) {
    scan.getNextToken();
  }

  return true;
}

bool Parser::validateToken(TokenValue value) const {
  if (scan.getToken().getValue() != value) {
    return false;
  }
  scan.getNextToken();
  return true;
}

bool Parser::validateToken(TokenValue value, bool advanceToNextToken) const {
  if (scan.getToken().getValue() != value) {
    return false;
  }
  if (advanceToNextToken) {
    scan.getNextToken();
  }
  return true;
}

void Parser::errorReport(const std::string &msg) const {
  Ctx.isParseOrSemaSuccess = false;
  errorParser(scan.getLastToken().getTokenLoc().toString() + " --- " + msg);
}

/// \brief syntaxErrorRecovery - achieve syntax error recovery.
void Parser::syntaxErrorRecovery(ParseContext::context context) {
  std::vector<TokenValue> curSafeSymbols;
  switch (context) {
  case ParseContext::context::CompoundStatement:
    curSafeSymbols = ParseContext::StmtSafeSymbols;
    break;
  case ParseContext::context::Statement:
    // statement, if-statement, while-statement, break-statement,
    // continue-statement return-statement, expression-statement,
    // declaration-statement, variable-statement, class-declaration,
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
  auto initial = scan.getToken().getKind();
  if (initial == TokenValue::FILE_EOF) {
    return;
  }

iterate:
  auto curKind = initial;
  while (find(curSafeSymbols.begin(), curSafeSymbols.end(), curKind) ==
         curSafeSymbols.end()) {
    scan.getNextToken();
    curKind = scan.getToken().getKind();
  }

  if (initial == scan.getToken().getKind()) {
    scan.getNextToken();
    goto iterate;
  }
}