//===--------------------------------Use.h--------------------------------===//
//
// This defines the Use class. The Use class represents the operand of an 
// instruction or some other User instance which refers to a Value. The Use
// class keeps the "use list" of the referenced value up to date.
//
//===---------------------------------------------------------------------===//
/// LLVM IR中的SSA value的def-use与use-def信息其实是用同一套双向引用来维护的。
/// 所以说只要A use了B，A与B之间的use-def与def-use关系就同时建好了。这个维护双向
/// 引用的数据结构叫做'Use'。LLVM中代码写的那么复杂的主要原因"The Use helper class 
/// is employed to do the bookkeeping and to facilitate O(1) addition and removal."
#ifndef MOSES_IR_USE_H
#define MOSES_IR_USE_H
#include <memory>

namespace compiler
{
	namespace IR
	{
		class Value;
		class User;

		/// \brief A Use represents the edge between a Value definition and its users.
		/// 
		/// This is notionally a two-dimensional linked list. It supports traversing 
		/// all of the uses for a particular value definition. It also supporrts jumping
		/// directly to the used value when we arrive from the User's operands, and
		/// jumping directly to the User when we arrive from the Value's uses.
		///
		/// This is essentially the single most memory intensive object in LLVM(moses IR)
		/// because of the number of uses in the system. At the same time, the constant
		/// time operations it allows are essential to many optimizations having reasonable
		/// time complexity.
		class Use
		{
		public:
			typedef std::shared_ptr<Use> UsePtr;
			typedef std::shared_ptr<Value> ValPtr;
			typedef std::shared_ptr<User> UserPtr;
		private:
			ValPtr Val;
			UserPtr U;
			UsePtr Prev;
			UsePtr Next;
		public:
			Use(std::shared_ptr<Value> v, std::shared_ptr<User> user);
			Use(const Use& u);
			~Use();

			operator std::shared_ptr<Value>() const{ return Val; }
			std::shared_ptr<Value> get() const { return Val; }
			std::shared_ptr<User> getUser() const { return U; }

			void set(std::shared_ptr<Value> Val);

			// ---------------------nonsense for coding--------------------------
			// value之间的使用关系用use对象关联起来，这里对use对象进行赋值，use = value;
			std::shared_ptr<Value> operator=(std::shared_ptr<Value> RHS)
			{
				set(RHS);
				return RHS;
			}

			const Use& operator=(std::shared_ptr<Use> RHS)
			{
				set(RHS->get());
				return *this;
			}

			std::shared_ptr<Use> getPrev() { return Prev; }
			std::shared_ptr<Use> getNext() { return Next; }

			void setPrev(std::shared_ptr<Use> Prev) { this->Prev = Prev; }
			void setNext(std::shared_ptr<Use> Next) { this->Next = Next; }
		};
	}
}
#endif