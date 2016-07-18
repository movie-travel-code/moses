//===------------------------------IRPrinter.h----------------------------===//
//
// This file implements the IRPrinter.
//
//===---------------------------------------------------------------------===//
#include <iostream>
#include <vector>
#include "../Value.h"
#include "../User.h"
#include "../Instruction.h"
#include "../IRType.h"
namespace compiler
{
	namespace IR
	{
		/// \brief The base class of BBPrinter and FunctionPrinter
		class IRPrinter
		{
		public:
			static void Print(const std::list<ValPtr>& IR, std::ostringstream& out);
		};
	}	
}
