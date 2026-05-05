#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeDefinition

    This extends the typeless AttributeDefinitionBase class by a typed
    template class, which adds compiletime-type-safety to attribute definitions.

    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "attributedefinitionbase.h"

//------------------------------------------------------------------------------
namespace Attr
{
template<class VALUETYPE, class TYPE>
class AttributeDefinition : public AttributeDefinitionBase
{
public:
    /// constructor
    AttributeDefinition(const Util::String& n, const Util::String& tn, const Util::FourCC& fourCC, AccessMode am, TYPE defVal);

    /// Inner type
    using AttrDeclType = TYPE;
};

//------------------------------------------------------------------------------
/**
*/
template<class VALUETYPE, class TYPE>
AttributeDefinition<VALUETYPE, TYPE>::AttributeDefinition(const Util::String& n, const Util::String& tn, const Util::FourCC& fcc, Attr::AccessMode am, TYPE defVal) :
    AttributeDefinitionBase(n, tn, fcc, am, defVal, false)
{
    // empty
}

//------------------------------------------------------------------------------
//  macro definitions
//------------------------------------------------------------------------------
#define DeclareAttrByte(NAME) extern const AttributeDefinition<Attr::ByteTypeClass, byte> NAME;
#define DefineAttrByte(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::ByteTypeClass, byte> NAME(#NAME, "byte", FOURCC,ACCESSMODE, byte(0));
#define DefineAttrByteWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::ByteTypeClass, byte> NAME(#NAME, "byte", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrShort(NAME) extern const AttributeDefinition<Attr::ShortTypeClass, short> NAME;
#define DefineAttrShort(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::ShortTypeClass, short> NAME(#NAME, "short", FOURCC,ACCESSMODE, short(0));
#define DefineAttrShortWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::ShortTypeClass, short> NAME(#NAME, "short", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrUShort(NAME) extern const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME;
#define DefineAttrUShort(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME(#NAME, "ushort", FOURCC,ACCESSMODE, ushort(0));
#define DefineAttrUShortWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME(#NAME, "ushort", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrInt(NAME) extern const AttributeDefinition<Attr::IntTypeClass, int> NAME;
#define DefineAttrInt(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME, "int", FOURCC,ACCESSMODE, int(0));
#define DefineAttrIntWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME, "int", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrUInt(NAME) extern const AttributeDefinition<Attr::UIntTypeClass, uint> NAME;
#define DefineAttrUInt(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UIntTypeClass, uint> NAME(#NAME, "uint", FOURCC,ACCESSMODE, uint(0));
#define DefineAttrUIntWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UIntTypeClass, uint> NAME(#NAME, "uint", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrInt64(NAME) extern const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME;
#define DefineAttrInt64(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME(#NAME, "int64", FOURCC,ACCESSMODE, int64_t(0));
#define DefineAttrInt64WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME(#NAME, "int64", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrUInt64(NAME) extern const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME;
#define DefineAttrUInt64(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME(#NAME, "uint64_t", FOURCC,ACCESSMODE, uint64_t(0));
#define DefineAttrUInt64WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME(#NAME, "uint64_t", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloat(NAME) extern const AttributeDefinition<Attr::FloatTypeClass, float> NAME;
#define DefineAttrFloat(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME, "float", FOURCC,ACCESSMODE, float(0.0f));
#define DefineAttrFloatWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME, "float", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrDouble(NAME) extern const AttributeDefinition<Attr::DoubleTypeClass, double> NAME;
#define DefineAttrDouble(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::DoubleTypeClass, double> NAME(#NAME, "double", FOURCC,ACCESSMODE, double(0.0));
#define DefineAttrDoubleWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::DoubleTypeClass, double> NAME(#NAME, "double", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrBool(NAME) extern const AttributeDefinition<Attr::BoolTypeClass, bool> NAME;
#define DefineAttrBool(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME, "bool", FOURCC,ACCESSMODE, bool(false));
#define DefineAttrBoolWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME, "bool", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloat2(NAME) extern const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME;
#define DefineAttrFloat2(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME(#NAME, "Math::float2", FOURCC,ACCESSMODE, Math::float2(0.0f, 0.0f));
#define DefineAttrFloat2WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME(#NAME, "Math::float2", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloat4(NAME) extern const AttributeDefinition<Attr::Float4TypeClass, Math::vec4> NAME;
#define DefineAttrFloat4(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float4TypeClass, Math::vec4> NAME(#NAME, "Math::vec4", FOURCC,ACCESSMODE, Math::vec4(0.0f, 0.0f, 0.0f, 0.0f));
#define DefineAttrFloat4WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float4TypeClass, Math::vec4> NAME(#NAME, "Math::vec4", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrQuaternion(NAME) extern const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME;
#define DefineAttrQuaternion(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME(#NAME, "Math::quaternion", FOURCC,ACCESSMODE, Math::quaternion(0.0f, 0.0f, 0.0f, 1.0f));
#define DefineAttrQuaternionWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME(#NAME, "Math::quaternion", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrString(NAME) extern const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME;
#define DefineAttrString(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME(#NAME, "Util::String", FOURCC,ACCESSMODE, Util::String());
#define DefineAttrStringWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME(#NAME, "Util::String", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrMatrix44(NAME) extern const AttributeDefinition<Attr::Matrix44TypeClass, Math::mat4> NAME;
#define DefineAttrMatrix44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Matrix44TypeClass, Math::mat4> NAME(#NAME, "Math::mat4", FOURCC,ACCESSMODE, Math::mat4());
#define DefineAttrMatrix44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Matrix44TypeClass, Math::mat4> NAME(#NAME, "Math::mat4", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrTransform44(NAME) extern const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME;
#define DefineAttrTransform44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME(#NAME, "Math::transform44", FOURCC,ACCESSMODE, Math::transform44());
#define DefineAttrTransform44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME(#NAME, "Math::transform44", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrBlob(NAME) extern const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME;
#define DefineAttrBlob(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME(#NAME, "Util::Blob", FOURCC,ACCESSMODE, Util::Blob());
#define DefineAttrBlobWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME(#NAME, "Util::Blob", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrGuid(NAME) extern const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME;
#define DefineAttrGuid(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME(#NAME, "Util::Guid", FOURCC,ACCESSMODE, Util::Guid());
#define DefineAttrGuidWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME(#NAME, "Util::Guid", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrVoidPtr(NAME) extern const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME;
#define DefineAttrVoidPtr(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME(#NAME, "void*", FOURCC,ACCESSMODE, nullptr);
#define DefineAttrVoidPtrWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME(#NAME, "void*", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrIntArray(NAME) extern const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME;
#define DefineAttrIntArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME(#NAME, "Util::Array<int>", FOURCC,ACCESSMODE, Util::Array<int>());
#define DefineAttrIntArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME(#NAME, "Util::Array<int>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloatArray(NAME) extern const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME;
#define DefineAttrFloatArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME(#NAME, "Util::Array<float>", FOURCC,ACCESSMODE, Util::Array<float>());
#define DefineAttrFloatArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME(#NAME, "Util::Array<float>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrBoolArray(NAME) extern const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME;
#define DefineAttrBoolArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME(#NAME, "Util::Array<bool>", FOURCC,ACCESSMODE, Util::Array<bool>());
#define DefineAttrBoolArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME(#NAME, "Util::Array<bool>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloat2Array(NAME) extern const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME;
#define DefineAttrFloat2Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME(#NAME, "Util::Array<Math::float2>", FOURCC,ACCESSMODE, Util::Array<Math::float2>());
#define DefineAttrFloat2ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME(#NAME, "Util::Array<Math::float2>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrFloat4Array(NAME) extern const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::vec4>> NAME;
#define DefineAttrFloat4Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::vec4>> NAME(#NAME, "Util::Array<Math::vec4>", FOURCC,ACCESSMODE, Util::Array<Math::vec4>());
#define DefineAttrFloat4ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::vec4>> NAME(#NAME, "Util::Array<Math::vec4>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrStringArray(NAME) extern const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME;
#define DefineAttrStringArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME(#NAME, "Util::Array<Util::String>", FOURCC,ACCESSMODE, Util::Array<Util::String>());
#define DefineAttrStringArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME(#NAME, "Util::Array<Util::String>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrMatrix44Array(NAME) extern const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::mat4>> NAME;
#define DefineAttrMatrix44Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::mat4>> NAME(#NAME, "Util::Array<Math::mat4>", FOURCC,ACCESSMODE, Util::Array<Math::mat4>());
#define DefineAttrMatrix44ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::mat4>> NAME(#NAME, "Util::Array<Math::mat4>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrBlobArray(NAME) extern const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME;
#define DefineAttrBlobArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME(#NAME, "Util::Array<Util::Blob>", FOURCC,ACCESSMODE, Util::Array<Util::Blob>());
#define DefineAttrBlobArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME(#NAME, "Util::Array<Util::Blob>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareAttrGuidArray(NAME) extern const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME;
#define DefineAttrGuidArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME(#NAME, "Util::Array<Util::Guid>", FOURCC,ACCESSMODE, Util::Array<Util::Guid>());
#define DefineAttrGuidArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME(#NAME, "Util::Array<Util::Guid>", FOURCC,ACCESSMODE,DEFVAL);

} // namespace Attr
//------------------------------------------------------------------------------
