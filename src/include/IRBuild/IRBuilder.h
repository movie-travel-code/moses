//===----------------------------IRBuilder.h-----------------------------===//
// 
// This class for intermediate generation.
// Note: IRBuilder refer to ¡¶Crafting a Compiler¡·- "Syntax-directed 
// Translation" and "Code Generation for a Virtual Machine".
// 
//===--------------------------------------------------------------------===//

// ---------------------------nonsense for coding---------------------------
// At some point in each of the code-generation "visit" methods, actual code
// must be emitted. The syntax and specification of such code depend on the
// form of moses IR.
// -------------------------------------------------------------------------

#ifndef MOSES_IR_BUILDER_H
#define MOSES_IR_BUILDER_H
#include "../Parser/ast.h"
namespace compiler
{
	namespace IR
	{
		using namespace ast;
		/// @brief IRBuilder - This class inherit Visitor
		class IRBuilder : public StatementAST::Visitor
		{
		public:
			// Emit IR for binary expression.
			virtual void visit()
			{
				
			}
		};

		/// @brief ModuleBuilder - This class for module code generation.
		/// The outermost portions of an AST contain class and method declarations.
		/// The ModuleBuilder is responsible for processing each class and method
		/// declarations.
		class ModuleBuilder : public IRBuilder
		{

		};

		/// @brief FunctionBodyVisitor - This class for Function Body generation.
		class FunctionBodyBuilder : public IRBuilder
		{

		};

		/// @brief - 
	}
}

#endif