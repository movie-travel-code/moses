//===------------------------------CGCall.h-------------------------------===//
//
// These classes wrap the information about a call.
//
//===---------------------------------------------------------------------===//
#ifndef CGCALL_H
#define CGCALL_H
#include <vector>
#include "../Parser/Type.h"
namespace compiler
{
	namespace IRBuild
	{
		using namespace ast;
		using ASTTyPtr = std::shared_ptr<ast::Type>;

		/// CGFunctionInfo - Class to encapsulate the information about a function
		/// definition.
		class CGFunctionInfo
		{
			unsigned NumArgs;
			std::vector<std::pair<ASTTyPtr, std::string>> ArgInfos;
			ASTTyPtr returnTy;
		private:			
			// Note: 暂时moses不支持多种调用惯例，例如函数值返回是否需要进行优化
			// unsigned CallingConvention;
			bool NoReturn;
		public:
			CGFunctionInfo(unsigned NumArgs, std::vector<std::pair<ASTTyPtr, std::string>>, ASTTyPtr retty);
			static std::shared_ptr<CGFunctionInfo const> create(const FunctionDecl* FD);
			ASTTyPtr getRetTy() const { return returnTy; }
			bool isNoReturn() const { return NoReturn; }
			unsigned getArgNums() const { return NumArgs; }
			const std::pair<ASTTyPtr, std::string>& getParm(unsigned index) const 
			{ 
				assert(index <= NumArgs - 1 && "Index out of range when we get FunctionInfo.");
				return ArgInfos[index]; 
			}
			const std::vector<std::pair<ASTTyPtr, std::string>>& getParmInfo() const { return ArgInfos; }
		};
	}
}

#endif