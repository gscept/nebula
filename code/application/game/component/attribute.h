#pragma once
#include "core/types.h"
#include "util/fourcc.h"
#include "attr/accessmode.h"
#include "attr/valuetype.h"
#include "attr/attributedefinitionbase.h"
#include "attr/attrid.h"

namespace Attr
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
template<> constexpr Attr::ValueType TypeToValueType<Math::mat4>()	        { return Attr::ValueType::Mat4Type; }
template<> constexpr Attr::ValueType TypeToValueType<Math::vec4>()		    { return Attr::ValueType::Float4Type; }
template<> constexpr Attr::ValueType TypeToValueType<Math::quat>()	        { return Attr::ValueType::QuaternionType; }
template<> constexpr Attr::ValueType TypeToValueType<Util::String>()		{ return Attr::ValueType::StringType; }
template<> constexpr Attr::ValueType TypeToValueType<Util::Guid>()		    { return Attr::ValueType::GuidType; }
template<> constexpr Attr::ValueType TypeToValueType<Game::Entity>()		{ return Attr::ValueType::EntityType; }

//------------------------------------------------------------------------------
/**
	@note	Make sure to send an explicit type as default value (ex. uint(10), Math::mat4, etc.)
*/
#define __DeclareAttribute(ATTRIBUTENAME, TYPE, FOURCC, ACCESSMODE, DEFAULTVALUE) \
namespace Runtime\
{\
extern const AttributeDefinitionBase ATTRIBUTENAME ## Id;\
}\
\
class ATTRIBUTENAME : public Attr::AttrId\
{\
public:\
    ATTRIBUTENAME()\
    {\
        this->defPtr = &Attr::Runtime::ATTRIBUTENAME ## Id;\
    };\
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
    	return Attr::TypeToValueType<TYPE>();\
    }\
    static const TYPE DefaultValue()\
    {\
    	return TYPE(DEFAULTVALUE);\
    }\
};

#define __DefineAttribute(ATTRIBUTENAME, TYPE, FOURCC, ACCESSMODE, DEFAULTVALUE) \
namespace Runtime\
{\
    const AttributeDefinitionBase ATTRIBUTENAME ## Id(Util::String(#ATTRIBUTENAME), #TYPE, Util::FourCC(FOURCC), ACCESSMODE, DEFAULTVALUE, false);\
}


} // namespace Attr
