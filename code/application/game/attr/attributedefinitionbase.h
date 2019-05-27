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
#include "game/entity.h"
#include "util/string.h"
#include "util/blob.h"
#include "math/quaternion.h"
#include "math/transform44.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttributeDefinitionBase
{
public:
	explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode);
	/// Constructor with default byte value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const byte& defVal);
	/// Constructor with default short value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const short& defVal);
	/// Constructor with default ushort value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const ushort& defVal);
	/// Constructor with default int value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const int& defVal);
	/// Constructor with default uint value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const uint& defVal);
	/// Constructor with default int64 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const int64_t& defVal);
	/// Constructor with default uint64 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const uint64_t& defVal);
	/// Constructor with default float value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const float& defVal);
	/// Constructor with default double value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const double& defVal);
	/// Constructor with default bool value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const bool& defVal);
	/// Constructor with default float2 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::float2& defVal);
	/// Constructor with default float4 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::float4& defVal);
	/// Constructor with default quaternion value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::quaternion& defVal);
	/// Constructor with default string value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::String& defVal);
	/// Constructor with default matrix44 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::matrix44& defVal);
	/// Constructor with default transform44 value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Math::transform44& defVal);
	/// Constructor with default blob value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Blob& defVal);
	/// Constructor with default guid value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Guid& defVal);
	/// Constructor with default void pointer value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, void* defVal);
	/// Constructor with default int array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<int>& defVal);
	/// Constructor with default float array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<float>& defVal);
	/// Constructor with default bool array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<bool>& defVal);
	/// Constructor with default float2 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::float2>& defVal);
	/// Constructor with default float4 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::float4>& defVal);
	/// Constructor with default string array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::String>& defVal);
	/// Constructor with default matrix44 array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Math::matrix44>& defVal);
	/// Constructor with default blob array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Blob>& defVal);
	/// Constructor with default guid array value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Array<Util::Guid>& defVal);
	/// Constructor with default entity value
    explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Game::Entity& defVal);
	/// Constructor with default variant value
	explicit AttributeDefinitionBase(const Util::String& name, const Util::FourCC& fourCC, AccessMode accessMode, const Util::Variant& defVal, ValueType type);

    /// destructor
    ~AttributeDefinitionBase();
    /// static destruction method - call to cleanup the registry hashtable
    static void Destroy();

    /// get attribute name
    const Util::String& GetName() const;
    /// get fourcc code
    const Util::FourCC& GetFourCC() const;
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

	/// setup registry if it doesn't exist
	static void InitializeRegistry();

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

    Util::String name;
    Util::FourCC fourCC;
    AccessMode accessMode;
    Util::Variant defaultValue;
	const Attr::ValueType valueType;

    friend class AttrId;
    static AttrExitHandler attrExitHandler;
    static Util::HashTable<Util::String, const AttributeDefinitionBase*>* NameRegistry;
    static Util::Dictionary<Util::FourCC, const AttributeDefinitionBase*>* FourCCRegistry;
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
        return nullptr;
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
        return nullptr;
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

} // namespace Attr
//------------------------------------------------------------------------------
