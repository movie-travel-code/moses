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
		/// Note: moses��ʱ����ҪABI��Ϣ��
		class CGFunctionInfo
		{
			unsigned NumArgs;
			std::vector<std::pair<ASTTyPtr, std::string>> ArgInfos;
		private:
			CGFunctionInfo(std::shared_ptr<ASTTyPtr>);
			// Note: ��ʱmoses��֧�ֶ��ֵ��ù��������纯��ֵ�����Ƿ���Ҫ�����Ż�
			// unsigned CallingConvention;
			bool NoReturn;
		public:
			bool isNoReturn() const { return NoReturn; }
		};
	}
}

#endif