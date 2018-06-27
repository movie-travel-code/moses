//===------------------------------sema.cpp--------------------------------===//
//
// This file is used to implement sema.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/sema.h"
#include "../../include/Support/error.h"
using namespace compiler::sema;
using namespace compiler::lex;

using VarSymPtr = std::shared_ptr<VariableSymbol>;
using ParmSymPtr = std::shared_ptr<ParmDeclSymbol>;
using ClassSymPtr = std::shared_ptr<ClassSymbol>;
using FuncSymPtr = std::shared_ptr<FunctionSymbol>;
using UDTyPtr = std::shared_ptr<UserDefinedType>;
using AnonTyPtr = std::shared_ptr<AnonymousType>;

std::shared_ptr<Symbol> Scope::Resolve(std::string name) const {
  // Look up the name in current scope.
  for (auto item : SymbolTable) {
    if (item->getLexem() == name) {
      return item;
    }
  }
  if (Parent) {
    return Parent->Resolve(name);
  }
  return nullptr;
}

/// \biref ���������Ҫ�������ļ�ͷ����Top-Level Scope.
void Sema::ActOnTranslationUnitStart() {
  CurScope = std::make_shared<Scope>("##TranslationUnit", 0, nullptr,
                                     Scope::ScopeKind::SK_TopLevel);
  ScopeStack.push_back(CurScope);
}

/// \brief ActOnFunctionDecl - Mainly for checking function name and recording
/// function name.
void Sema::ActOnFunctionDeclStart(std::string name) {
  // Check function name.
  // Note: moses doesn't support function overload now.
  if (CurScope->Resolve(name)) {
    errorReport("Function redefinition.");
  }

  // Create new scope for function.
  unsigned CurDepth = CurScope->getDepth();
  std::shared_ptr<Scope> funcScope = std::make_shared<Scope>(
      name, CurDepth + 1, CurScope, Scope::ScopeKind::SK_Function);

  // Record this func symbol.
  std::shared_ptr<FunctionSymbol> funcsym =
      std::make_shared<FunctionSymbol>(name, nullptr, CurScope, funcScope);

  CurScope->addDef(funcsym);

  // Update CurScope.
  CurScope = funcScope;
  ScopeStack.push_back(CurScope);
  FunctionStack.push_back(funcsym);
}

/// \brief ActOnFunctionDecl - Set return type and create new scope.
void Sema::ActOnFunctionDecl(std::string name,
                             std::shared_ptr<Type> returnType) {
  getFunctionStackTop()->setReturnType(returnType);

  auto OldScope = CurScope;
  // Create new scope for function body.
  CurScope = std::make_shared<Scope>("", CurScope->getDepth() + 1, CurScope,
                                     Scope::ScopeKind::SK_Block);

  auto symbol = std::make_shared<ScopeSymbol>(CurScope, OldScope);
  OldScope->addDef(symbol);

  ScopeStack.push_back(CurScope);
}

/// \brief Action routines about IfStatement.
StmtASTPtr Sema::ActOnIfStmt(SourceLocation start, SourceLocation end,
                             ExprASTPtr condition, StmtASTPtr ThenPart,
                             StmtASTPtr ElsePart) {
  Expr *cond = condition.get();
  if (!cond) {
    // To Do: ����
  }
  return std::make_shared<IfStatement>(start, end, condition, ThenPart,
                                       ElsePart);
}

/// \brief Action routines about CompoundStatement.
void Sema::ActOnCompoundStmt() {
  auto OldScope = CurScope;
  // Create new scope for function body.
  CurScope = std::make_shared<Scope>("", CurScope->getDepth() + 1, CurScope,
                                     Scope::ScopeKind::SK_Block);

  auto symbol = std::make_shared<ScopeSymbol>(CurScope, OldScope);
  OldScope->addDef(symbol);

  ScopeStack.push_back(CurScope);
}

/// \brief Actions routines about unpack decl.
/// var {num, mem} = anony;
/// Mainly check redefintion.
bool Sema::ActOnUnpackDeclElement(std::string name) {
  // �ڵ�ǰCurScope�з���ͬ������
  if (CurScope->CheckWhetherInCurScope(name)) {
    errorReport("Error occured in unpack decl.  Variable " + name +
                " redefinition.");
    return false;
  }
  return true;
}

