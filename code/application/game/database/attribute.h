#pragma once
#include "core/types.h"
#include "util/fourcc.h"
#include "resources/resourceid.h"
#include <variant>
#include "util/guid.h"
#include "util/stringatom.h"
#include "util/string.h"
#include "game/entity.h"
#include "math/matrix44.h"
#include "math/float2.h"
#include "math/float4.h"
#include "util/fourcc.h"
#include "util/hashtable.h"
#include "attr/accessmode.h"

namespace Game
{

enum class AttributeType
{
	// mem-copyable types
	Int8Type,
	UInt8Type,
	Int16Type,
	UInt16Type,
	Int32Type,
	UInt32Type,
	Int64Type,
	UInt64Type,
	FloatType,
	DoubleType,
	BoolType,
	Float2Type,
	Float4Type,
	QuaternionType,
	Matrix44Type,
	GuidType,
	EntityType,

	// complex types
	StringType,
	//StringAtomType,
	//ResourceNameType,
	//LocalizedStringType,


	NumAttributeTypes
};

using AttributeValue = std::variant<
	int8_t,
	uint8_t,
	int16_t,
	uint16_t,
	int32_t,
	uint32_t,
	int64_t,
	uint64_t,
	float,
	double,
	bool,
	Math::float2,
	Math::float4,
	Math::quaternion,
	Math::matrix44,
	Util::Guid,
	Game::Entity,
	Util::String
>;

using AttributeCategoryTable = Util::HashTable<CategoryId, void**, 32, 1>;

struct AttributeDefinition
{
	Util::String name;
	Util::FourCC fourcc;
	Game::AttributeType type;
	Game::AttributeValue defaultValue;
	Attr::AccessMode accessMode;

	AttributeCategoryTable* registry;
};

template<typename T>
constexpr Game::AttributeType TypeToAttributeType()
{
	static_assert(false, "Type is not implemented!");
	return Game::AttributeType::NONE;
}

template<> constexpr Game::AttributeType TypeToAttributeType<bool>()				{ return Game::AttributeType::BoolType; }
template<> constexpr Game::AttributeType TypeToAttributeType<int32_t>()				{ return Game::AttributeType::Int32Type; }
template<> constexpr Game::AttributeType TypeToAttributeType<uint32_t>()			{ return Game::AttributeType::UInt32Type; }
template<> constexpr Game::AttributeType TypeToAttributeType<float>()				{ return Game::AttributeType::FloatType; }
template<> constexpr Game::AttributeType TypeToAttributeType<double>()				{ return Game::AttributeType::DoubleType; }
template<> constexpr Game::AttributeType TypeToAttributeType<Math::matrix44>()		{ return Game::AttributeType::Matrix44Type; }
template<> constexpr Game::AttributeType TypeToAttributeType<Math::float4>()		{ return Game::AttributeType::Float4Type; }
template<> constexpr Game::AttributeType TypeToAttributeType<Math::quaternion>()	{ return Game::AttributeType::QuaternionType; }
template<> constexpr Game::AttributeType TypeToAttributeType<Util::String>()		{ return Game::AttributeType::StringType; }
template<> constexpr Game::AttributeType TypeToAttributeType<Util::Guid>()			{ return Game::AttributeType::GuidType; }
template<> constexpr Game::AttributeType TypeToAttributeType<Game::Entity>()		{ return Game::AttributeType::EntityType; }


constexpr SizeT
GetAttributeSize(Game::AttributeType type)
{
	switch (type)
	{
	//case VoidType:
	//	static_assert("Cannot get size of void type!");
	//	return 0;

	case Game::AttributeType::Int8Type:			return sizeof(int8_t);
	case Game::AttributeType::UInt8Type:		return sizeof(uint8_t);
	case Game::AttributeType::Int16Type:		return sizeof(int16_t);
	case Game::AttributeType::UInt16Type:		return sizeof(uint16_t);
	case Game::AttributeType::Int32Type:		return sizeof(int32_t);
	case Game::AttributeType::UInt32Type:		return sizeof(uint32_t);
	case Game::AttributeType::Int64Type:		return sizeof(int64_t);
	case Game::AttributeType::UInt64Type:		return sizeof(uint64_t);
	case Game::AttributeType::FloatType:		return sizeof(float);
	case Game::AttributeType::DoubleType:		return sizeof(double);
	case Game::AttributeType::BoolType:			return sizeof(bool);
	case Game::AttributeType::Float2Type:		return sizeof(Math::float2);
	case Game::AttributeType::Float4Type:		return sizeof(Math::float4);
	case Game::AttributeType::QuaternionType:	return sizeof(Math::quaternion);
	case Game::AttributeType::Matrix44Type:		return sizeof(Math::matrix44);
	case Game::AttributeType::GuidType:			return sizeof(Util::Guid);
	case Game::AttributeType::EntityType:		return sizeof(Game::Entity);
	case Game::AttributeType::StringType:		return sizeof(Util::String);
	default:
		static_assert("Invalid type!");
		n_error("Invalid type!");
		return 0;
	}
}

class AttributeId
{
public:
	AttributeId() : defPtr(nullptr) {}
	AttributeId(Game::AttributeDefinition const& def) :
		defPtr(&def)
	{
		// empty
	}
	~AttributeId()
	{
		// empty
	}

