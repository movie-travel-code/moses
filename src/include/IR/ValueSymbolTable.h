//===--------------------------------SymbolTable.h------------------------===//
//
// This file implements the main symbol table for moses IR.
// 暂时moses使用 in-memory形式的IR，暂时不需要SymbolTable，in-memory IR直接
// 使用链表进行关联，并且，Function、BasicBlock以及Instruction中间有links进行
// 关联。但是还是保留这样一个文件。
// 
//===----------------------------------------------------------------------===//
#ifndef MOSES_IR_VALUE_SYMBOL_TABLE_H
#define MOSES_IR_VALUE_SYMBOL_TABLE_H
#endif