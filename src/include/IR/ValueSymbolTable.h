//===--------------------------------SymbolTable.h------------------------===//
//
// This file implements the main symbol table for moses IR.
//
//===----------------------------------------------------------------------===//
#ifndef MOSES_IR_VALUE_SYMBOL_TABLE_H
#define MOSES_IR_VALUE_SYMBOL_TABLE_H
#include <map>
#include <string>
#include "Value.h"

namespace compiler
{
	namespace IR
	{
		/// This class provides a symbol table of name/value pairs that is broken
		/// up by type. For each Type* there is a "plane" of name/value pairs in
		/// the symbol table. Identical types may overlapping symbol names as long
		/// as they are distinct. The SymbolTable also tracks, separately, a map of
		/// name/type pairs. This allows types to be named. Types are treated
		/// distinctly from Values.
		/// 
		/// ----------------------------nonsens for coding------------------------
		/// The LLVM in-memory representation of its IR does not use a symbol table.
		/// Instruction contain direct memory links to their operands(and their
		/// users), so if you have an instruction and wants to access its operand, 
		/// just follow the link, you do not have to perform a lookup in any symbol
		/// table.
		///
		/// There are some lists associated with LLVM contexts, modules, functions 
		/// and basic blocks, which allow you to access the contained elements, not
		/// tables associating a name with anything.
		///
		/// Of course, if you want to parse a textual IR file(II), you would probably
		/// need to a symbol table(or something similar) to do so and create the
		/// above-mentioned links; but there's little reason to do that seeing that 
		/// LLVM already contains such a parser(and that parser indeed uses some way
		/// to associate a "name" with a value - see the implementation of 
		/// BitcodeReader).
		/// 
		/// As for LLVM front-ends for generating IR - that is up to you. I'd say
		/// that if you want to parse a C-like languages, using a symbol table would
		/// be really useful.
		/// http://stackoverflow.com/questions/13089015/how-symbol-table-handling-is-done-in-llvm-based-compiler
		/// ----------------------------nonsense for coding-----------------------
		/// 但是在创建LLVM IR时，肯定是需要保留以前的symbol info.
		/// 参见：http://www.llvmpy.org/llvmpy-doc/0.9/doc/kaleidoscope/PythonLangImpl3.html
		/// 中3.2，
		/// We will also need to define some global variables which we will be used during code
		/// generation:
		/// # The LLVM module, which holds all the IR code.
		/// g_llvm_module = Module.new('my cool jit')
		/// 
		/// # The LLVM instruction builder. Created whenever a new function is entered.
		/// g_llvm_builder = None
		/// 
		/// # A dictonary that keeps track of which values are defined in the current scope
		/// # and what their LLVM representation is.
		/// g_named_valued = {}
		/// ---------------------------nosense for coding--------------------------
		/// 编译器可能包含几个不同的、专门化的符号表，这里的符号表与语法分析过程中的
		/// 符号表不同。几乎所有的转换（翻译）过程都需要符号表。
		///		源码		->	AST		符号表（用于记录所有的identifier）
		///		AST		->	IR		符号表（用于记录所有的值value）
		///		例如： a = num1 + num2 * num3 - num4;
		///		对于语法分析来说，这里有5个identifier，但是对于中间代码生成来说不止这些。
		///		temp1 <- num2 * num3
		///		temp2 <- num1 + temp1
		///		a <- temp2 - num4
		///	另外，在中间代码生成（线性IR）的时候，需要跳转label，这些都是需要记录的value，但是
		/// 在语法分析的时候是不需要的。
		/// -----------------------------------------------------------------------
		class ValueSymbolTable
		{
			// 用于存储当前current scope的symbol info
			std::map<std::string, ValPtr> ValueMap;
		public:
			/// This method finds the value with the given \p Name in the the symbol
			/// table.
			/// @return the value associated with the \p Name.
			/// @brief Look up a named Value.
			ValPtr lookup(std::string Name) const 
			{ 
				auto iter = ValueMap.find(Name);
				if (iter == ValueMap.end())
					return nullptr;			
				return iter->second;
			}

			/// @return true iff the symbol table is empty
			/// @brief Determine if the symbol table is empty.
			bool empty() const { return ValueMap.empty(); }

			/// @brief The number of name/type pairs is returned.
			unsigned size() const { return ValueMap.size(); }
		private:
			// 为该Value创建一个唯一的名字
			std::string makeUniqueName(Value *V);

			/// This methods adds provided value \p N to the symbol table. The Value
			/// must have a name which is used to place the value in the symbol table.
			/// If the inserted name conflicts, this renames the value.
			/// @brief Add a name value to the symbol table.
			void reinsertValue(ValPtr);

			/// createValueName - This method attempts to create a value name and insert
			/// it into the symbol table with the specified name. If it conflicits, it
			/// auto-renames the name and returns that instead.
			std::string createValueName(std::string Name, Value *V);

			/// This method removes a value from the symbol table. It leaves the ValueName
			/// attached to the value, but it is no longer inserted in the symtab.
			void removeValueName(std::string Name);
		};
	}
}
#endif