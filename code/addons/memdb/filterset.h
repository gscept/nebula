#pragma once
//------------------------------------------------------------------------------
/**
    @class	MemDb::Filterset

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
    FilterSet(const std::initializer_list<PropertyId> inclusive, const std::initializer_list<PropertyId> exclusive);
    /// construct from single inclusive array
    explicit FilterSet(const Util::FixedArray<PropertyId>& inclusive);
    /// construct from inclusive and exclusive arrays
    explicit FilterSet(const Util::FixedArray<PropertyId>& inclusive, const Util::FixedArray<PropertyId>& exclusive);
    /// construct from table signatures
    explicit FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<PropertyId>& inclusiveProperties);

    /// get the inclusive signature mask
    TableSignature const& Inclusive() const;
    /// get the exclusive signature mask
    TableSignature const& Exclusive() const;
    /// get a fixed array of all the properties contained in the inclusive set
    Util::FixedArray<PropertyId> const& PropertyIds() const;

private:
    /// categories must include all properties in this signature
    TableSignature inclusive;
    /// categories must NOT contain any attributes in this array
    TableSignature exclusive;
    /// properties that are in the inclusive set
    Util::FixedArray<PropertyId> inclusiveProperties;
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const std::initializer_list<PropertyId> inclusive, const std::initializer_list<PropertyId> exclusive) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveProperties(inclusive)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const Util::FixedArray<PropertyId>& inclusive) :
    inclusive(inclusive),
    exclusive(),
    inclusiveProperties(inclusive)
{
    // empty
};

//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const Util::FixedArray<PropertyId>& inclusive, const Util::FixedArray<PropertyId>& exclusive) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveProperties(inclusive)
{
    // empty
}
//------------------------------------------------------------------------------
/**
*/
inline
FilterSet::FilterSet(const TableSignature& inclusive, const TableSignature& exclusive, const Util::FixedArray<PropertyId>& inclusiveProperties) :
    inclusive(inclusive),
    exclusive(exclusive),
    inclusiveProperties(inclusiveProperties)
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
inline Util::FixedArray<PropertyId> const&
FilterSet::PropertyIds() const
{
    return this->inclusiveProperties;
}

} // namespace MemDb
