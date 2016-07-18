//===------------------------------CGCall.h-------------------------------===//
//
// These classes wrap the information about a call.
//
//===---------------------------------------------------------------------===//
#ifndef CGCALL_H
#define CGCALL_H
#include <vector>
#include "../Parser/Type.h"
#include "../IR/IRType.h"
namespace compiler
{
	namespace IRBuild
	{
		class ArgABIInfo;
		using namespace ast;
		using namespace IR;
		using AAIPtr = std::shared_ptr<ArgABIInfo>;

		// Note: Moses temporarily support single TU, so CallingConvention is very
		// simple(variation of cdecl).
		// (1) int and bool passed directly.
		// e.g.	source code =====> func add(lhs:int, rhs:int) -> void {}
		//		moses IR    =====> define @add(int lhs, int rhs) {}
		//
		// (2) For small struct(i8 ~ i64), can be passed directly through stack or
		//     register.
		// e.g.	source code	=====> class Size{ var lhs:int; }; 
		//						   func add(parm:Size) {}
		//		moses IR    =====> %struct.Size = type {i32}
		//						   define @add(i32 parm) {}
		//		or
		//		source code =====> func add(parm : {int, int}) -> void {}
		//		moses IR    =====> define @add(int parm1, int parm2) {}
		//		
		//		Note: we may flatten the struct to arguments.
		//
		// (3) For larger struct, passed through a hidden pointer.
		// e.g. source code =====> class Size { var m1:int; var m2:bool; var m3:int; };
		//						   func add(oarm : Size) -> void {}
		//		moses IR    =====> %struct.Size = type {int, bool, int}
		//					=====> define @add(%struct.Size parm) {}
		//
		//		Note: Caller allocate the space for temp memory and pass a pointer
		//			  of the temp memory to the callee.
		class ArgABIInfo
		{
		public:
			enum Kind 
			{ 
				/// Direct - Pass the argument directly using the normal converted moses IR
				/// type, or by coercing to another specified type stored in 'CoerceToType'.
				Direct, 

				/// Indirect - Pass the argument indirectly via a hidden pointer
				/// with the specified alignment (0 indicates default alignment).
				InDirect,

				/// Ignore - Ignore the argument(treat as void). Useful for void and
				/// empty structs.
				Ignore
			};
		private:
			ASTTyPtr Ty;
			Kind TheKind;

			// struct type can be flattened.
			// e.g. class { var num:int; };					---->	coerce to int(i32)
			std::shared_ptr<IR::Type> TypeData; // isDirect()
			// e.g. class { var num:int, var flag:bool; };	---->	int, int
			bool CanBeFlattened;
		public:
			ArgABIInfo(ASTTyPtr type, Kind kind, std::shared_ptr<IR::Type> tydata = nullptr, bool flatten = false) :
				Ty(Ty), TheKind(kind), TypeData(tydata), CanBeFlattened(flatten)
			{}

			static std::shared_ptr<ArgABIInfo> Create(ASTTyPtr type, Kind kind);
			ASTTyPtr getType() const { return Ty; }
			Kind getKind() const { return TheKind; }
			bool canBeFlattened() const { return CanBeFlattened; }
		};

		/// CGFunctionInfo - Class to encapsulate the information about a function
		/// definition.
		class CGFunctionInfo
		{
			std::vector<AAIPtr> ArgInfos;
			AAIPtr ReturnInfo;
		private:			
			bool NoReturn;
		public:
			CGFunctionInfo(std::vector<ASTTyPtr> ArgsTy, ASTTyPtr RetTy);
			static std::shared_ptr<CGFunctionInfo const> create(const FunctionDecl* FD);
			bool isNoReturn() const { return NoReturn; }
			unsigned getArgNums() const { return ArgInfos.size(); }
			const std::vector<AAIPtr>& getArgsInfo() const { return ArgInfos; }
			const ASTTyPtr getParm(unsigned index) const 
			{ 
				assert(index <= getArgNums() - 1 && "Index out of range when we get FunctionInfo.");
				return ArgInfos[index]->getType(); 
			}
			const ArgABIInfo::Kind getKind(unsigned index) const 
			{ 
				assert(index <= getArgNums() - 1 && "Index out of range when we get FunctionInfo.");
				return ArgInfos[index]->getKind();
			}			
			const AAIPtr getArgABIInfo(unsigned index) const
			{
				assert(index <= getArgNums() - 1 && "Index out of range when we get FunctionInfo.");
				return ArgInfos[index];
			}
			AAIPtr getReturnInfo() const { return ReturnInfo; }
			// Generate ArgABIInfo for return type.
			static AAIPtr classifyReturnTye(ASTTyPtr RetTy);
			static AAIPtr classifyArgumentType(ASTTyPtr ArgTy);
		};
	}
}

#endif