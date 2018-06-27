//===------------------------------ASTContext.h---------------------------===//
//
// This file defines the ASTContext interface.
//
//===---------------------------------------------------------------------===//
#ifndef AST_CONTEXT_H
#define AST_CONTEXT_H
#include "../Support/Hasing.h"
#include "../Support/TypeSet.h"
#include "Type.h"

using namespace compiler::SupportStructure;
namespace compiler {
namespace ast {
/// ASTContext - This class holds types that can be referred to thorought
/// the semantic analysis of a file.
///
/// Note: ��sema�е�symbol tableҲ�ᱣ���������еõ��ķ��ţ�����userdefined
/// type decl, �Լ�����decl�����оͰ�������ClassType�ľ�����Ϣ����������ClassType
/// Ӧ����ȫ��ֻ����һ�ݶ�������AnonymousTypeҲӦ�ñ��������������������ﶨ����
/// ASTContext���������Type info��Ȼ��ASTNode�Լ�SymbolTable���Ǵ�ASTContext��
/// ���������ġ�
///
///		 --------------			-------------
///------------------ 		|  ASTContext  |	   |   ASTNode	 |		   |
///SymbolTableEntry |
///		 --------------		    -------------
///------------------ 				|					  |
///| 				|					  |
///|
///			   \|/					 \|/
///\|/ 	vector<shared_ptr<Type>>  shared_ptr<Type> shared_ptr<Type> 					/
///����ASTContext������			(��ASTContext����) 				   /
////
///		==========/==============  /
///	   ||		 /			    ||/
///	   ||	ClassTypes		    |/
///	   ||	AnonymousTypes		/|
///	   ||	BuiltinTypes	   /||
///	   ||					    ||
///		=========================
///
/// ���磺
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
/// ��ʱ��ASTContext�������µ�������Ϣ��
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
///	   ||		|	 	 ||		||
///||
///	   ||	A : UDType	 ||	   /|| {int, bool} : Anony	||
///	   ||				 ||	  /	||
///||
///		==================	 /	 ========================
///							/
///			===============/=================
///		   ||			  / ||
///		   ||	{int, Anony, Anony} : Anony	||
///		   ||					| ||
///			====================|============
///								|
///					============|==========
///				   ||			|		  ||
///				   ||	{bool, int}		  ||
///				   ||					  ||
///				    =======================
///
/// Ҳ����˵��mosesԴ�����г��ֵ���������element����ASTContext�ж�����Ψһ�Ĵ��ڡ�
///
class ASTContext {
private:
  typedef TypeKeyInfo::UserDefinedTypeKeyInfo UDKeyInfo;
  typedef TypeKeyInfo::AnonTypeKeyInfo AnonTypeKeyInfo;

public:
  // Ԥ������������
  ASTContext()
      : Int(std::make_shared<BuiltinType>(TypeKind::INT)),
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
} // namespace ast
} // namespace compiler
#endif