//------------------------------------------------------------------------------
//  commonattributes.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "commonattributes.h"

namespace Attr
{

//------------------------------------------------------------------------------
/**
    Common attributes
*/
DefineGuid(Guid, 'auid', AccessMode::ReadOnly);
DefineString(Name, 'Anme', AccessMode::ReadWrite);

} // namespace Attr
