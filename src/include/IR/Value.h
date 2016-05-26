//===-------------------------------Value.h-------------------------------===//
//
// This file defines the very important Value class. This is subclassed by a
// bunch of other important classes, like Instruction, Function, Type, etc...
// 
// moses IR参考了LLVM IR，注释部分摘自LLVM Value.h (吐槽：为什么要起Value这个名字)
// 
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_VALUE
#define MOSES_IR_VALUE

#include <list>
#include <iostream>
#include <string>
#include <memory>
#include "Use.h"
#include "IRType.h"
//===---------------------------------------------------------------------===//
//						Value Class
//===---------------------------------------------------------------------===//

/// Value - The base class of all values computed by a program that may be used
/// as operands to other values. Value is the super class of other important
/// classes such as Instruction and Function. All Values have a Type. Type is
/// not a subclass of Value.
///
/// Every value has a "use list" that keeps track of which other Values are
/// using this value.
/// ------------------------------nonsense for coding------------------------
///	Value是moses IR中的基类，类似于语法树中statement，value只定义一些简单的操作。
/// moses IR是一种线性IR，内置了CFG信息（通过Block来组织），由于是SSA-based的，
/// 所以内含def-use信息。为了高效的查找，使用散列表作为符号表的基本结构（但为了
///	方便，符号表暂时设计为vector）。
/// -------------------------------------------------------------------------
namespace compiler
{
	namespace IR
	{
		class Type;
		class Constant;
		class Argument;
		class Instruction;
		class BasicBlock;
		class GlobalValue;
		class CmpInst;
		class AllocaInst;
		class GetElementPtrInst;
		class CallInst;
		class Function;
		class FunctionType;
		class ExtractValueInst;
		class PHINode;
		class ReturnInst;
		class GlobalVariable;
		class ValueSymbolTable;
		class ValueSymbolTable;
		class BinaryOperator;
		class User;
		class Argument;
		class TerminatorInst;

		typedef std::shared_ptr<Type>				TyPtr;
		typedef std::shared_ptr<Value>				ValPtr;
		typedef std::shared_ptr<Use>				UsePtr;
		typedef std::shared_ptr<Instruction>		InstPtr;
		typedef std::shared_ptr<BasicBlock>			BBPtr;
		typedef std::shared_ptr<BinaryOperator>		BOPtr;
		typedef std::shared_ptr<CmpInst>			CmpInstPtr;
		typedef std::shared_ptr<AllocaInst>			AllocInstPtr;
		typedef std::shared_ptr<GetElementPtrInst>	GEPInstPtr;
		typedef std::shared_ptr<CallInst>			CallInstPtr;
		typedef std::shared_ptr<FunctionType>		FuncTyPtr;
		typedef std::shared_ptr<ExtractValueInst>	EVInstPtr;
		typedef std::shared_ptr<PHINode>			PHINodePtr;
		typedef std::shared_ptr<ReturnInst>			RetInstPtr;
		typedef std::shared_ptr<Function>			FuncPtr;
		typedef std::shared_ptr<ValueSymbolTable>	SymTabPtr;
		typedef std::shared_ptr<User>				UserPtr;
		typedef std::shared_ptr<Argument>			ArgPtr;
		typedef std::shared_ptr<TerminatorInst>		TermiPtr;

		class Value
		{
		public:
			// ------------------nonsense for coding------------------------
			// LLVM IR is strongly typed, every instruction has a specific
			// type it expects as each operand. For example, store requires
			// that the address's type will always be a pointer to the type
			// of the value being stored.
			enum class ValueTy
			{
				TypeVal,			// This is an instance of Type
				ConstantVal,		// This is an instance of Constant
				ArgumentVal,		// This is an instance of Argument
				InstructionVal,		// This is an instance of Instruction
				BasicBlockVal,		// This is an instance of BasicBlock
				FunctionVal,		// This is an instance of Function
				GlobalVariableVal	// This is an instance of VlobalVariable
			};
			
		private:
			// a "use list" that keep track of which other Values are using this value.
			// -------------------------nonsense for coding---------------------------
			// PS: LLVM IR中如何定义def-use链的机制没有搞懂。所以暂时实现自己的def-use机制
			// 使用内置的list将一系列的 'use' object链接起来
			// -----------------------------------------------------------------------
			std::list<UsePtr> Uses;
			std::string Name;
			ValueTy VTy;
			TyPtr Ty;
			void operator=(const Value&) = delete;	// Do not implement
			Value(const Value&) = delete;			// Do not implement

		public:
			// Type表示类型，例如int short class等等。
			// ValueTy表示的是指令的种类.
			Value(std::shared_ptr<Type> Ty, ValueTy vty, const std::string& name = "");
			virtual ~Value();

			// All values are typed, get the type of this value.
			std::shared_ptr<Type> getType() const { return Ty; }

			bool hasName() const { return Name != ""; }
			const std::string& getName() const { return Name; }

			virtual void SetName(const std::string &name/*, SymbolTable* = 0*/)
			{
				Name = name;
			}

			/// getValueType - Return the immediate subclass of this Value.
			ValueTy getValueType() const { return VTy; }

			/// replaceAllUsesWith - Go through the use list for this definition and make
			/// each use point to "V" instead of "this". After this completes, this's
			/// use list is guaranteed to be empty.
			/// 将指定V的use链中的value指向当前值。
			void replaceAllUsesWith(Value *V);

			//---------------------------------------------------------------------
			// Methods for handling the vector of uses of this Value.
			// 
			unsigned use_size()	const { return Uses.size(); }
			bool use_empty() const { return Uses.empty(); }

			/// addUse/killUse - These two methods should only be used by the Use class.
			void addUse(Use& U)		{}
			void killUse(Use& U)	{}
		};
	}
}
#endif