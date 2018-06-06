#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeDefinitionBase
    
    Implements a universal attribute definition, consisting of an attribute
    name, attribute fourcc code, value type and access type. 
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/fourcc.h"
#include "util/string.h"
#include "util/hashtable.h"
#include "util/dictionary.h"
#include "game/attr/valuetype.h"
#include "game/attr/accessmode.h"
#include "game/attr/attrexithandler.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttributeDefinitionBase
{
public:
    /// constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, bool isDynamic);
    /// bool constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, bool defVal, bool isDynamic);
    /// int constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, int defVal, bool isDynamic);
    /// float constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, float defVal, bool isDynamic);
    /// string constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::String& defVal, bool isDynamic);
    /// float4 constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::float4& defVal, bool isDynamic);
    /// matrix44 constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::matrix44& defVal, bool isDynamic);
	/// transform44 constructor
	explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::transform44& defVal, bool isDynamic);
    /// blob constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Blob& defVal, bool isDynamic);
    /// guid constructor
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Guid& defVal, bool isDynamic);    
    /// destructor
    ~AttributeDefinitionBase();
    /// static destruction method - call to cleanup the registry hashtable
    static void Destroy();

    /// get attribute name
    const Util::String& GetName() const;
    /// get fourcc code
    const Util::FourCC& GetFourCC() const;
    /// return true if this is a dynamic attribute
    bool IsDynamic() const;
    /// get default value
    const Util::Variant& GetDefaultValue() const;
    /// get access type
    AccessMode GetAccessMode() const;
    /// get value type
    ValueType GetValueType() const;
    /// find by name
    static const AttributeDefinitionBase* FindByName(const Util::String& n);
    /// find by FourCC
    static const AttributeDefinitionBase* FindByFourCC(const Util::FourCC& fcc);
    /// register a dynamic attribute
    static void RegisterDynamicAttribute(const Util::String& name, const Util::FourCC& fourCC, ValueType valueType, AccessMode accessMode);
    /// clear all dynamic attributes
    static void ClearDynamicAttributes();

protected:
    /// register an attribute definition
    void Register();

private:
    /// default constructor is private
    AttributeDefinitionBase();
    /// check if name registry exists and create on demand
    static void CheckCreateNameRegistry();
    /// check if fourcc registry exists and create on demand
    static void CheckCreateFourCCRegistry();
    /// check if dynamic attribute array exists and create on demand
    static void CheckCreateDynamicAttributesArray();

    bool isDynamic;
    Util::String name;
    Util::FourCC fourCC;
    AccessMode accessMode;
    Util::Variant defaultValue;

    friend class AttrId;
    static AttrExitHandler attrExitHandler;
    static Util::HashTable<Util::String, const AttributeDefinitionBase*>* NameRegistry;
    static Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>* FourCCRegistry;
    static Util::Array<const AttributeDefinitionBase*>* DynamicAttributes;
};

//------------------------------------------------------------------------------
/**
*/
inline
const AttributeDefinitionBase*
AttributeDefinitionBase::FindByName(const Util::String& n)
{
    n_assert(0 != NameRegistry);
    if (!NameRegistry->Contains(n))
    {
        return 0;
    }
    else
    {
        return (*NameRegistry)[n];
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const AttributeDefinitionBase*
AttributeDefinitionBase::FindByFourCC(const Util::FourCC& fcc)
{
    n_assert(0 != FourCCRegistry);
    if (!FourCCRegistry->Contains(fcc))
    {
        return 0;
    }
    else
    {
        return (*FourCCRegistry)[fcc];
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String& 
AttributeDefinitionBase::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::FourCC&
AttributeDefinitionBase::GetFourCC() const
{
    return this->fourCC;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Variant& 
AttributeDefinitionBase::GetDefaultValue() const
{
    return this->defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
inline
ValueType
AttributeDefinitionBase::GetValueType() const
{
    return (ValueType) this->defaultValue.GetType();
}

//------------------------------------------------------------------------------
/**
*/
inline
AccessMode
AttributeDefinitionBase::GetAccessMode() const
{
    return this->accessMode;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
AttributeDefinitionBase::IsDynamic() const
{
    return this->isDynamic;
}

} // namespace Attr
//------------------------------------------------------------------------------
