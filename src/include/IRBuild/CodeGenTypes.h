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
#include <cassert>
#include "../IR/MosesIRContext.h"
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
		using CGFuncInfoConstPtr = std::shared_ptr<CGFunctionInfo const>;
		using GetFuncTypeRet = std::pair<IRFuncTyPtr, std::vector<std::string>>;

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
			MosesIRContext &IRCtx;;

			// Contains the moses-IR type for any converted RecordDecl.
			std::map<const ast::Type*, std::shared_ptr<StructType>> RecordDeclTypes;

			// Hold CGFunctionInfo results.
			std::map<const FunctionDecl*, CGFuncInfoConstPtr> FunctionInfos;
			std::map<const FunctionDecl*, FuncTypePtr> FunctionTypes;
		public:
			CodeGenTypes(MosesIRContext& IRCtx) : IRCtx(IRCtx) {}
			/// ConvertType - Convert type T into a moses-IR type.
			/// Note: 如果发现当前Type是StructType，则记录到Map中。
			IRTyPtr ConvertType(ASTTyPtr type);
						
			GetFuncTypeRet getFunctionType(const FunctionDecl* FD, std::shared_ptr<CGFunctionInfo const> Info);

			std::shared_ptr<const CGFunctionInfo> arrangeFunctionInfo(const FunctionDecl* FD);			
		};
	}
}
#endif