/// \brief Mainly check whether current context is while context.
bool Sema::ActOnBreakAndContinueStmt(bool whileContext) {
  if (whileContext) {
    errorReport("Current context is not loop-context.");
  }
  return whileContext;
}

/// \brief Mainly current whether identifier is user defined type.
std::shared_ptr<Type> Sema::ActOnReturnType(const std::string &name) const {
  // Note: ��moses�У�class��ʱ������Top-Level��
  if (ClassSymPtr sym = std::dynamic_pointer_cast<ClassSymbol>(
          ScopeStack[0]->CheckWhetherInCurScope(name))) {
    return sym->getType();
  } else {
    errorReport("Current identifier isn't user defined type.");
    return nullptr;
  }
}

void Sema::ActOnParmDecl(std::string name, ParmDeclPtr parm) {
  // Check redefinition.
  if (CurScope->CheckWhetherInCurScope(name)) {
    errorReport("Parameter redefinition.");
  }
  // Note: moses��FunctionDefinition�е�const�βΣ�Ĭ�϶���ͨ�����������е�ʵ�γ�ʼ����
  auto vsym = std::make_shared<ParmDeclSymbol>(name, CurScope,
                                               parm->getDeclType(), true, parm);

  CurScope->addDef(vsym);
  getFunctionStackTop()->addParmVariableSymbol(vsym);
}

void Sema::ActOnClassDeclStart(std::string name) {
  // check class redefinition.
  if (CurScope->CheckWhetherInCurScope(name)) {
    errorReport("Class redefinition.");
    return;
  }

  // Create new scope.
  auto ClassBodyScope = std::make_shared<Scope>(
      name, CurScope->getDepth() + 1, CurScope, Scope::ScopeKind::SK_Class);

  // Create class symbol.
  std::shared_ptr<ClassSymbol> CurSym =
      std::make_shared<ClassSymbol>(name, CurScope, ClassBodyScope);

  ClassBodyScope->setBelongToSymbolForClassScope(CurSym);
  CurScope->addDef(CurSym);
  ClassStack.push_back(CurSym);
  ScopeStack.push_back(ClassBodyScope);
  // Update CurScope.
  CurScope = ClassBodyScope;
}

/// \brief Create new variable symbol.
void Sema::ActOnVarDecl(
    std::string name,
    VarDeclPtr VD /*, std::shared_ptr<Type> declType, ExprASTPtr InitExpr*/) {
  ExprASTPtr Init = VD->getInitExpr();
  auto declType = VD->getDeclType();
  CurScope->addDef(std::make_shared<VariableSymbol>(
      name, CurScope, declType, VD->getInitExpr() ? true : false, VD));

  // To Do: moses���õ��ǽṹ���͵ȼ۵Ľṹ����Ҫ��¼Class Type����Type
  if (ClassStack.size() != 0) {
    // moses��ʱ�ڶ���class�У����ܸ�Member���ݳ�ԱInitExpr.
    if (Init) {
      errorReport("Member declaration can't have initial expression");
    }
    CurScope->getTheSymbolBelongTo()->addSubType(declType, name);
  }
}

/// \brief ��reutrn anonymous���������������
bool Sema::ActOnReturnAnonymous(std::shared_ptr<Type> type) const {
  if (!type) {
    return false;
  }
  // ���豨��ǰ���ڽ���function�Լ�return type��ʱ��������⣬�Żᵼ��Ϊ��
  // ǰ���Ѿ����������ظ�����
  if (!getFunctionStackTop() || !(getFunctionStackTop()->getReturnType())) {
    return false;
  }
  if (getFunctionStackTop()->getReturnType()->getKind() !=
      TypeKind::ANONYMOUS) {
    errorReport("Current function's return type isn't anonymous.");
    return false;
  }

  if (TypeKeyInfo::TypeKeyInfo::getHashValue(type) !=
      TypeKeyInfo::TypeKeyInfo::getHashValue(
          getFunctionStackTop()->getReturnType())) {
    errorReport("Return type not match.");
    return false;
  }
  return true;
}

bool Sema::ActOnReturnStmt(std::shared_ptr<Type> type) const {
  // To Do: ����Ч�ʽϵͣ��еĺ�����Ҫ��������
  if (!getFunctionStackTop() || !(getFunctionStackTop()->getReturnType())) {
    return false;
  }
  // �ж������Ƿ���ͬ
  // Note: moses��ʱ��֧��const��������
  if (TypeKeyInfo::TypeKeyInfo::getHashValue(type) !=
      TypeKeyInfo::TypeKeyInfo::getHashValue(
          getFunctionStackTop()->getReturnType())) {
    errorReport("Return type not match.");
    return false;
  }
  return true;
}

