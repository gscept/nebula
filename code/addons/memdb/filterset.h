#pragma once
//------------------------------------------------------------------------------
/**
    @class  MemDb::Filterset

    Used to query a database.
    
    @note   Setting up a filterset is not cheap, and should be done as
            infrequently as possible

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "propertyid.h"

namespace MemDb
{

class FilterSet
{
public:
    /// default constructor
    FilterSet() = default;
    /// construct from initializer lists
    FilterSet(const std::initializer_list<ComponentId> inclusive, const std::initializer_list<ComponentId> exclusive);
    /// construct from single inclusive array
    explicit FilterSet(const Util::FixedArray<ComponentId>& inclusive);
    /// construct from inclusive and exclusive arrays
    explicit FilterSet(const Util::FixedArray<ComponentId>& inclusive, const Util::FixedArray<ComponentId>& exclusive);
    /// construct from table signatures
    explicit FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<ComponentId>& inclusiveComponents);

    /// get the inclusive signature mask
    TableSignature const& Inclusive() const;
    /// get the exclusive signature mask
    TableSignature const& Exclusive() const;
    /// get a fixed array of all the components contained in the inclusive set
    Util::FixedArray<ComponentId> const& PropertyIds() const;

private:
    /// categories must include all components in this signature
    TableSignature inclusive;
    /// categories must NOT contain any components in this array
    TableSignature exclusive;
    /// components that are in the inclusive set
    Util::FixedArray<ComponentId> inclusiveComponents;
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const std::initializer_list<ComponentId> inclusive, const std::initializer_list<ComponentId> exclusive) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveComponents(inclusive)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const Util::FixedArray<ComponentId>& inclusive) :
    inclusive(inclusive),
    exclusive(),
    inclusiveComponents(inclusive)
{
    // empty
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const Util::FixedArray<ComponentId>& inclusive, const Util::FixedArray<ComponentId>& exclusive) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveComponents(inclusive)
{
    // empty
}
//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<ComponentId>& inclusiveComponents) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveComponents(inclusiveComponents)
{
}
;

//------------------------------------------------------------------------------
/**
*/
inline TableSignature const&
FilterSet::Inclusive() const
{
    return this->inclusive;
}

//------------------------------------------------------------------------------
/**
*/
inline TableSignature const&
FilterSet::Exclusive() const
{
    return this->exclusive;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::FixedArray<ComponentId> const&
FilterSet::PropertyIds() const
{
    return this->inclusiveComponents;
}

} // namespace MemDb
