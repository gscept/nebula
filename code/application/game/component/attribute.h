#pragma once
#include "core/types.h"
#include "util/fourcc.h"
#include "accessmode.h"
#include "valuetype.h"

namespace Game
{

template<typename T>
constexpr Attr::ValueType TypeToValueType()
{
	static_assert(false, "Type is not implemented!");
	return Attr::ValueType::NONE;
}

template<> constexpr Attr::ValueType TypeToValueType<bool>()				{ return Attr::ValueType::BoolType; }
template<> constexpr Attr::ValueType TypeToValueType<int>()				    { return Attr::ValueType::IntType; }
template<> constexpr Attr::ValueType TypeToValueType<uint>()				{ return Attr::ValueType::UIntType; }
template<> constexpr Attr::ValueType TypeToValueType<float>()				{ return Attr::ValueType::FloatType; }
template<> constexpr Attr::ValueType TypeToValueType<double>()			    { return Attr::ValueType::DoubleType; }
template<> constexpr Attr::ValueType TypeToValueType<Math::matrix44>()	    { return Attr::ValueType::Matrix44Type; }
template<> constexpr Attr::ValueType TypeToValueType<Math::float4>()		{ return Attr::ValueType::Float4Type; }
template<> constexpr Attr::ValueType TypeToValueType<Math::quaternion>()	{ return Attr::ValueType::QuaternionType; }
template<> constexpr Attr::ValueType TypeToValueType<Util::String>()		{ return Attr::ValueType::StringType; }
template<> constexpr Attr::ValueType TypeToValueType<Util::Guid>()		    { return Attr::ValueType::GuidType; }
template<> constexpr Attr::ValueType TypeToValueType<Game::Entity>()		{ return Attr::ValueType::EntityType; }

class Attribute
{
public:
	Attribute() :
		attrIndex(-1),
		fourcc(-1),
		name(""),
		type(Attr::ValueType::VoidType),
		accessMode(Attr::AccessMode::ReadOnly),
		defaultValue()
	{
		// empty
	}

	Attribute(uint index, Util::FourCC fourcc, const char* name, Attr::ValueType type, const char* typeName, Attr::AccessMode accessMode, Util::Variant const& defaultValue) :
		attrIndex(index),
		fourcc(fourcc),
		name(name),
		type(type),
        typeName(typeName),
		accessMode(accessMode),
		defaultValue(defaultValue)
	{
		// empty
	}

	using InnerType = uint;

	uint attrIndex;
	Util::FourCC fourcc;
	Util::String name;
    Util::String typeName;
    Attr::ValueType type;
	Util::Variant defaultValue;
    Attr::AccessMode accessMode;

    /*
        These static functions are implemented by each attribute when using
        the __DeclareAttribute macro.
        
	static constexpr Util::FourCC FourCC()
	{
		return Util::FourCC('FOUR');
	}
	static constexpr const char* Name()
	{
		return "AttributeName";
	}
	static constexpr const char* TypeName()
	{
		return "uint";
	}
	static constexpr Attr::ValueType Type()
	{
		return TypeToValueType<uint>();
	}
	static constexpr uint DefaultValue()
	{
		return uint(-1);
	}
    */
};

//------------------------------------------------------------------------------
/**
	@note	Make sure to send an explicit type as default value (ex. uint(10), Math::matrix44::identity(), etc.)
*/
#define __DeclareAttribute(ATTRIBUTENAME, TYPE, FOURCC, ACCESSMODE, DEFAULTVALUE) \
class ATTRIBUTENAME : public Game::Attribute\
{\
public:\
	ATTRIBUTENAME(uint index) : Attribute(index, FOURCC, #ATTRIBUTENAME, Game::TypeToValueType<TYPE>(), #TYPE, ACCESSMODE, Util::Variant(DEFAULTVALUE)) {};\
	using InnerType = TYPE;\
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
		return #TYPE;\
	}\
	static constexpr Attr::ValueType Type()\
	{\
		return Game::TypeToValueType<TYPE>();\
	}\
	static const TYPE DefaultValue()\
	{\
		return TYPE(DEFAULTVALUE);\
	}\
};

__DeclareAttribute(TestAttr, int, 'TEST', Attr::AccessMode::ReadWrite, int(12));
__DeclareAttribute(TestFloatAttr, float, 'TFLT', Attr::AccessMode::ReadOnly, float(20));

} // namespace Attr