/// \brief Act on declaration reference.
/// Perferm name lookup and type checking.
VarDeclPtr Sema::ActOnDeclRefExpr(std::string name) {
  if (VarSymPtr vsym =
          std::dynamic_pointer_cast<VariableSymbol>(CurScope->Resolve(name))) {
    return vsym->getDecl();
  } else if (ParmSymPtr psym = std::dynamic_pointer_cast<ParmDeclSymbol>(
                 CurScope->Resolve(name))) {
    return psym->getDecl();
  } else {
    errorReport("Undefined variable.");
    return nullptr;
  }
}

/// \brief Act on Call Expr.
/// Perform name lookup and parm type checking.
std::shared_ptr<Type>
Sema::ActOnCallExpr(std::string name, std::vector<std::shared_ptr<Type>> args,
                    FunctionDeclPtr &FD) {
  std::shared_ptr<Type> ReturnType = nullptr;
  if (FuncSymPtr FuncSym = std::dynamic_pointer_cast<FunctionSymbol>(
          ScopeStack[0]->CheckWhetherInCurScope(name))) {
    ReturnType = FuncSym->getReturnType();
    // check args number and type.
    if (args.size() != FuncSym->getParmNum()) {
      errorReport("Arguments number not match.");
    }

    unsigned size = args.size();
    for (unsigned i = 0; i < size; i++) {
      if (!args[i]) {
        errorReport("The argument of index " + std::to_string(i) +
                    " is wrong.");
        continue;
      }
      // Note:
      // ���������еĴ��󣬴˴����豨����continue����
      // �����������ʽ�Ķ�·��ֵ���ԣ��������(*FuncSym)[i]Ϊ�գ�ͬʱ
      if (!(*FuncSym)[i] || !((*FuncSym)[i]->getType())) {
        continue;
      }

      if (TypeKeyInfo::TypeKeyInfo::getHashValue(args[i]) ==
          TypeKeyInfo::TypeKeyInfo::getHashValue((*FuncSym)[i]->getType())) {
        continue;
      } else {
        errorReport("Arguments type not match.");
      }
    }
    FD = FuncSym->getFuncDeclPointer();
  } else {
    errorReport("Function undefined.");
  }
  return ReturnType;
}

