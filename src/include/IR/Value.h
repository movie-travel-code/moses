//===-------------------------------Value.h-------------------------------===//
//
// This file defines the very important Value class. This is subclassed by a
// bunch of other important classes, like Instruction, Function, Type, etc...
// 
// moses IR�ο���LLVM IR��ע�Ͳ���ժ��LLVM Value.h (�²ۣ�ΪʲôҪ��Value�������)
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
///	Value��moses IR�еĻ��࣬�������﷨����statement��valueֻ����һЩ�򵥵Ĳ�����
/// moses IR��һ������IR��������CFG��Ϣ��ͨ��Block����֯����������SSA-based�ģ�
/// �����ں�def-use��Ϣ��Ϊ�˸�Ч�Ĳ��ң�ʹ��ɢ�б���Ϊ���ű�Ļ����ṹ����Ϊ��
///	���㣬���ű���ʱ���Ϊvector����
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
			// PS: LLVM IR����ζ���def-use���Ļ���û�и㶮��������ʱʵ���Լ���def-use����
			// ʹ�����õ�list��һϵ�е� 'use' object��������
			// -----------------------------------------------------------------------
			std::list<UsePtr> Uses;
			std::string Name;
			ValueTy VTy;
			TyPtr Ty;
			void operator=(const Value&) = delete;	// Do not implement
			Value(const Value&) = delete;			// Do not implement

		public:
			// Type��ʾ���ͣ�����int short class�ȵȡ�
			// ValueTy��ʾ����ָ�������.
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
			/// ��ָ��V��use���е�valueָ��ǰֵ��
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