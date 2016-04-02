//===--------------------------------main.cpp---------------------------------===//
// 
// 2016.3.7
// 
//===-------------------------------------------------------------------------===//
#include <iostream>
#include "scanner.h"
#include "parser.h"
using namespace compiler::parse;
using namespace compiler::sema;
int main()
{
	Scanner scanner("E:/test/main.mo");
	Sema sema;
	Parser parse(scanner, sema);
	parse.parse();
	system("pause");
	return 0;
}