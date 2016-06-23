//===-------------------------------TypeSet.h-----------------------------===//
//
// This file defines the TypeSet class.
//
//===---------------------------------------------------------------------===//
#ifndef TYPE_SET_H
#define TYPE_SET_H
#include <vector>
namespace compiler
{
	namespace SupportStructure
	{
		template<typename ValueT, typename ValueInfoT>
		class TypeSet
		{
			std::vector<ValueT> Buckets;
		public:
			typedef ValueT key_type;
			typedef ValueT value_type;
			typedef unsigned size_type;

			explicit TypeSet(unsigned NumInit = 0) {}

			bool empty() const { return Buckets.empty(); }

			size_type size() const { return Buckets.size(); }

			bool isIn(const ValueT &V) const 
			{
				if (lookup(V))
					return true;
				return false;
			}

			ValueT lookup(const ValueT &V) const
			{
				for (auto item : Buckets)
				{
					if (ValueInfoT::getHashValue(item) == ValueInfoT::getHashValue(V))
						return item;
				}
				return nullptr;
			}

			/// Note: 插入之前先通过ValueInfoT计算HashValue，并查找是否存在。
			void insert(ValueT V) 
			{
				if (lookup(V))
				{
					return;
				}
				Buckets.push_back(V); 
			}
		};
	}
}
#endif