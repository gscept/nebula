#pragma once
//------------------------------------------------------------------------------
/**
    @file attributes.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "attr/attributedefinition.h"

namespace Attr
{
DeclareString(PropertyName, 'PRNM', AccessMode::ReadOnly);
DeclareString(PropertyType, 'PRTP', AccessMode::ReadOnly);
DeclareString(PropertyDefaultValue, 'PRDV', AccessMode::ReadOnly);
} // namespace Attr