	AttributeCategoryTable* GetCategoryTable() const
	{
		return defPtr->registry;
	}

	bool IsValid() const
	{
		return this->defPtr != nullptr;
	}

	const Util::FourCC GetFourCC() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->fourcc;
	}

	const Util::String& GetName() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->name;
	}

	const AttributeType GetType() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->type;
	}

	const Attr::AccessMode GetAccessMode() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->accessMode;
	}

	const AttributeValue& GetDefaultValue() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->defaultValue;
	}

	void operator=(AttributeId const& rhs)
	{
		this->defPtr = rhs.defPtr;
	}

	bool operator<(AttributeId const rhs) const
	{
		return this->defPtr < rhs.defPtr;
	}

	bool operator>(AttributeId const rhs) const
	{
		return this->defPtr > rhs.defPtr;
	}

	bool operator<=(AttributeId const rhs) const
	{
		return this->defPtr <= rhs.defPtr;
	}

	bool operator>=(AttributeId const rhs) const
	{
		return this->defPtr >= rhs.defPtr;
	}

	bool operator==(AttributeId const rhs) const
	{
		return this->defPtr == rhs.defPtr;
	}
	
	bool operator!=(AttributeId const rhs) const
	{
		return this->defPtr != rhs.defPtr;
	}
	
	IndexT HashCode() const
	{
		n_assert(this->defPtr != nullptr);
		return this->defPtr->fourcc.HashCode();
	}

private:
	Game::AttributeDefinition const* defPtr;
};

using Attribute = Util::KeyValuePair<AttributeId, AttributeValue>;

//------------------------------------------------------------------------------
/**
	@note	The ATTRIBUTENAME class is kinda wonky, but for good reason. We want to be able to use the attribute types
			during compile time, which requires a class type if we want to be able to deduce the actual inner type of the
			attribute. We DON'T want to instantiate that class however, hence the deleted constructors. So essentially, the
			class only acts as compile time attribute information. The runtime namespace can be used during runtime.
	
	@note	Make sure to send an explicit type as default value (ex. uint32_t(10), Math::matrix44::identity(), etc.)
*/
#define __DeclareAttribute(ATTRIBUTENAME, ACCESSMODE, VALUETYPE, FOURCC, DEFAULTVALUE) \
namespace Runtime\
{\
extern Game::AttributeDefinition ATTRIBUTENAME ## Id;\
}\
\
class ATTRIBUTENAME\
{\
public:\
	ATTRIBUTENAME() = delete;\
	~ATTRIBUTENAME() = delete;\
	using TYPE = VALUETYPE;\
	static const Game::Attribute Create(VALUETYPE v) { return { ATTRIBUTENAME::Id(), v }; }  \
	static constexpr uint FourCC()\
	{\
		return FOURCC;\
	}\
	static constexpr const char* Name()\
	{\
		return #ATTRIBUTENAME;\
	}\
	static constexpr const char* TypeName()\
	{\
		return #VALUETYPE;\
	}\
	static constexpr Game::AttributeType Type()\
	{\
		return Game::TypeToAttributeType<VALUETYPE>();\
	}\
	static constexpr Attr::AccessMode AccessMode()\
	{\
		return ACCESSMODE;\
	}\
	static const VALUETYPE DefaultValue()\
	{\
		return VALUETYPE(DEFAULTVALUE);\
	}\
	inline static Game::AttributeId Id()\
	{\
		return Runtime::ATTRIBUTENAME ## Id;\
	}\
};

#define __DefineAttribute(ATTRIBUTENAME) \
namespace Runtime\
{\
	Game::AttributeDefinition ATTRIBUTENAME ## Id = {\
		Util::String(#ATTRIBUTENAME), \
		Util::FourCC(ATTRIBUTENAME::FourCC()), \
		ATTRIBUTENAME::Type(), \
		Game::AttributeValue(ATTRIBUTENAME::DefaultValue()), \
		ATTRIBUTENAME::AccessMode(), \
		n_new(Game::AttributeCategoryTable()) \
	}; \
}


} // namespace Attr
