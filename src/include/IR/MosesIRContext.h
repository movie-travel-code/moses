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
#include "IRType.h"
#include "Function.h"
#include "ConstantAndGlobal.h"
#include "../Support/Hasing.h"
#include "../Support/TypeSet.h"
namespace compiler
{
	namespace IR
	{		
		using namespace std;
		using namespace Hashing;
		using namespace SupportStructure;

		struct FunctionTypeKeyInfo
		{
			struct KeyTy
			{
				std::shared_ptr<Type> ReturnType;
				std::vector< std::shared_ptr<Type>> Params;
				KeyTy(std::shared_ptr<Type> R, std::vector< std::shared_ptr<Type>> P) : ReturnType(R), Params(P) {}

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
			static unsigned long long getHashValue(std::shared_ptr<FunctionType> FT) { return getHashValue(KeyTy(FT)); }
			static bool isEqual(const KeyTy& LHS, std::shared_ptr<FunctionType> RHS) { return LHS == KeyTy(RHS); }
			static bool isEqual(std::shared_ptr<FunctionType> LHS, std::shared_ptr<FunctionType> RHS) { return LHS == RHS; }
		};

		struct AnonStructTypeKeyInfo
		{
			struct KeyTy
			{
				std::vector< std::shared_ptr<Type>> ETypes;
				// bool ispacked;
				KeyTy(const std::vector< std::shared_ptr<Type>>& E) : ETypes(E) {}
				KeyTy(std::shared_ptr<StructType> ST) : ETypes(ST->getContainedTys()) {}
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
			static unsigned long long getHashValue(std::shared_ptr<StructType> RHS) { return getHashValue(KeyTy(RHS)); }
			static bool isEqual(const KeyTy& LHS, std::shared_ptr<StructType> RHS) { return LHS == KeyTy(RHS); }
			static bool isEqual(std::shared_ptr<StructType> LHS, std::shared_ptr<StructType> RHS) { return LHS == RHS; }
		};

		struct NamedStructTypeKeyInfo
		{
			struct KeyTy
			{
				std::vector< std::shared_ptr<Type>> ETypes;
				std::string Name;
				// bool ispacked;
				KeyTy(const std::vector< std::shared_ptr<Type>>& E, std::string Name) : ETypes(E), Name(Name) {}
				KeyTy(std::shared_ptr<StructType> ST) : ETypes(ST->getContainedTys()) {}
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
			static unsigned long long getHashValue(std::shared_ptr<StructType> RHS)
			{
				return getHashValue(KeyTy(RHS));
			}
			static bool isEqual(const KeyTy& LHS, std::shared_ptr<StructType> RHS)
			{
				return LHS == KeyTy(RHS);
			}
			static bool isEqual(std::shared_ptr<StructType> LHS, std::shared_ptr<StructType> RHS)
			{
				return LHS == RHS;
			}
		};

		class MosesIRContext
		{
			std::vector<std::shared_ptr<Intrinsic>> Intrinsics;

			// Basic type instances.
			// To Do: 为了减少对内存的占用，我们在MosesIRContext中存放VoidTy和IntTy，
			// 但是我们统一使用std::shared_ptr<>进行内存管理，所以反而增加了对内存的
			// 占用，这样的内存占用显然是很大的。
			std::shared_ptr<Type> VoidTy, IntTy, BoolTy, LabelTy;
			std::shared_ptr<ConstantBool> TheTrueVal, TheFalseVal;

			TypeSet<std::shared_ptr<FunctionType>, FunctionTypeKeyInfo> FunctionTypeSet;
			TypeSet<std::shared_ptr<StructType>, AnonStructTypeKeyInfo> StructTypeSet;
			TypeSet<std::shared_ptr<StructType>, NamedStructTypeKeyInfo> NamedStructTypeSet;
		public:
			MosesIRContext() : VoidTy(std::make_shared<Type>(Type::TypeID::VoidTy)), 
				IntTy(std::make_shared<Type>(Type::TypeID::IntegerTy)),
				BoolTy(std::make_shared<Type>(Type::TypeID::BoolTy)),
				LabelTy(std::make_shared<Type>(Type::TypeID::LabelTy))
			{
				std::vector<std::string> Names = {"dst", "src"};
				Intrinsics.push_back(std::make_shared<Intrinsic>("mosesir.memcpy", Names));
			}
			/// \brief Add literal structure type(AnonymousType).
			void AddStructType(std::shared_ptr<StructType> Type)
			{
				StructTypeSet.insert(Type);
			}

			std::shared_ptr<Intrinsic> getMemcpy() const { return Intrinsics[0]; }

			/// \brief Add named structure type(UserdefinedType - class).
			void AddNamedStructType(std::shared_ptr<StructType> Type)
			{
				NamedStructTypeSet.insert(Type);
			}
			
			/// \brief 检查某种类型在GlobalType中是否存在.
			bool CheckHave(std::shared_ptr<StructType> forchecking);

			// helper method.
			std::shared_ptr<Type> getVoidTy() const { return VoidTy; }
			std::shared_ptr<Type> getIntTy() const { return IntTy; }
			std::shared_ptr<Type> getBoolTy() const { return BoolTy; }
			std::shared_ptr<Type> getLabelTy() const { return LabelTy; }

			std::vector<std::shared_ptr<StructType>> getAnonyTypes() const
			{
				return StructTypeSet.getBuckets();
			}

			std::vector<std::shared_ptr<StructType>> getNamedTypes() const
			{
				return NamedStructTypeSet.getBuckets();
			}
		};
	}
}
#endif