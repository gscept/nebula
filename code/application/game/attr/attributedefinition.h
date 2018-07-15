#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeDefinition
  
    This extends the typeless AttributeDefinitionBase class by a typed 
    template class, which adds compiletime-type-safety to attribute definitions.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "game/attr/attributedefinitionbase.h"

//------------------------------------------------------------------------------
namespace Attr
{
template<class VALUETYPE, class TYPE> 
class AttributeDefinition : public AttributeDefinitionBase
{
public:
    /// constructor
    AttributeDefinition(const Util::String& n, const Util::FourCC& fourCC, AccessMode am, TYPE defVal);
};

//------------------------------------------------------------------------------
/**
*/
template<class VALUETYPE, class TYPE>
AttributeDefinition<VALUETYPE,TYPE>::AttributeDefinition(const Util::String& n, const Util::FourCC& fcc, AccessMode am, TYPE defVal) :
    AttributeDefinitionBase(n, fcc, am, defVal, false)
{
    // empty
}

//------------------------------------------------------------------------------
//  macro definitions
//------------------------------------------------------------------------------
#define DeclareBool(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BoolTypeClass, bool> NAME;
#define DefineBool(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME,FOURCC,ACCESSMODE, false);
#define DefineBoolWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BoolTypeClass, bool> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareInt(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::IntTypeClass, int> NAME;
#define DefineInt(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME,FOURCC,ACCESSMODE, 0);
#define DefineIntWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::IntTypeClass, int> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::FloatTypeClass, float> NAME;
#define DefineFloat(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME,FOURCC,ACCESSMODE, 0.0f);
#define DefineFloatWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::FloatTypeClass, float> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareString(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::StringTypeClass, const Util::String&> NAME;
#define DefineString(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::StringTypeClass, const Util::String&> NAME(#NAME,FOURCC,ACCESSMODE, Util::String(""));
#define DefineStringWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::StringTypeClass, const Util::String&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareFloat4(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Float4TypeClass, const Math::float4&> NAME;
#define DefineFloat4(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Float4TypeClass, const Math::float4&> NAME(#NAME,FOURCC,ACCESSMODE, Math::float4(0,0,0,0));
#define DefineFloat4WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Float4TypeClass, const Math::float4&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareMatrix44(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Matrix44TypeClass, const Math::matrix44&> NAME;
#define DefineMatrix44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Matrix44TypeClass, const Math::matrix44&> NAME(#NAME,FOURCC,ACCESSMODE, Math::matrix44());
#define DefineMatrix44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Matrix44TypeClass, const Math::matrix44&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareTransform44(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::Transform44TypeClass, const Math::transform44&> NAME;
#define DefineTransform44(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::Transform44TypeClass, const Math::transform44&> NAME(#NAME,FOURCC,ACCESSMODE, Math::transform44());
#define DefineTransform44WithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::Transform44TypeClass, const Math::transform44&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareBlob(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::BlobTypeClass, const Util::Blob&> NAME;
#define DefineBlob(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::BlobTypeClass, const Util::Blob&> NAME(#NAME,FOURCC,ACCESSMODE, Util::Blob());
#define DefineBlobWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::BlobTypeClass, const Util::Blob&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareGuid(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::GuidTypeClass, const Util::Guid&> NAME;
#define DefineGuid(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::GuidTypeClass, const Util::Guid&> NAME(#NAME,FOURCC,ACCESSMODE, Util::Guid());
#define DefineGuidWithDefault(NAME,FOURCC,ACCESSMODE,DEFVAL) const AttributeDefinition<Attr::GuidTypeClass, const Util::Guid&> NAME(#NAME,FOURCC,ACCESSMODE,DEFVAL);

#define DeclareEntity(NAME,FOURCC,ACCESSMODE) extern const AttributeDefinition<Attr::EntityTypeClass, const uint32_t&> NAME;
#define DefineEntity(NAME,FOURCC,ACCESSMODE) const AttributeDefinition<Attr::EntityTypeClass, const uint32_t&> NAME(#NAME,FOURCC,ACCESSMODE, uint32_t(0));

} // namespace Attr
//------------------------------------------------------------------------------
