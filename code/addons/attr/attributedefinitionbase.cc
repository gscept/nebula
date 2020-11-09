//------------------------------------------------------------------------------
//  definition.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "attributedefinitionbase.h"

namespace Attr
{
AttrExitHandler AttributeDefinitionBase::attrExitHandler;

Util::HashTable<Util::String, const AttributeDefinitionBase*>* AttributeDefinitionBase::NameRegistry = 0;
Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>* AttributeDefinitionBase::FourCCRegistry = 0;
Util::Array<const AttributeDefinitionBase*>* AttributeDefinitionBase::DynamicAttributes = 0;

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase() :
    accessMode(ReadOnly),
    valueType(ValueType::VoidType),
    isDynamic(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    valueType(ValueType::VoidType),
    isDynamic(isDynamic)
{
    this->Register();
}


//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const byte& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::ByteType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const short& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::ShortType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const ushort& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UShortType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const int& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::IntType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const uint& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UIntType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const int64_t& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Int64Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const uint64_t& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::UInt64Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const float& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::FloatType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const double& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::DoubleType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const bool& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BoolType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::vec2& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Vec2Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::vec4& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Vec4Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::quat& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::QuaternionType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::String& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::StringType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::mat4& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Mat4Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Math::transform44& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Transform44Type),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Blob& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BlobType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Guid& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::GuidType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, void* defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::VoidPtrType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<int>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::IntArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<float>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::FloatArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<bool>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BoolArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::vec2>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Vec2ArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::vec4>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Vec4ArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::String>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::StringArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::mat4>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::Mat4ArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Blob>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::BlobArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Guid>& defVal, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(ValueType::GuidArrayType),
    isDynamic(isDynamic)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& name, const Util::String& typeName, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Variant& defVal, ValueType type, bool isDynamic) :
    name(name),
    typeName(typeName),
    fourCC(fourCC),
    accessMode(accessMode),
    defaultValue(defVal),
    valueType(type),
    isDynamic(isDynamic)
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
        NameRegistry = new Util::HashTable<Util::String, const AttributeDefinitionBase*>();
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
*/
void
AttributeDefinitionBase::InitializeRegistry()
{
    CheckCreateFourCCRegistry();
    CheckCreateNameRegistry();
    CheckCreateDynamicAttributesArray();
}

//------------------------------------------------------------------------------
/**
    Check if dynamic attrs array exists and create if necessary.
*/
void
AttributeDefinitionBase::CheckCreateDynamicAttributesArray()
{
    if (0 == DynamicAttributes)
    {
        DynamicAttributes = new Util::Array<const AttributeDefinitionBase*>;
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

    if (this->IsDynamic())
    {
        CheckCreateDynamicAttributesArray();
        DynamicAttributes->Append(this);
    }
    else
    {
        // only non-dynamic attributes has a valid fourcc code
        n_assert(this->fourCC.IsValid());

#if NEBULA_DEBUG
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
}

//------------------------------------------------------------------------------
/**
    Cleanup the name registry!
*/
void
AttributeDefinitionBase::Destroy()
{
    // first clear the dynamic attributes
    if (0 != DynamicAttributes)
    {
        ClearDynamicAttributes();
    }

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

//------------------------------------------------------------------------------
/**
    This method registers a new dynamic attribut.
*/
void
AttributeDefinitionBase::RegisterDynamicAttribute(const Util::String &name, const Util::String& typeName, const Util::FourCC& fourCC, Attr::ValueType valueType, Attr::AccessMode accessMode)
{
    if (FindByName(name))
    {
        n_error("RegisterDynamicAttribute: attribute '%s' already exists!\n", name.AsCharPtr());
    }
    AttributeDefinitionBase* dynAttr;
    switch (valueType)
    {
    case VoidType:
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, true));
        break;

    case IntType:
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, 0, true));
        break;

    case FloatType:
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, 0.0f, true));
        break;

    case BoolType:
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, false, true));
        break;

    case Vec4Type:
    {
        const static Math::vec4 nullVec {0.0f, 0.0f, 0.0f, 0.0f};
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, nullVec, true));
        break;
    }
    break;

    case Mat4Type:
    {
        const static Math::mat4 identity;
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, identity, true));
        break;
    }
    break;

    case BlobType:
    {
        const static Util::Blob blob;
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, blob, true));
        break;
    }
    break;

    case GuidType:
    {
        const static Util::Guid guid;
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, guid, true));
        break;
    }
    break;

    case StringType:
    {
        const static Util::String str;
        dynAttr = n_new(AttributeDefinitionBase(name, typeName, fourCC, accessMode, str, true));
    }
    break;

    default:
        n_error("RegisterDynamicAttribute(): ValueType not implemented or invalid!");
        break;
    }
}

//------------------------------------------------------------------------------
/**
    This clears all dynamic attributes.
*/
void
AttributeDefinitionBase::ClearDynamicAttributes()
{
    if (0 != DynamicAttributes)
    {
        IndexT i;
        SizeT num = DynamicAttributes->Size();
        for (i = 0; i < num; i++)
        {
            n_assert(0 != (*DynamicAttributes)[i]);

            // need to delete the attribute from the hash table
            NameRegistry->Erase((*DynamicAttributes)[i]->GetName());

            // delete the attribute object itself
            delete (*DynamicAttributes)[i];
            (*DynamicAttributes)[i] = 0;
        }
        delete DynamicAttributes;
        DynamicAttributes = 0;
    }
}

} // namespace Attr
