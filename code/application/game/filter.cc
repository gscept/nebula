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

static Ids::IdAllocator<InclusiveTableMask, ExclusiveTableMask, ComponentArray, AccessModeArray>  filterAllocator;

//------------------------------------------------------------------------------
/**
*/
void
DestroyFilter(Filter filter)
{
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
FilterBuilder::FilterBuilder() :
    info(FilterCreateInfo())
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
    uint32_t filter = filterAllocator.Alloc();

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

    filterAllocator.Set(filter,
        InclusiveTableMask(inclusiveArray),
        ExclusiveTableMask(exclusiveArray),
        inclusiveArray,
        accessArray
    );

    return filter;
}
} // namespace Game
