//===--------------------------------Type.cpp-----------------------------===//
// 
// This file is used to implement class Type.
// 
//===---------------------------------------------------------------------===//
#include "Type.h"

namespace compiler
{
	namespace ast
	{
		TypeKind Type::checkTypeKind(tok::TokenValue kind)
		{
			switch (kind)
			{
			case compiler::tok::TokenValue::KEYWORD_int:
				return TypeKind::INT;
			case compiler::tok::TokenValue::KEYWORD_bool:
				return TypeKind::BOOL;
			default:
				return TypeKind::USERDEFIED;
			}
		}
	}
}