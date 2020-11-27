#pragma once
//------------------------------------------------------------------------------
/**
    PropertyDescription

    Describes a property's type and default value.
    This information is used to allocate the table columns.

    @note   This is mostly just used internally by the database and typeregistry,
            so you most likely won't need to care about this class in normal use-cases.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/fourcc.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "table.h"
#include "propertyid.h"

namespace MemDb
{

class PropertyDescription
{
public:
    /// construct from template type, with default value.
    template<typename T>
    explicit PropertyDescription(Util::StringAtom name, T const& defaultValue);
    /// default constructor
    PropertyDescription() = default;
    /// move constructor
    PropertyDescription(PropertyDescription&& desc) noexcept;
    /// desctructor
    ~PropertyDescription();
    
    /// move assignment operator
    void operator=(PropertyDescription&& rhs) noexcept;
    /// assignment operator
    void operator=(PropertyDescription& rhs);

    /// name of property
    Util::StringAtom name;
    /// size of type in bytes
    SizeT typeSize = 0;
    /// default value
    void* defVal = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
template<typename T> inline
PropertyDescription::PropertyDescription(Util::StringAtom name, T const& defaultValue)
{
    this->typeSize = sizeof(T);
    this->name = name;
    this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
    Memory::Copy(&defaultValue, this->defVal, sizeof(T));
}

//------------------------------------------------------------------------------
/**
*/
inline
PropertyDescription::PropertyDescription(PropertyDescription&& desc) noexcept :
    defVal(desc.defVal), typeSize(desc.typeSize), name(desc.name)
{
    desc.defVal = nullptr;
    desc.name = nullptr;
    desc.typeSize = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
PropertyDescription::~PropertyDescription()
{
    if (this->defVal != nullptr)
    {
        Memory::Free(Memory::HeapType::ObjectHeap, this->defVal);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
PropertyDescription::operator=(PropertyDescription&& rhs) noexcept
{
    this->typeSize = rhs.typeSize;
    this->defVal = rhs.defVal;
    this->name = rhs.name;

    rhs.typeSize = 0;
    rhs.defVal = nullptr;
    rhs.name = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PropertyDescription::operator=(PropertyDescription& rhs)
{
    this->typeSize = rhs.typeSize;
    this->defVal = rhs.defVal;
    this->name = rhs.name;

    rhs.typeSize = 0;
    rhs.defVal = nullptr;
    rhs.name = nullptr;
}

} // namespace MemDb