/// \brief Act on Binary Operator(Type checking and create new binary expr
/// through lhs and rhs). Note: ��C/C++�У���ֵ��䷵�ص���������ֵ�� ���磺'a =
/// 10;'���ص�ֵ��10. To Do: Shit code!
ExprASTPtr Sema::ActOnBinaryOperator(ExprASTPtr lhs, Token tok,
                                     ExprASTPtr rhs) {
  if (!lhs || !rhs)
    return nullptr;
  if (!(lhs->getType())) {
    errorReport("Left hand expression' type wrong.");
    return nullptr;
  }

  if (!(rhs->getType())) {
    errorReport("Right hand expression' type wrong.");
    return nullptr;
  }

  // Check expression's value kind.
  std::shared_ptr<Type> type = nullptr;
  if (tok.isAssign()) {
    if (!lhs->isLValue())
      errorReport("Left hand expression must be lvalue.");
    if (TypeKeyInfo::TypeKeyInfo::getHashValue(lhs->getType()) !=
        TypeKeyInfo::TypeKeyInfo::getHashValue(rhs->getType()))
      errorReport("Type on the left and type on the right must be same.");
    type = lhs->getType();
    if (DeclRefExprPtr DRE = std::dynamic_pointer_cast<DeclRefExpr>(lhs)) {
      if (DRE->getDecl()->isConst()) {
        // ����Ƿ��ʼ��
        if (VarSymPtr sym = std::dynamic_pointer_cast<VariableSymbol>(
                CurScope->Resolve(DRE->getDeclName()))) {
          if (sym->isInitial()) {
            errorReport("Can not assign to const type.");
          } else {
            sym->setInitial(true);
            sym->getDecl()->setInitExpr(rhs);
          }
        } else {
          errorReport("Undefined symbol.");
        }
      }
    }
  }

  tok::TokenValue tokKind = tok.getKind();

  // Note:����mosesֻ���������ͣ�int��bool.
  // To Do: moses��û��ʵ������ת����type cast��
  // To Do: moses��ʱ�������Զ������ͽ������������
  // Type checking.
  if (tok.isIntOperator()) {
    if (lhs->getType()->getKind() != TypeKind::INT ||
        rhs->getType()->getKind() != TypeKind::INT) {
      errorReport(
          "Left hand expression and right hand expression must be int type.");
    }
  }

  if (tok.isBoolOperator()) {
    if (lhs->getType()->getKind() != TypeKind::BOOL ||
        rhs->getType()->getKind() != TypeKind::BOOL) {
      errorReport(
          "Left hand expression and right hand expression must be bool type.");
    }
  }

  if (tok.isCmpOperator()) {
    if (TypeKeyInfo::TypeKeyInfo::getHashValue(lhs->getType()) !=
        TypeKeyInfo::TypeKeyInfo::getHashValue(rhs->getType()))
      errorReport("Type on the left and type on the right must be same.");
  }

  // �����ǰ��������������㣬��BinaryOperator��int����
  if (tok.isArithmeticOperator()) {
    type = Ctx.Int;
  }

  // �����ǰ��������߼����������BinaryOperator��bool����
  if (tok.isLogicalOperator()) {
    type = Ctx.Bool;
  }

  // �����ǰ�����Ǹ�ֵ���㣬��BinaryOperator�ķ���ֵ��lhs��ֵ��
  // --------------------Assignment operators-------------------------
  // An assignment operator stores a value in the object designated by the left
  // operand. An assignment expression has the value of the left operand after
  // the assignment, but is not an lvalue.
  // http://stackoverflow.com/questions/22616044/assignment-operator-sequencing-in-c11-expressions
  //---------------------Assignment operators-------------------------
  if (tok.isArithmeticOperator()) {
    type = lhs->getType();
  }

  // Note: Ϊ�˼���ƣ�BinaryOperatorĬ����rvalue
  auto BE = std::make_shared<BinaryExpr>(lhs->getLocStart(), rhs->getLocEnd(),
                                         type, tok.getLexem(), lhs, rhs);

  return BE;
}

/// \brief Perform simple semantic analysis for member access expr.
/// Like:
///		class A
///		{
///			var num : int;
///		}
///		var base : A;
///		base.num = 10;
ExprASTPtr Sema::ActOnMemberAccessExpr(ExprASTPtr lhs, Token tok) {
  /// (1) Check LHS
  /// LHS��ʾ���Ƕ��󲿷֣����󲿷ִ󲿷�����¶���DeclRefExpr
  /// ��Ҳ�кܴ��������DeclRefExpr��Ҳ�п����Ǻ������ñ��ʽ��
  /// ���磺
  /// func add() -> A {}
  /// ����moses���Զ������Ͳ��õ����������ͣ�Ҳ����˵�Զ������Ͳ�����ջ�Ϸ����
  /// ����ʹ��moses�Լ����ڴ������ƽ���ͳһ����ģ�Ȼ����ʱ�ؽ����������ա�
  /// ����ֻ��Ҫ���LHS���ֵ������Ƿ�Ϊ�û��Զ������ͼ��ɡ�
  if (!(lhs->getType()) || lhs->getType()->getKind() != TypeKind::USERDEFIED) {
    errorReport("Type error. Expect user defined type.");
  }

  std::shared_ptr<Type> memberType = nullptr;
  int idx = -1;
  /// (2) Check Member name.
  /// ������Ҫ�����û��Զ��������Ƿ��������Ա�������ڵĻ���ͬʱ��ȡ�����ݳ�Ա������
  /// Note:
  /// ���ｫUserDefinedType*ָ�뱩¶�����ǲ����Ϲ淶�ģ�����ֻ����ԭ��ָ̬���
  /// ����²��ܽ��ж�̬��ת����
  if (UDTyPtr BaseType =
          std::dynamic_pointer_cast<UserDefinedType>(lhs->getType())) {
    if (!(BaseType->HaveMember(tok.getLexem()))) {
      errorReport("Type " + BaseType->getTypeName() + " have no member " +
                  tok.getLexem());
      return nullptr;
    }
    memberType = BaseType->getMemberType(tok.getLexem());
    idx = BaseType->getIdx(tok.getLexem());
  }

  if (!memberType)
    return nullptr;
  return std::make_shared<MemberExpr>(
      lhs->getLocStart(), lhs->getLocEnd(), memberType, lhs, tok.getTokenLoc(),
      tok.getLexem(), memberType->getKind() != TypeKind::USERDEFIED, idx);
}

