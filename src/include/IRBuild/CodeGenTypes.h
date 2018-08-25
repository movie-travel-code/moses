//===--------------------------CodeGenTypes.h-----------------------===//
//
// This file handles AST -> moses-IR type lowering.
//
//===---------------------------------------------------------------===//
#ifndef CODE_GEN_TYPES_H
#define CODE_GEN_TYPES_H
#include "include/IR/IRType.h"
#include "include/IR/MosesIRContext.h"
#include "include/Parser/Type.h"
#include "include/Parser/ast.h"
#include "CGCall.h"
#include <cassert>
#include <map>
#include <set>
#include <utility>

namespace compiler {
namespace IRBuild {
using namespace compiler::IR;
class ModuleBuilder;

using IRType = compiler::IR::Type;
using IRStructTy = compiler::IR::StructType;
using IRFuncTy = compiler::IR::FunctionType;
using IRTyPtr = std::shared_ptr<IRType>;
using IRFuncTyPtr = std::shared_ptr<IRFuncTy>;
using CGFuncInfoConstPtr = std::shared_ptr<CGFunctionInfo const>;
using GetFuncTypeRet = std::pair<IRFuncTyPtr, std::vector<std::string>>;

/// This class orgasizes the cross-module state that is used while lowering
/// AST types to moses-IR types.
class CodeGenTypes {
  //   ----------------    <--- CodeGenModule
  //  |                |
  //	 ----------------
  //	|                |
  //	 ----------------
  //	|                |
  //	 ----------------
  //	|  CodeGenTypes	 |
  //	|                |
  //	|      &CGM      |
  //	 ----------------
  MosesIRContext &IRCtx;
  ;

  // Contains the moses-IR type for any converted RecordDecl.
  std::map<const ast::Type *, std::shared_ptr<StructType>> RecordDeclTypes;

  // Hold CGFunctionInfo results.
  std::map<const FunctionDecl *, CGFuncInfoConstPtr> FunctionInfos;
  std::map<const FunctionDecl *, FuncTypePtr> FunctionTypes;
  unsigned AnonyTypesCounter;
  std::string TypeNamePrefix;

public:
  CodeGenTypes(MosesIRContext &IRCtx)
      : IRCtx(IRCtx), AnonyTypesCounter(0), TypeNamePrefix("@") {}
  /// ConvertType - Convert type T into a moses-IR type.
  IRTyPtr ConvertType(ASTTyPtr type);
  GetFuncTypeRet getFunctionType(const FunctionDecl *FD,
                                 std::shared_ptr<CGFunctionInfo const> Info);
  std::shared_ptr<const CGFunctionInfo>
  arrangeFunctionInfo(const FunctionDecl *FD);
  std::string getAnonyName();
};
} // namespace IRBuild
} // namespace compiler
#endif