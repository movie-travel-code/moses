//===--------------------------------MosesIRContext.h---------------------===//
//
// This file declares MosesIRContext, a container of "global" state in moes IR,
// such as the global type and constant uniquing tables.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_CONTEXT_H
#define MOSES_IR_CONTEXT_H
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "../IR/Function.h"
#include "../Support/Hasing.h"
#include "../IR/ConstantAndGlobal.h"
#include "IRType.h"
#include "../Support/TypeSet.h"
namespace compiler
{
	namespace IR
	{
		typedef std::shared_ptr<Type> TyPtr;
		typedef std::shared_ptr<FunctionType> FuncTypePtr;
		using namespace std;
		using namespace Hashing;
		using namespace SupportStructure;
		struct FunctionTypeKeyInfo
		{
			struct KeyTy
			{
				TyPtr ReturnType;
				std::vector<TyPtr> Params;
				KeyTy(TyPtr R, std::vector<TyPtr> P) : ReturnType(R), Params(P) {}

				KeyTy(std::shared_ptr<FunctionType> FT) :
					ReturnType(FT->getReturnType()), Params(FT->getParams()) {}

				bool operator==(const KeyTy& rhs) const
				{
					if (ReturnType.get() != rhs.ReturnType.get())
						return false;
					if (Params == rhs.Params)
						return true;
					return false;
				}
				bool operator!=(const KeyTy& rhs) const { return !this->operator==(rhs); }
			};
			static unsigned long long getHashValue(const KeyTy& Key)
			{
				return hash_combine_range(hash_value(Key.ReturnType), Key.Params.begin(), 
					Key.Params.end());
			}
			static unsigned long long getHashValue(FuncTypePtr FT) { return getHashValue(KeyTy(FT)); }
			static bool isEqual(const KeyTy& LHS, FuncTypePtr RHS) { return LHS == KeyTy(RHS); }
			static bool isEqual(FuncTypePtr LHS, FuncTypePtr RHS) { return LHS == RHS; }
		};

		struct AnonStructTypeKeyInfo
		{
			struct KeyTy
			{
				std::vector<TyPtr> ETypes;
				// bool ispacked;
				KeyTy(const std::vector<TyPtr>& E) : ETypes(E) {}
				KeyTy(StructTypePtr ST) : ETypes(ST->getContainedTys()) {}
				bool operator==(const KeyTy& rhs) const
				{
					if (ETypes == rhs.ETypes)
						return true;
					return false;
				}
				bool operator!=(const KeyTy& rhs) const { return !this->operator==(rhs); }
			};
			static unsigned long long getHashValue(const KeyTy& Key)
			{
				return hash_combine_range(0, Key.ETypes.begin(), Key.ETypes.end());
			}
			static unsigned long long getHashValue(StructTypePtr RHS) { return getHashValue(KeyTy(RHS)); }
			static bool isEqual(const KeyTy& LHS, StructTypePtr RHS) { return LHS == KeyTy(RHS); }
			static bool isEqual(StructTypePtr LHS, StructTypePtr RHS) { return LHS == RHS; }
		};

		struct NamedStructTypeKeyInfo
		{
			struct KeyTy
			{
				std::vector<TyPtr> ETypes;
				std::string Name;
				// bool ispacked;
				KeyTy(const std::vector<TyPtr>& E, std::string Name) : ETypes(E), Name(Name) {}
				KeyTy(StructTypePtr ST) : ETypes(ST->getContainedTys()) {}
				bool operator==(const KeyTy& rhs) const
				{
					if (Name == rhs.Name)
						return false;
					if (ETypes == rhs.ETypes)
						return true;
					return false;
				}
				bool operator!=(const KeyTy& rhs) const { return !this->operator==(rhs); }
			};
			static unsigned long long getHashValue(const KeyTy& Key)
			{
				return hash_combine_range(hash_value(Key.Name), Key.ETypes.begin(), Key.ETypes.end());
			}
			static unsigned long long getHashValue(StructTypePtr RHS)
			{
				return getHashValue(KeyTy(RHS));
			}
			static bool isEqual(const KeyTy& LHS, StructTypePtr RHS)
			{
				return LHS == KeyTy(RHS);
			}
			static bool isEqual(StructTypePtr LHS, StructTypePtr RHS)
			{
				return LHS == RHS;
			}
		};

		class MosesIRContext
		{
			typedef std::shared_ptr<StructType> StructTypePtr;
		private:
			std::vector<IntrinsicPtr> Intrinsics;

			// Basic type instances.
			// To Do: 为了减少对内存的占用，我们在MosesIRContext中存放VoidTy和IntTy，
			// 但是我们统一使用std::shared_ptr<>进行内存管理，所以反而增加了对内存的
			// 占用，这样的内存占用显然是很大的。
			std::shared_ptr<Type> VoidTy, IntTy, BoolTy;
			std::shared_ptr<ConstantBool> TheTrueVal, TheFalseVal;

			TypeSet<FuncTypePtr, FunctionTypeKeyInfo> FunctionTypeSet;
			TypeSet<StructTypePtr, AnonStructTypeKeyInfo> StructTypeSet;
			TypeSet<StructTypePtr, NamedStructTypeKeyInfo> NamedStructTypeSet;
		public:
			MosesIRContext() : VoidTy(std::make_shared<Type>(Type::TypeID::VoidTy)), 
				IntTy(std::make_shared<Type>(Type::TypeID::IntegerTy)),
				BoolTy(std::make_shared<Type>(Type::TypeID::BoolTy))
			{
				std::vector<std::string> Names = {"dst", "src"};
				Intrinsics.push_back(std::make_shared<Intrinsic>("mosesir.memcpy", Names));
			}
			/// \brief Add literal structure type(AnonymousType).
			void AddStructType(StructTypePtr Type)
			{
				StructTypeSet.insert(Type);
			}

			IntrinsicPtr getMemcpy() const { return Intrinsics[0]; }

			/// \brief Add named structure type(UserdefinedType - class).
			void AddNamedStructType(StructTypePtr Type)
			{
				NamedStructTypeSet.insert(Type);
			}
			
			/// \brief 检查某种类型在GlobalType中是否存在.
			bool CheckHave(StructTypePtr forchecking);

			// helper method.
			std::shared_ptr<Type> getVoidTy() const { return VoidTy; }
			std::shared_ptr<Type> getIntTy() const { return IntTy; }
			std::shared_ptr<Type> getBoolTy() const { return BoolTy; }

			std::vector<StructTypePtr> getAnonyTypes() const
			{
				return StructTypeSet.getBuckets();
			}

			std::vector<StructTypePtr> getNamedTypes() const
			{
				return NamedStructTypeSet.getBuckets();
			}
		};
	}
}
#endif