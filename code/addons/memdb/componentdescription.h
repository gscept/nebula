#pragma once
//------------------------------------------------------------------------------
/**
    ComponentDescription

    Describes a component's type and default value.
    The components type can be omitted by registering it with a size of zero bytes.
        This essentially means the component is only used when querying the database
    This information is used to allocate the table columns.

    @note   This is mostly just used internally by the database and typeregistry,
            so you most likely won't need to care about this class in normal use-cases.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/fourcc.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "table.h"
#include "componentid.h"

namespace MemDb
{

class ComponentDescription
{
public:
    /// construct from template type, with default value.
    template<typename T>
    explicit ComponentDescription(Util::StringAtom name, T const& defaultValue, uint32_t flags);
    /// construct from type size in bytes, with default value from void*.
    explicit ComponentDescription(Util::StringAtom name, SizeT typeSizeBytes, void const* defaultValue, uint32_t flags);
    /// default constructor
    ComponentDescription() = default;
    /// move constructor
    ComponentDescription(ComponentDescription&& desc) noexcept;
    /// desctructor
    ~ComponentDescription();
    
    /// move assignment operator
    void operator=(ComponentDescription&& rhs) noexcept;
    /// assignment operator
    void operator=(ComponentDescription& rhs);

    /// name of component
    Util::StringAtom name;
    /// size of type in bytes
    SizeT typeSize = 0;
    /// default value
    void* defVal = nullptr;
    /// externally managed flags
    uint32_t externalFlags;
};

//------------------------------------------------------------------------------
/**
*/
template<typename T> inline
ComponentDescription::ComponentDescription(Util::StringAtom name, T const& defaultValue, uint32_t flags) :
    name(name),
    typeSize(sizeof(T)),
    externalFlags(flags)
{
    this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
    Memory::Copy(&defaultValue, this->defVal, sizeof(T));
}

//------------------------------------------------------------------------------
/**
*/
inline
ComponentDescription::ComponentDescription(Util::StringAtom name, SizeT typeSizeBytes, void const* defaultValue, uint32_t flags) :
    name(name),
    typeSize(typeSizeBytes),
    externalFlags(flags)
{
    if (typeSizeBytes > 0)
    {
        this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, typeSizeBytes);
        if (defaultValue != nullptr)
        {
            Memory::Copy(defaultValue, this->defVal, typeSizeBytes);
        }
        else
        {
            // just set everything to zero
            Memory::Clear(this->defVal, typeSizeBytes);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
ComponentDescription::ComponentDescription(ComponentDescription&& desc) noexcept :
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
ComponentDescription::~ComponentDescription()
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
ComponentDescription::operator=(ComponentDescription&& rhs) noexcept
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
ComponentDescription::operator=(ComponentDescription& rhs)
{
    this->typeSize = rhs.typeSize;
    this->defVal = rhs.defVal;
    this->name = rhs.name;

    rhs.typeSize = 0;
    rhs.defVal = nullptr;
    rhs.name = nullptr;
}

} // namespace MemDb
