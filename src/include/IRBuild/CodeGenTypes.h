//===--------------------------CodeGenTypes.h-----------------------===//
//
// This file handles AST -> moses-IR type lowering.
//
//===---------------------------------------------------------------===//
#ifndef CODE_GEN_TYPES_H
#define CODE_GEN_TYPES_H
#include <map>
#include <set>
#include <utility>
#include "CGCall.h"
#include "../IR/IRType.h"
#include "../Parser/Type.h"
#include "../Parser/ast.h"
namespace compiler
{
	namespace IRBuild
	{
		using namespace compiler::IR;
		class ModuleBuilder;

		using IRType = compiler::IR::Type;
		using IRStructTy = compiler::IR::StructType;
		using IRFuncTy = compiler::IR::FunctionType;
		using IRTyPtr = std::shared_ptr<IRType>;
		using IRFuncTyPtr = std::shared_ptr<IRFuncTy>;

		/// This class orgasizes the cross-module state that is used while lowering
		/// AST types to moses-IR types.
		/// Note: 在moses IR中有一个文件MosesIRContext.h, 存储了Global的Type和Constant
		/// 等信息。与这里的并不冲突，这里的CodeGenTypes只是作为一个接口，只服务于代码生成。
		class CodeGenTypes
		{
			//   ----------------		<--- CodeGenModule
			//  |				 |
			//	 ----------------
			//	|				 |
			//	 ----------------
			//	|				 |
			//	 ----------------
			//	|  CodeGenTypes	 |
			//	|				 |
			//	|	   &CGM		 |
			//	 ----------------
			ModuleBuilder &CGM;

			// Contains the moses-IR type for any converted RecordDecl.
			std::map<const ast::Type*, std::shared_ptr<StructType>> RecordDeclTypes;

			// Hold CGFunctionInfo results.
			std::set<CGFunctionInfo> FunctionInfos;

		public:
			CodeGenTypes(ModuleBuilder& CGM) : CGM(CGM) {}
			/// ConvertType - Convert type T into a moses-IR type.
			IRTyPtr ConvertType(ASTTyPtr type);
			
			// std::shared_ptr<IR::Type> ConvertFunctionType(FunctionDeclPtr FD);
			IRFuncTyPtr getFunctionType(const CGFunctionInfo &Info);
		};
	}
}
#endif