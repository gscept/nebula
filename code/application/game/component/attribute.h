#pragma once
#include "core/types.h"
#include "util/fourcc.h"
#include "game/attr/accessmode.h"

namespace Attr
{

// template <typename 


enum AttributeType
{
	NONE,
	INT,
	UINT,
	FLOAT,
	STRING,
	FLOAT4,
	MATRIX44
};

template<typename T>
constexpr AttributeType TypeToAttributeType()
{
	static_assert(false, "Type is not implemented!");
	return AttributeType::NONE;
}

template<> constexpr AttributeType TypeToAttributeType<int>() { return AttributeType::INT; }
template<> constexpr AttributeType TypeToAttributeType<uint>() { return AttributeType::UINT; }
template<> constexpr AttributeType TypeToAttributeType<float>() { return AttributeType::FLOAT; }


class Attribute
{
public:
	Attribute() :
		attrIndex(-1),
		fourcc(-1),
		name(""),
		type(AttributeType::NONE),
		accessMode(Attr::AccessMode::ReadOnly),
		defaultValue()
	{
		// empty
	}

	Attribute(uint index, Util::FourCC fourcc, const char* name, AttributeType type, Attr::AccessMode accessMode, Util::Variant const& defaultValue) :
		attrIndex(index),
		fourcc(fourcc),
		name(name),
		type(type),
		accessMode(accessMode),
		defaultValue(defaultValue)
	{
		// empty
	}

	using InnerType = uint;

	const uint attrIndex;
	const Util::FourCC fourcc;
	const char* name;
	const AttributeType type;
	const Util::Variant defaultValue;
	const Attr::AccessMode accessMode;

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
	static constexpr AttributeType Type()
	{
		return TypeToAttributeType<uint>();
	}
	static constexpr uint DefaultValue()
	{
		return uint(-1);
	}
};

#define __DeclareAttribute(ATTRIBUTENAME, TYPE, FOURCC, ACCESSMODE, DEFAULTVALUE) \
class ATTRIBUTENAME : public Attribute\
{\
public:\
	ATTRIBUTENAME(uint index) : Attribute(index, FOURCC, #ATTRIBUTENAME, TypeToAttributeType<TYPE>(), ACCESSMODE, Util::Variant(TYPE(DEFAULTVALUE))) {};\
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
	static constexpr AttributeType Type()\
	{\
		return TypeToAttributeType<TYPE>();\
	}\
	static constexpr TYPE DefaultValue()\
	{\
		return TYPE(DEFAULTVALUE);\
	}\
};

__DeclareAttribute(TestAttr, int, 'TEST', AccessMode::ReadWrite, 12);
__DeclareAttribute(TestFloatAttr, float, 'TFLT', AccessMode::ReadOnly, 20);

} // namespace Attr
