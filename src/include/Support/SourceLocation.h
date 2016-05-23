#ifndef SOURCELOCATION_INCLUDE
#define SOURCELOCATION_INCLUDE
#include <iostream>
#include <string>
#include "../Lexer/Token.h"

namespace compiler
{
	using namespace compiler;
	/// @brief SourceLocation - This class represents source location.
	/// SourceLocation is same as TokenLocation
	class SourceLocation
	{
		std::string FileName;
		unsigned long line;
		unsigned long column;
	public:
		SourceLocation() : line(0), column(0), FileName(""){}
		SourceLocation(const TokenLocation& tokLoc) :
			FileName(tokLoc.getTokenFileName()), line(tokLoc.getTokenLineNumber()),
			column(tokLoc.getTokenColNumber()) {}

		unsigned long getLineNumber() const { return line; }
		unsigned long getColumnNumber() const { return column; }
		std::string getFileName() const { return FileName; }

		void setLineNumber(unsigned long line) { this->line = line; }
		void setColumnNumber(unsigned long column) { this->column = column; }
		void setFileName(std::string fileName) { this->FileName = fileName; }

		bool operator==(const SourceLocation& loc) const
		{
			return loc.getColumnNumber() == column &&
				loc.getLineNumber() == line && loc.getFileName() == FileName;
		}
		bool operator!=(const SourceLocation& loc) const
		{
			return !operator==(loc);
		}
	};
}
#endif