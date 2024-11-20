//------------------------------------------------------------------------------
//  @file filter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "filter.h"
#include "api.h"
#include "ids/idallocator.h"

namespace Game
{
using ComponentArray = Util::FixedArray<ComponentId>;
using AccessModeArray = Util::FixedArray<AccessMode>;

// 0: inclusiveMask, 1: exclusiveMask, 2: inclusiveComponents, 3: accessmodes, 4: exclusiveComponents
static Ids::IdAllocator<InclusiveTableMask, ExclusiveTableMask, ComponentArray, AccessModeArray, ComponentArray> filterAllocator;

//------------------------------------------------------------------------------
/**
*/
void
DestroyFilter(Filter filter)
{
    filterAllocator.Get<0>(filter) = {};
    filterAllocator.Get<1>(filter) = {};
    filterAllocator.Get<2>(filter) = {};
    filterAllocator.Get<3>(filter) = {};
    filterAllocator.Get<4>(filter) = {};
    filterAllocator.Dealloc(filter);
}

//------------------------------------------------------------------------------
/**
*/
InclusiveTableMask const&
GetInclusiveTableMask(Filter filter)
{
    return filterAllocator.Get<0>(filter);
}

//------------------------------------------------------------------------------
/**
*/
ExclusiveTableMask const&
GetExclusiveTableMask(Filter filter)
{
    return filterAllocator.Get<1>(filter);
}

//------------------------------------------------------------------------------
/**
*/
Util::FixedArray<ComponentId> const&
ComponentsInFilter(Filter filter)
{
    return filterAllocator.Get<2>(filter);
}

//------------------------------------------------------------------------------
/**
*/
Util::FixedArray<AccessMode> const&
AccessModesInFilter(Filter filter)
{
    return filterAllocator.Get<3>(filter);
}

//------------------------------------------------------------------------------
/**
*/
Util::FixedArray<ComponentId> const&
ExcludedComponentsInFilter(Filter filter)
{
    return filterAllocator.Get<4>(filter);
}

//------------------------------------------------------------------------------
/**
*/
FilterBuilder::FilterBuilder()
    : info(FilterCreateInfo())
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FilterBuilder&
FilterBuilder::Including(std::initializer_list<ComponentRequest> components)
{
    for (auto request : components)
    {
        int index = this->info.numInclusive++;
        n_assert(index < Dataset::MAX_COMPONENT_BUFFERS);
        this->info.inclusive[index] = request.id;
        this->info.access[index] = request.access;
    }
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
FilterBuilder&
FilterBuilder::Excluding(std::initializer_list<ComponentId> components)
{
    for (auto request : components)
    {
        int index = this->info.numExclusive++;
        n_assert(index < FilterCreateInfo::MAX_EXCLUSIVE_COMPONENTS);
        this->info.exclusive[index] = request.id;
    }
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
Filter
FilterBuilder::Build()
{
    return FilterBuilder::CreateFilter(this->info);
}

//------------------------------------------------------------------------------
/**
*/
Filter
FilterBuilder::CreateFilter(FilterCreateInfo info)
{
    n_assert(info.numInclusive > 0);
    Ids::Id32 filter = filterAllocator.Alloc();

    ComponentArray inclusiveArray;
    inclusiveArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        inclusiveArray[i] = info.inclusive[i];
    }

    ComponentArray exclusiveArray;
    exclusiveArray.Resize(info.numExclusive);
    for (uint8_t i = 0; i < info.numExclusive; i++)
    {
        exclusiveArray[i] = info.exclusive[i];
    }

    AccessModeArray accessArray;
    accessArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        accessArray[i] = info.access[i];
    }

#define VERIFY_FILTERS 1
#if _DEBUG && VERIFY_FILTERS
    for (auto const& comp : inclusiveArray)
    {
        int count = 0;
        for (auto const& other : inclusiveArray)
        {
            count += (comp == other);
        }
        n_assert2(count == 1, "Component cannot exists more than once in a filter!");
    }

    for (auto const& comp : exclusiveArray)
    {
        int count = 0;
        for (auto const& other : exclusiveArray)
        {
            count += (comp == other);
        }
        n_assert2(count == 1, "Component cannot exists more than once in a filter!");
    }

    for (auto const& comp : inclusiveArray)
    {
        int count = 0;
        for (auto const& other : exclusiveArray)
        {
            count += (comp == other);
        }
        n_assert2(count == 0, "Component cannot exist in both inclusive and exclusive sets in the same filter!");
    }
#endif

    filterAllocator.Set(
        filter,
        InclusiveTableMask(inclusiveArray),
        ExclusiveTableMask(exclusiveArray),
        inclusiveArray,
        accessArray,
        exclusiveArray
    );

    return filter;
}
} // namespace Game
