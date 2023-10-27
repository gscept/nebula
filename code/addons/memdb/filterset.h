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
#include "attributeid.h"

namespace MemDb
{

class FilterSet
{
public:
    /// default constructor
    FilterSet() = default;
    /// construct from initializer lists
    FilterSet(const std::initializer_list<AttributeId> inclusive, const std::initializer_list<AttributeId> exclusive);
    /// construct from single inclusive array
    explicit FilterSet(const Util::FixedArray<AttributeId>& inclusive);
    /// construct from inclusive and exclusive arrays
    explicit FilterSet(const Util::FixedArray<AttributeId>& inclusive, const Util::FixedArray<AttributeId>& exclusive);
    /// construct from table signatures
    explicit FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<AttributeId>& inclusiveComponents);

    /// get the inclusive signature mask
    TableSignature const& Inclusive() const;
    /// get the exclusive signature mask
    TableSignature const& Exclusive() const;
    /// get a fixed array of all the components contained in the inclusive set
    Util::FixedArray<AttributeId> const& PropertyIds() const;

private:
    /// categories must include all components in this signature
    TableSignature inclusive;
    /// categories must NOT contain any components in this array
    TableSignature exclusive;
    /// components that are in the inclusive set
    Util::FixedArray<AttributeId> inclusiveComponents;
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const std::initializer_list<AttributeId> inclusive, const std::initializer_list<AttributeId> exclusive) :
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
FilterSet::FilterSet(const Util::FixedArray<AttributeId>& inclusive) :
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
FilterSet::FilterSet(const Util::FixedArray<AttributeId>& inclusive, const Util::FixedArray<AttributeId>& exclusive) :
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
FilterSet::FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<AttributeId>& inclusiveComponents) :
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
inline Util::FixedArray<AttributeId> const&
FilterSet::PropertyIds() const
{
    return this->inclusiveComponents;
}

} // namespace MemDb
