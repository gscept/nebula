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
Util::Array<const AttributeDefinitionBase*>* AttributeDefinitionBase::DynamicAttributes = 0;
   
//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase() :
    isDynamic(false),
    accessMode(ReadOnly)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, bool defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, int defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, float defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Util::String& defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Math::float4& defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Math::matrix44& defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Math::transform44& defVal, bool dyn) :
	isDynamic(dyn),
	name(n),
	fourCC(fcc),
	accessMode(m),
	defaultValue(defVal)
{
	this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Util::Blob& defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const Util::Guid& defVal, bool dyn) :
    isDynamic(dyn),
    name(n),
    fourCC(fcc),
    accessMode(m),
    defaultValue(defVal)
{
    this->Register();
}

//------------------------------------------------------------------------------
/**
*/
AttributeDefinitionBase::AttributeDefinitionBase(const Util::String& n, const Util::FourCC& fcc, AccessMode m, const uint32_t& defVal, bool dyn) :
	isDynamic(dyn),
	name(n),
	fourCC(fcc),
	accessMode(m),
	defaultValue(defVal)
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
AttributeDefinitionBase::RegisterDynamicAttribute(const Util::String &name, const Util::FourCC& fourCC, Attr::ValueType valueType, Attr::AccessMode accessMode)
{
    if (FindByName(name))
    {
        n_error("RegisterDynamicAttribute: attribute '%s' already exists!\n", name.AsCharPtr());
    }
    AttributeDefinitionBase* dynAttr;
    switch (valueType)
    {
        case VoidType:
            dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, true));
            break;

        case IntType:
            dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, 0, true));
            break;

        case FloatType:
            dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, 0.0f, true));
            break;

        case BoolType:
            dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, false, true));
            break;

        case Float4Type:
            {
                const static Math::float4 nullVec;
                dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, nullVec, true));
                break;
            }
            break;

        case Matrix44Type:
            {
                const static Math::matrix44 identity;
                dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, identity, true));
                break;
            }
            break;

		case Transform44Type:
			{
				const static Math::transform44 identity;
				dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, identity, true));
				break;
			}
			break;

        case BlobType:
            {
                const static Util::Blob blob;
                dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, blob, true));
                break;
            }
            break;

        case GuidType:
            {
                const static Util::Guid guid;
                dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, guid, true));
                break;
            }
            break;

        case StringType:
            {
                const static Util::String str;
                dynAttr = n_new(AttributeDefinitionBase(name, fourCC, accessMode, str, true));
            }
            break;

        default:
            n_error("RegisterDynamicAttribute(): invalid value type!");
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