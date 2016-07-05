//===----------------------------ConstantAndGlobal.hh---------------------===//
//
// This file contains the declaration of the Constant class.
// 
// GlobalValue is a common base class of all globally definable objects. As
// such, it is subclassed by GlobalVariable and by Function. This is used
// because you can do certain things with these global objects that you can't
// do to anything else. For example, use the address of one as a constant.
// 参照llvm1.0
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_CONSTANT_AND_GLOBAL_H
#define MOSES_IR_CONSTANT_AND_GLOBAL_H
#include "User.h"

namespace compiler
{
	namespace IR
	{
		class ArrayType;
		class StructType;
		class PointerType;
		class Constant;

		using ConstantPtr = std::shared_ptr<Constant>;

		/// All constants share the capabilities provided in this class. All constants
		/// can have a null value. They can have an operand list. Constants can be
		/// simple (integer values), complex (structures), or expression based(
		/// computations yielding a constant value composed of only certain operators
		/// and other constant values).
		///
		/// Note that Constants are immutable (once created they never change) and
		/// are fully shared by structural equivalence. This means that two
		/// structurally equivalent constants will always have the same address.
		/// Constants are created on demand as needed and never deleted: thus clients
		/// don't have to worry about the lifetime of the objects.
		/// @brief moses IR Constant Representation
		class Constant : public User
		{
			void operator=(const Constant&) = delete;
			Constant(const Constant&) = delete;
		protected:
			Constant(TyPtr Ty) : User(Ty, Value::ValueTy::ConstantVal){}
			~Constant() {}
		public:
			// setName - Specialize setName to handle symbol table majik...(把戏)
			// virtual void setName(const std::string &name, SymbolTable* ST = 0);

			/// isConstantExpr - Return true if this is a ConstantExpr
			virtual bool isConstantExpr() const { return false; }

			/// destoryConstant - Called if some element of this constant is no longer
			/// valid. At this point only other constants may be on the use_list for this
			/// constant. Any constants on our Use list must also be destory'd. The
			/// implementation must be sure to remove the constants from the list of 
			/// available cached constants. Implementations should call destoryConstantImpl
			/// as the last thing they do, to destory all users and delete this.
			///
			/// Note that this call is only valid on non-premitive constants: You cannot
			/// destory an integer constant for example. This API is used to delete
			/// constants that have ConstantPointerRef's embeded in them when the module
			/// is deleted, and it is used by globalDCE to remove ConstantPointerRefs 
			/// that are unneeded, allowing globals to be DCE'd.
			virtual void destoryConstant() {}

			/// Return true if the value is one. ???
			bool isOneValue() const;

			/// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(const Constant*) { return true; }
			static bool classof(const Value* V)
			{
				return V->getValueType() == Value::ValueTy::ConstantVal;
			}

			/// replaceUsesOfWithOnConstant - This method is a special form of
			/// User::replaceUsesOfWith(which does not work on constants) that does work
			/// on constants. Basicall this method goes through the trouble of building
			/// a new constant that is equivalent to the current one, with all uses of 
			/// From replaced woth uses of To. After this construction is completed, all
			/// of the users of 'this' are replaced to use the new constant, and then 
			/// this is deleted. Ingeral, you should not call this method, instead, use
			/// Value::replaceAllUsesWith, which automatically dispatches to this method
			/// as needed.
		};

		/// As such, it is subclassed by GlobalVariable and by Function. This is used 
		/// because you can do certain things with these global objects that you can't do
		/// to anything else. For example. use the address of one as a constant.
		/// ( 感觉 LLVM 起初有种过度设计的感觉 )
		class PointerType;
		class GlobalValue : public User
		{
			GlobalValue(const GlobalValue&) = delete;
		protected:
			GlobalValue(TyPtr Ty, ValueTy vty, const std::string &name = "") :
				User(Ty, vty, name) {}
		public:
			~GlobalValue() {}

			const PointerType *getType() const
			{
				// LLVM 1.0使用的还是C语言的类型转换函数
			}

			static bool classof(const GlobalValue* T) { return true; }
			static bool classof(const Value* V)
			{
				return V->getValueType() == Value::ValueTy::FunctionVal ||
					V->getValueType() == Value::ValueTy::GlobalVariableVal;
			}
		};


		/// \brief Global variables are constant pointers that refer to hunks of(大块的)
		/// space that are allocated by either the VM, or by the linker in a static 
		/// compiler. A global variable may have an initial value, which is copied into 
		/// the executables .data area. Global Constants are required to have initializers.
		class GlobalVariable : public GlobalValue
		{
			bool isConstantGlobal;
		public:
			/// GlobalVariable ctor - If a parent module is specifiedm the global is 
			/// automatically inserted into the end of the specified modules global list.
			GlobalVariable(const Type *Ty, bool isConstant, Constant *Initializer = 0,
				const std::string &Name = "");

