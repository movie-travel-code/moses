//===---------------------------------------error.cpp-----------------------------------===//
//
// This file impletments error mechanism.
//
//===-----------------------------------------------------------------------------------===//
#include <iostream>
#include "error.h"

namespace compiler
{
	void errorToken(const std::string& msg)
	{
		std::cerr << "Token Error: " << msg << std::endl;
	}

	void errorParser(const std::string& msg)
	{
		std::cerr << "parser error: " << msg << std::endl;
	}
}