ExprASTPtr Sema::ActOnDecOrIncExpr(ExprASTPtr rhs) {
  if (!rhs->getType()) {
    return nullptr;
  }
  // (1) rhs ������int����
  if (rhs->getType()->getKind() != TypeKind::INT) {
    errorReport("Operator '++' '--' need operand is int type.");
  }

  // (2) rhs ��������ֵ
  // Note: moses��ֻ����ֵ����ֵ֮�֣�û��ʲôxrvalue��prvalue֮�֡�
  if (rhs->isRValue()) {
    errorReport("Operator '++' '--' need operand is lvalue");
  }

  // (3) ���rhs�Ƿ���const����
  if (DeclRefExprPtr DeclRef = std::dynamic_pointer_cast<DeclRefExpr>(rhs)) {
    auto decl = DeclRef->getDecl();
    if (!decl)
      errorReport("Declaration reference error!");
    auto symbol = CurScope->Resolve(decl->getName());
    if (VarSymPtr sym = std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
      if (decl->isConst() && sym->isInitial()) {
        errorReport("Const variable can't be assigned.");
      }
    } else if (ParmSymPtr psym =
                   std::dynamic_pointer_cast<ParmDeclSymbol>(symbol)) {
      return rhs;
    } else {
      errorReport("Declaration reference error!");
    }
  }
  return rhs;
}

ExprASTPtr Sema::ActOnUnaryExclamatoryExpr(ExprASTPtr rhs) {
  // ����moses���û��Զ������Ͳ������������������ȡ������ֻ������bool����
  if (rhs->getType()->getKind() != TypeKind::BOOL) {
    errorReport("Operator '!' need operand is bool type.");
  }
  return rhs;
}

/// \brief ���ڶԵ�Ŀ��ֵ������м��
/// To Do:
/// ע������intֵ�ı߽粢û���޶�����ʱ��C�����е�int����ͬ16λ(����������ʵ����32λ����)
ExprASTPtr Sema::ActOnUnarySubExpr(ExprASTPtr rhs) {
  if (rhs->getType()->getKind() != TypeKind::INT) {
    errorReport("Operator '-' need operand is int type");
  }
  return rhs;
}

/// \brief ��unpack decl���������������Ҫ�����ͼ��
/// ���磺 var {num, {flag, lhs, rhs}} = anony;
/// Shit code!
UnpackDeclPtr Sema::ActOnUnpackDecl(UnpackDeclPtr unpackDecl,
                                    std::shared_ptr<Type> type) {
  if (UnpackDeclPtr unpackd =
          std::dynamic_pointer_cast<UnpackDecl>(unpackDecl)) {
    if (!type) {
      errorReport("Unapck declaration's initial expression type error.");
    }
    if (type->getKind() != TypeKind::ANONYMOUS) {
      errorReport(
          "Unpack declaration's initial expression must be anonymous type.");
      return nullptr;
    }

    return unpackDeclTypeChecking(unpackDecl, type);
  }
  errorReport("Left hand expression isn's unpack declaration.");
  // To Do: �˴�ֱ�ӷ���nullptr̫����������Ҫ�����ʵĴ���ʽ��
  return nullptr;
}

/// \brief
BinaryPtr Sema::ActOnAnonymousTypeVariableAssignment(ExprASTPtr lhs,
                                                     ExprASTPtr rhs) const {
  if (DeclRefExprPtr DRE = std::dynamic_pointer_cast<DeclRefExpr>(lhs)) {
    // Type Checking.
    if (TypeKeyInfo::TypeKeyInfo::getHashValue(DRE->getType()) !=
        TypeKeyInfo::TypeKeyInfo::getHashValue(rhs->getType())) {
      errorReport("Type error occured in anonymous type variable assigning.");
    }
  } else {
    errorReport("Error occured in anonymous type variable assigning.");
  }
  return std::make_shared<BinaryExpr>(lhs->getLocStart(), rhs->getLocEnd(),
                                      lhs->getType(), "=", lhs, rhs);
}

/// \brief Mainly check the conditon expression type.
bool Sema::ActOnConditionExpr(std::shared_ptr<Type> type) const {
  if (!type || type->getKind() != TypeKind::BOOL) {
    errorReport("Conditional expression is not a boolean type.");
    return false;
  }
  return true;
}

