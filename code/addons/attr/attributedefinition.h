#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeDefinition

    This extends the typeless AttributeDefinitionBase class by a typed
    template class, which adds compiletime-type-safety to attribute definitions.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
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
    using AttrDeclType = typename TYPE;
};

//------------------------------------------------------------------------------
/**
*/
template<class VALUETYPE, class TYPE>
AttributeDefinition<VALUETYPE, TYPE>::AttributeDefinition(const Util::String& n, const Util::String& tn, const Util::FourCC& fcc, AccessMode am, TYPE defVal) :
    AttributeDefinitionBase(n, tn, fcc, am, defVal, false)
{
    // empty
}

//------------------------------------------------------------------------------
//  macro definitions
//------------------------------------------------------------------------------
#define DeclareByte(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::ByteTypeClass, byte> NAME;
#define DefineByte(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::ByteTypeClass, byte> NAME(#NAME, "byte", FOURCC,ACCESSMODE, byte(0));
#define DefineByteWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::ByteTypeClass, byte> NAME(#NAME, "byte", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareShort(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::ShortTypeClass, short> NAME;
#define DefineShort(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::ShortTypeClass, short> NAME(#NAME, "short", FOURCC,ACCESSMODE, short(0));
#define DefineShortWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::ShortTypeClass, short> NAME(#NAME, "short", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareUShort(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME;
#define DefineUShort(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME(#NAME, "ushort", FOURCC,ACCESSMODE, ushort(0));
#define DefineUShortWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UShortTypeClass, ushort> NAME(#NAME, "ushort", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareInt(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::IntTypeClass, int> NAME;
#define DefineInt(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME, "int", FOURCC,ACCESSMODE, int(0));
#define DefineIntWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME, "int", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareUInt(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::UIntTypeClass, uint> NAME;
#define DefineUInt(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UIntTypeClass, uint> NAME(#NAME, "uint", FOURCC,ACCESSMODE, uint(0));
#define DefineUIntWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UIntTypeClass, uint> NAME(#NAME, "uint", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareInt64(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME;
#define DefineInt64(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME(#NAME, "int64", FOURCC,ACCESSMODE, int64_t(0));
#define DefineInt64WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Int64TypeClass, int64_t> NAME(#NAME, "int64", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareUInt64(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME;
#define DefineUInt64(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME(#NAME, "uint64", FOURCC,ACCESSMODE, uint64_t(0));
#define DefineUInt64WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::UInt64TypeClass, uint64_t> NAME(#NAME, "uint64", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::FloatTypeClass, float> NAME;
#define DefineFloat(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME, "float", FOURCC,ACCESSMODE, float(0.0f));
#define DefineFloatWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME, "float", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareDouble(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::DoubleTypeClass, double> NAME;
#define DefineDouble(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::DoubleTypeClass, double> NAME(#NAME, "double", FOURCC,ACCESSMODE, double(0.0));
#define DefineDoubleWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::DoubleTypeClass, double> NAME(#NAME, "double", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareBool(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BoolTypeClass, bool> NAME;
#define DefineBool(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME, "bool", FOURCC,ACCESSMODE, bool(false));
#define DefineBoolWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME, "bool", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat2(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME;
#define DefineFloat2(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME(#NAME, "Math::float2", FOURCC,ACCESSMODE, Math::float2(0.0f, 0.0f));
#define DefineFloat2WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float2TypeClass, Math::float2> NAME(#NAME, "Math::float2", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat4(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Float4TypeClass, Math::float4> NAME;
#define DefineFloat4(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float4TypeClass, Math::float4> NAME(#NAME, "Math::float4", FOURCC,ACCESSMODE, Math::float4(0.0f, 0.0f, 0.0f, 0.0f));
#define DefineFloat4WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float4TypeClass, Math::float4> NAME(#NAME, "Math::float4", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareQuaternion(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME;
#define DefineQuaternion(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME(#NAME, "Math::quaternion", FOURCC,ACCESSMODE, Math::quaternion(0.0f, 0.0f, 0.0f, 1.0f));
#define DefineQuaternionWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::QuaternionTypeClass, Math::quaternion> NAME(#NAME, "Math::quaternion", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareString(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME;
#define DefineString(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME(#NAME, "Util::String", FOURCC,ACCESSMODE, Util::String());
#define DefineStringWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::StringTypeClass, Util::String> NAME(#NAME, "Util::String", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareMatrix44(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Matrix44TypeClass, Math::matrix44> NAME;
#define DefineMatrix44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Matrix44TypeClass, Math::matrix44> NAME(#NAME, "Math::matrix44", FOURCC,ACCESSMODE, Math::matrix44::identity());
#define DefineMatrix44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Matrix44TypeClass, Math::matrix44> NAME(#NAME, "Math::matrix44", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareTransform44(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME;
#define DefineTransform44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME(#NAME, "Math::transform44", FOURCC,ACCESSMODE, Math::transform44());
#define DefineTransform44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Transform44TypeClass, Math::transform44> NAME(#NAME, "Math::transform44", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareBlob(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME;
#define DefineBlob(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME(#NAME, "Util::Blob", FOURCC,ACCESSMODE, Util::Blob());
#define DefineBlobWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BlobTypeClass, Util::Blob> NAME(#NAME, "Util::Blob", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareGuid(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME;
#define DefineGuid(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME(#NAME, "Util::Guid", FOURCC,ACCESSMODE, Util::Guid());
#define DefineGuidWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::GuidTypeClass, Util::Guid> NAME(#NAME, "Util::Guid", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareVoidPtr(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME;
#define DefineVoidPtr(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME(#NAME, "void*", FOURCC,ACCESSMODE, nullptr);
#define DefineVoidPtrWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::VoidPtrTypeClass, void*> NAME(#NAME, "void*", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareIntArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME;
#define DefineIntArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME(#NAME, "Util::Array<int>", FOURCC,ACCESSMODE, Util::Array<int>());
#define DefineIntArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::IntArrayTypeClass, Util::Array<int>> NAME(#NAME, "Util::Array<int>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloatArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME;
#define DefineFloatArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME(#NAME, "Util::Array<float>", FOURCC,ACCESSMODE, Util::Array<float>());
#define DefineFloatArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::FloatArrayTypeClass, Util::Array<float>> NAME(#NAME, "Util::Array<float>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareBoolArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME;
#define DefineBoolArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME(#NAME, "Util::Array<bool>", FOURCC,ACCESSMODE, Util::Array<bool>());
#define DefineBoolArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BoolArrayTypeClass, Util::Array<bool>> NAME(#NAME, "Util::Array<bool>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat2Array(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME;
#define DefineFloat2Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME(#NAME, "Util::Array<Math::float2>", FOURCC,ACCESSMODE, Util::Array<Math::float2>());
#define DefineFloat2ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float2ArrayTypeClass, Util::Array<Math::float2>> NAME(#NAME, "Util::Array<Math::float2>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat4Array(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::float4>> NAME;
#define DefineFloat4Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::float4>> NAME(#NAME, "Util::Array<Math::float4>", FOURCC,ACCESSMODE, Util::Array<Math::float4>());
#define DefineFloat4ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float4ArrayTypeClass, Util::Array<Math::float4>> NAME(#NAME, "Util::Array<Math::float4>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareStringArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME;
#define DefineStringArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME(#NAME, "Util::Array<Util::String>", FOURCC,ACCESSMODE, Util::Array<Util::String>());
#define DefineStringArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::StringArrayTypeClass, Util::Array<Util::String>> NAME(#NAME, "Util::Array<Util::String>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareMatrix44Array(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::matrix44>> NAME;
#define DefineMatrix44Array(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::matrix44>> NAME(#NAME, "Util::Array<Math::matrix44>", FOURCC,ACCESSMODE, Util::Array<Math::matrix44>());
#define DefineMatrix44ArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Matrix44ArrayTypeClass, Util::Array<Math::matrix44>> NAME(#NAME, "Util::Array<Math::matrix44>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareBlobArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME;
#define DefineBlobArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME(#NAME, "Util::Array<Util::Blob>", FOURCC,ACCESSMODE, Util::Array<Util::Blob>());
#define DefineBlobArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BlobArrayTypeClass, Util::Array<Util::Blob>> NAME(#NAME, "Util::Array<Util::Blob>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareGuidArray(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME;
#define DefineGuidArray(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME(#NAME, "Util::Array<Util::Guid>", FOURCC,ACCESSMODE, Util::Array<Util::Guid>());
#define DefineGuidArrayWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::GuidArrayTypeClass, Util::Array<Util::Guid>> NAME(#NAME, "Util::Array<Util::Guid>", FOURCC,ACCESSMODE,DEFVAL);

#define DeclareEntity(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::EntityTypeClass, Game::Entity> NAME;
#define DefineEntity(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::EntityTypeClass, Game::Entity> NAME(#NAME, "Game::Entity", FOURCC,ACCESSMODE, Game::Entity(-1));
#define DefineEntityWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::EntityTypeClass, Game::Entity> NAME(#NAME, "Game::Entity", FOURCC,ACCESSMODE,DEFVAL);

} // namespace Attr
//------------------------------------------------------------------------------
