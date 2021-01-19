#pragma once
//------------------------------------------------------------------------------
/**
    @file   commonattributes.h

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "attrid.h"

namespace Attr
{

//------------------------------------------------------------------------------
/**
    Common attributes
*/
DeclareGuid(Guid, 'auid', AccessMode::ReadOnly);
DeclareString(Name, 'anme', AccessMode::ReadWrite);

} // namespace Attr
