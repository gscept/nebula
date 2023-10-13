#pragma once
//------------------------------------------------------------------------------
/**
    @file filter.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "category.h"
#include "dataset.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
enum AccessMode
{
    READ,
    WRITE
};

/// Opaque filter identifier.
typedef uint32_t Filter;

typedef MemDb::TableSignature InclusiveTableMask;
typedef MemDb::TableSignature ExclusiveTableMask;

void DestroyFilter(Filter filter);

/// retrieve the inclusive table mask
InclusiveTableMask const& GetInclusiveTableMask(Filter);
/// retrieve the exclusive table mask
ExclusiveTableMask const& GetExclusiveTableMask(Filter);
/// retrieve the inclusive component array
Util::FixedArray<ComponentId> const& ComponentsInFilter(Filter);

class FilterBuilder
{
public:
    struct ComponentRequest
    {
        AccessMode access;
        ComponentId id;
    };

public:
    FilterBuilder();
    
    template<typename ... TYPES>
    FilterBuilder& Including();
    FilterBuilder& Including(std::initializer_list<ComponentRequest>);
    
    // Get current count in inclusive set
    uint8_t GetNumInclusive();

    template<typename ... TYPES>
    FilterBuilder& Excluding();
    FilterBuilder& Excluding(std::initializer_list<ComponentId>);
    Filter Build(); 

    struct FilterCreateInfo
    {
        static const uint32_t MAX_EXCLUSIVE_COMPONENTS = 32;

        /// number of components in the inclusive set
        uint8_t numInclusive = 0;
        /// inclusive set
        ComponentId inclusive[Dataset::MAX_COMPONENT_BUFFERS];
        /// how we intend to access the components
        AccessMode access[Dataset::MAX_COMPONENT_BUFFERS];
        /// number of components in the exclusive set
        uint8_t numExclusive = 0;
        /// exclusive set
        ComponentId exclusive[MAX_EXCLUSIVE_COMPONENTS];
    };

    static Filter CreateFilter(FilterCreateInfo);

private:
    FilterCreateInfo info;

    template<class TYPE>
    void SetInclusive(size_t const i)
    {
        using UnqualifiedType = typename std::remove_const<typename std::remove_reference<TYPE>::type>::type;

        int offset = info.numInclusive++;
        n_assert(offset < Dataset::MAX_COMPONENT_BUFFERS);

        info.inclusive[offset] = GetComponentId<UnqualifiedType>();
        info.access[offset] = std::is_const<typename std::remove_reference<TYPE>::type>() ? Game::AccessMode::READ : Game::AccessMode::WRITE;
    }

    template<typename ... TYPES, std::size_t...Is>
    void UnrollInclusiveComponents(std::index_sequence<Is...>)
    {
        (SetInclusive<typename std::tuple_element<Is, std::tuple<TYPES...>>::type>(Is), ...);
    }

    template<class TYPE>
    void SetExclusive(size_t const i)
    {
        using UnqualifiedType = typename std::remove_const<typename std::remove_reference<TYPE>::type>::type;
        int offset = info.numExclusive++;
        n_assert(offset < FilterCreateInfo::MAX_EXCLUSIVE_COMPONENTS);
        info.exclusive[offset] = GetComponentId<UnqualifiedType>();
    }

    template<typename ... TYPES, std::size_t...Is>
    void UnrollExclusiveComponents(std::index_sequence<Is...>)
    {
        size_t offset = info.numInclusive;
        (SetExclusive<typename std::tuple_element<Is, std::tuple<TYPES...>>::type>(Is), ...);
    }
};

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
inline FilterBuilder&
FilterBuilder::Including()
{
    UnrollInclusiveComponents<TYPES...>(std::make_index_sequence<sizeof...(TYPES)>());
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
inline FilterBuilder&
FilterBuilder::Excluding()
{
    UnrollExclusiveComponents<TYPES...>(std::make_index_sequence<sizeof...(TYPES)>());
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline uint8_t
FilterBuilder::GetNumInclusive()
{
    return this->info.numInclusive;
}

} // namespace Game
