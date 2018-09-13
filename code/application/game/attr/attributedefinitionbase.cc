//------------------------------------------------------------------------------
//  definition.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/attr/attributedefinitionbase.h"

namespace Attr
{
AttrExitHandler AttributeDefinitionBase::attrExitHandler;

Util::HashTable<Util::String, const AttributeDefinitionBase*>* AttributeDefinitionBase::NameRegistry = 0;
Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>* AttributeDefinitionBase::FourCCRegistry = 0;
   
//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase() :
	accessMode(ReadOnly),
	valueType(ValueType::VoidType)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode) :
	name(name),
	fourCC(fourCC),
	accessMode(accessMode),
	valueType(ValueType::VoidType)
{
	this->Register();
}


//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const byte& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::ByteType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const short& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::ShortType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const ushort& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UShortType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const int& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::IntType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const uint& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UIntType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const int64_t& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Int64Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const uint64_t& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UInt64Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const float& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::FloatType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const double& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::DoubleType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const bool& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BoolType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::float2& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Float2Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::float4& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Float4Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::quaternion& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::QuaternionType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::String& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::StringType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::matrix44& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Matrix44Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::transform44& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Transform44Type)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Blob& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BlobType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Guid& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::GuidType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, void* defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::VoidPtrType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<int>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::IntArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<float>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::FloatArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<bool>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BoolArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::float2>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Float2ArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::float4>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Float4ArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::String>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::StringArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::matrix44>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Matrix44ArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Blob>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BlobArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Guid>& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::GuidArrayType)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Game::Entity& defVal) :
    name(name),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal.id),
    valueType(ValueType::EntityType)
{
    this->Register();
}


//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::~AttributeDefinitionBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Check if name registry exists and create if necessary.
*/
void
AttributeDefinitionBase::CheckCreateNameRegistry()
{
    if (0 == NameRegistry)
    {
        NameRegistry = new Util::HashTable<Util::String, const AttributeDefinitionBase*>(4096);
    }
}
//------------------------------------------------------------------------------
/**
    Check if fourcc registry exists and create if necessary.
*/
void
AttributeDefinitionBase::CheckCreateFourCCRegistry()
{
    if (0 == FourCCRegistry)
    {
        FourCCRegistry = new Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>;
    }
}

//------------------------------------------------------------------------------
/**
    Register this static attribute definition in the name and fourcc registries.
    Since the order of initialization is not defined for static 
    objects we need to use pointers and creation-on-demand for the registry
    objects.
*/
void
AttributeDefinitionBase::Register()
{
    n_assert(this->name.IsValid());
    this->CheckCreateNameRegistry();
    this->CheckCreateFourCCRegistry();
	if (NameRegistry->Contains(this->name))
    {
        n_error("Attribute '%s' already registered!", this->name.AsCharPtr());
    }
    NameRegistry->Add(this->name, this);
    
    // only non-dynamic attributes has a valid fourcc code
    n_assert(this->fourCC.IsValid());

#if NEBULA3_DEBUG
    // check if attribute fourcc already exists
    if (FourCCRegistry->Contains(this->fourCC))
    {
        Util::String errorMsg;
        errorMsg.Format("Attribute fourcc '%s' (name: %s) has already been registered!", 
            this->fourCC.AsString().AsCharPtr(),
            this->name.AsCharPtr());
        Core::SysFunc::Error(errorMsg.AsCharPtr());
        return;
    }
#endif
    FourCCRegistry->Add(this->fourCC, this);
}

//------------------------------------------------------------------------------
/**
    Cleanup the name registry!
*/
void
AttributeDefinitionBase::Destroy()
{
    if (NameRegistry != 0)
    {
        // cleanup name registry
        delete NameRegistry;
        NameRegistry = 0;
    }
    if (FourCCRegistry != 0)
    {
        // cleanup fourcc registry
        delete FourCCRegistry;
        FourCCRegistry = 0;
    }
}

} // namespace Attr