			/// The initializer for the global variable/constant is held by Operands[0]
			/// if an initializer is specified.
			bool hasInitializer() const {}

			/// getInitializer - Return the initializer for this global variable.
			Constant* getInitializer() const
			{
				return nullptr;
			}

			void setInitializer(Constant *CPV)
			{

			}

			// If the value is a global constant, its value is immutable throughout the
			// runtime execution of the program. Assigning a value into the constant
			// leads to undefined behavior.
			bool isConstant() const { return isConstantGlobal; }
			void setConstant(bool Value) { isConstantGlobal = Value; }

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(const GlobalVariable*) { return true; }
			static bool classof(const Value* V)
			{
				return V->getValueType() == Value::ValueTy::GlobalVariableVal;
			}
		};

		//===------------------------------------------------------------------===//
		// This is the shared class of boolean and integer constants. This class 
		// represents both boolean and integral constants.
		// @brief Class for constant integers.
		// 注：在moses IR中bool使用i1表示
		class ConstantIntegral : public Constant
		{
			ConstantIntegral(const ConstantIntegral&) = delete;
		protected:
			ConstantIntegral() : Constant(std::make_shared<Type>(Type::IntegerTy)) {}
		public:
			/// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(const ConstantIntegral *) { return true; }
			static bool classof(const Constant *CPV);	// defined in Constants.cpp
			// To Do:
			static bool classof(const Value* V)
			{
				return true;
			}
		};

		//===----------------------------------------------------------------===//
		// ConsrantBool - Boolean Values
		class ConstantBool final : public ConstantIntegral
		{
		private:
			bool Val;
		public:			
			ConstantBool(bool val) : Val(val) {}
			static ConstantBool *True, *False;
			
			/// getTrue() - static factory methods - Return objects of the specified value.
			static ConstantBoolPtr getTrue() { return std::make_shared<ConstantBool>(true); }

			/// getFalse() - static factory methods - Return objects of the specified value.
			static ConstantBoolPtr getFalse() { return std::make_shared<ConstantBool>(false); }

			/// inverted - Return the opposite value of the current value.
			ConstantBool * inverted() const { return (this == True) ? False : True; }

			/// getVal - return the boolean value of this constant.
			bool getVal() const { return Val; }		

			/// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(const ConstantBool *) { return true; }

			static bool classof(const Constant *CPV)
			{
				return (CPV == True) | (CPV == False);
			}

			static bool classof(const Value* V)
			{
				return true;
			}
		};

		// 在moses中，暂时只有int和bool类型，int类型可以使用i32表示。
		class ConstantInt final : public ConstantIntegral
		{
			int Val;
			ConstantInt(const ConstantInt&) = delete;
		public:
			ConstantInt(int val) : Val(val) {}

			/// \brief Get a ConstantInt for a specific value.
			static ConstantIntPtr get(int value) { return std::make_shared<ConstantInt>(value);}

			/// equalsInt - 提供一个工具方法来判断当前值是否等于某个特定的值
			bool equalsInt(int v) const { return Val == v; }

			/// 获取constant val.
			int getVal() const { return Val; }

			static ConstantIntPtr getZeroValueForNegative()
			{
				return std::make_shared<ConstantInt>(0);
			}

			static bool classof(ValPtr V)
			{
				return V->getValueType() == Value::ValueTy::ConstantVal;
			}
		};
		
		//===----------------------------------------------------------------===//
		// ConstantSturct - Constant Struct Declarations
		class ConstantStruct : public Constant
		{
			ConstantStruct(const ConstantStruct&) = delete;
		protected:
			ConstantStruct(const StructType* T, const std::vector<Constant*> &Val);
		public:
			/// get() - Static factory methods - Return objects of the specified value
			static ConstantStruct* get(const StructType* T, 
				const std::vector<Constant*> &V);

			/// getType() specialization - Reduce amount of casting...
			const StructType *getType() const
			{
				return nullptr;
			}

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(std::shared_ptr<ConstantStruct>) { return true; }

			/// getValues - Return a vector of the component constants that make up
			/// this structure.
			// const std::vector<Use> &getValue() const { return Operands; }
		};


		// ConstantExpr - a constant value that is initialized with an expression using
		// other constant values.
		class ConstantExpr
		{
		};
	}
}

#endif