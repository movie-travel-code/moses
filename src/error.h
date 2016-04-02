//===---------------------------------error.h-----------------------------===//
//
// This file for error report.
//
//===--------------------------------------------------------------------===//
#ifndef ERROR_H
#define ERROR_H
#include <string>
namespace compiler
{
	extern void errorToken(const std::string& msg);
	extern void errorParser(const std::string& msg);
}

#endif