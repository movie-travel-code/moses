//===---------------------------MosesIRContext.cpp------------------------===//
//
// This file implements class MosesIRContext.
//
//===--------------------------------------------------------------------===//
#include "../../include/IR/MosesIRContext.h"
using namespace compiler::IR;

IRTyPtr Type::getVoidType(MosesIRContext &Ctx) { return Ctx.getVoidTy(); }

IRTyPtr Type::getLabelType(MosesIRContext &Ctx) { return Ctx.getLabelTy(); }

IRTyPtr Type::getIntType(MosesIRContext &Ctx) { return Ctx.getIntTy(); }

IRTyPtr Type::getBoolType(MosesIRContext &Ctx) { return Ctx.getBoolTy(); }