/// \brief Mainly check parameter declaration type.
std::shared_ptr<Type> Sema::ActOnParmDeclUserDefinedType(Token tok) const {
  if (ClassSymPtr csym = std::dynamic_pointer_cast<ClassSymbol>(
          ScopeStack[0]->CheckWhetherInCurScope(tok.getLexem()))) {
    return csym->getType();
  } else {
    errorReport("Undefined type.");
  }
  return nullptr;
}

/// \brief Mainly check variable declararion type.
std::shared_ptr<Type> Sema::ActOnVarDeclUserDefinedType(Token tok) const {
  return ActOnParmDeclUserDefinedType(tok);
}

void Sema::PopClassStack() { ClassStack.pop_back(); }

void Sema::PopFunctionStack() { FunctionStack.pop_back(); }

std::shared_ptr<FunctionSymbol> Sema::getFunctionStackTop() const {
  if (FunctionStack.size() == 0) {
    errorReport(
        "Now in top-level scope and we can't get current function symbol.");
    return nullptr;
  }
  return FunctionStack[FunctionStack.size() - 1];
}

std::shared_ptr<ClassSymbol> Sema::getClassStackTop() const {
  if (ClassStack.size() == 0) {
    errorReport("Now in top-level scope and we can't get current class symbol");
    return nullptr;
  }
  return ClassStack[ClassStack.size() - 1];
}

void Sema::PopScope() {
  // Pop Scope and update info.
  ScopeStack.pop_back();
  CurScope = getScopeStackTop();
}

/// \brief Look up name for current scope.
std::shared_ptr<Symbol> Scope::CheckWhetherInCurScope(std::string name) {

  for (auto item : SymbolTable) {
    if (item->getLexem() == name) {
      return item;
    }
  }
  return nullptr;
}

std::shared_ptr<Scope> Sema::getScopeStackBottom() const {
  return ScopeStack[0];
}

std::shared_ptr<Scope> Sema::getScopeStackTop() const {
  if (ScopeStack.size() == 0) {
    errorReport("Now in top-level scope and we can't get current scope.");
    return nullptr;
  }
  return ScopeStack[ScopeStack.size() - 1];
}

/// \brief �÷�����Ҫ���ڽ������ͼ�飬������������
UnpackDeclPtr
Sema::unpackDeclTypeChecking(UnpackDeclPtr decl,
                             std::shared_ptr<Type> initType) const {
  if (AnonTyPtr anonyt = std::dynamic_pointer_cast<AnonymousType>(initType)) {
    // (1) ������е�ÿ�������Ƿ�����
    if (!(decl->TypeCheckingAndTypeSetting(anonyt))) {
      errorReport("Unpack declaration type error.");
      return nullptr;
    }

    // Note: ��ʵ����Ҫtype���ã���Ϊ����Ҫ����symbol table.
    decl->setCorrespondingType(initType);

    // (2) ����symbol
    // Note: �������������Ҫ�ǽ�unpack decl�ֲ���
    // ���磺 {{start, end}, {lhs, rhs}} = num;
    // ���������start, end, lhs, rhs
    // 1: �ռ�decl names
    std::vector<VarDeclPtr> unpackDecls;
    decl->getDecls(unpackDecls);

    // 2: �ռ�types
    std::vector<std::shared_ptr<Type>> types;
    anonyt->getTypes(types);

    // 3: check
    if (unpackDecls.size() != types.size()) {
      errorReport("Unpack declaration type error.");
    }
    // 3: ����symbol
    unsigned size = unpackDecls.size();
    for (unsigned index = 0; index < size; index++) {
      // ����variable symbol�����뵱ǰscope
      // ������һ�����⣺unpackdecl var {num1, num2} ��������VarSymbol
      // ������VarSymbol����Ҫ�洢UnpackDecl.
      CurScope->addDef(std::make_shared<VariableSymbol>(
          unpackDecls[index]->getName(), CurScope, types[index], true,
          unpackDecls[index]));
    }
  } else {
    errorReport("Unpack declaration type error.");
    return nullptr;
  }
  return decl;
}
/// \brief ����sema�ı���
void Sema::errorReport(const std::string &msg) const {
  Ctx.isParseOrSemaSuccess = false;
  errorSema(scan->getLastToken().getTokenLoc().toString() + " --- " + msg);
}