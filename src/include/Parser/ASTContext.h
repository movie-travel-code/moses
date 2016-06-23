//===------------------------------ASTContext.h---------------------------===//
//
// This file defines the ASTContext interface.
//
//===---------------------------------------------------------------------===//
#ifndef AST_CONTEXT_H
#define AST_CONTEXT_H
#include "Type.h"
#include "../Support/Hasing.h"
#include "../Support/TypeSet.h"
using namespace compiler::SupportStructure;
namespace compiler
{
	namespace ast
	{
		/// ASTContext - This class holds types that can be referred to thorought
		/// the semantic analysis of a file.
		///
		/// Note: 在sema中的symbol table也会保存编译过程中得到的符号，包括userdefined
		/// type decl, 以及各种decl，其中就包含关于ClassType的具体信息。但是由于ClassType
		/// 应该是全局只会有一份儿，另外AnonymousType也应该保存起来，所以我们这里定义了
		/// ASTContext来保存各种Type info。然后ASTNode以及SymbolTable都是从ASTContext中
		/// 拷贝过来的。
		/// 
		///		 --------------			-------------			------------------
		///		|  ASTContext  |	   |   ASTNode	 |		   | SymbolTableEntry |
		///		 --------------		    -------------		    ------------------
		///				|					  |						      |
		///				|					  |						      |
		///			   \|/					 \|/					     \|/
		///	vector<shared_ptr<Type>>  shared_ptr<Type>		     shared_ptr<Type>
		///					/       （从ASTContext拷贝）			(从ASTContext拷贝)
		///				   /				/					
		///		==========/==============  /
		///	   ||		 /			    ||/
		///	   ||	ClassTypes		    |/
		///	   ||	AnonymousTypes		/|
		///	   ||	BuiltinTypes	   /||
		///	   ||					    ||
		///		=========================
		///
		/// 例如：
		/// class B
		///	{
		///		var flag : bool;
		/// }
		/// class A
		///	{
		///		var start : B;
		///		var end : int;
		///	};
		/// var num : int;
		/// var anonyWithInit = {10, {0, true}, {false, 10}};
		/// var anonyWithoutInit : {int, bool};
		/// 此时在ASTContext中有如下的类型信息：
		///	
		///				==============		=============
		///			   ||	bool 	 ||	   ||	int 	||
		///				===|==========	   /=============
		///				   |			  /
		///		===========|==========   /
		///	   ||	B{bool} : UDType || /
		///		========|============= /
		///				|			  /
		///				|			 /
		///		========|=========	/	 ========================
		///	   ||		|	 	 ||		||						||
		///	   ||	A : UDType	 ||	   /|| {int, bool} : Anony	||
		///	   ||				 ||	  /	||						||
		///		==================	 /	 ========================
		///							/
		///			===============/=================
		///		   ||			  /					||
		///		   ||	{int, Anony, Anony} : Anony	||
		///		   ||					|			||
		///			====================|============
		///								|
		///					============|==========
		///				   ||			|		  ||
		///				   ||	{bool, int}		  ||
		///				   ||					  ||
		///				    =======================
		///
		/// 也就是说在moses源代码中出现的所有类型element，在ASTContext中都会有唯一的存在。
		///
		class ASTContext
		{
		private:
			typedef TypeKeyInfo::UserDefinedTypeKeyInfo UDKeyInfo;
			typedef TypeKeyInfo::AnonTypeKeyInfo AnonTypeKeyInfo;
		public:
			// 预分配内置类型
			ASTContext() : 
				Int(std::make_shared<BuiltinType>(TypeKind::INT)), 
				Bool(std::make_shared<BuiltinType>(TypeKind::BOOL)),
				Void(std::make_shared<BuiltinType>(TypeKind::VOID)),
				isParseOrSemaSuccess(true) {}

			std::shared_ptr<BuiltinType> Int;
			std::shared_ptr<BuiltinType> Bool;
			std::shared_ptr<BuiltinType> Void;

			TypeSet<std::shared_ptr<UserDefinedType>, UDKeyInfo> UDTypes;
			TypeSet<std::shared_ptr<AnonymousType>, AnonTypeKeyInfo> AnonTypes;

			bool isParseOrSemaSuccess;
		};
	}
}
#endif