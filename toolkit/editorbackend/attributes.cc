//------------------------------------------------------------------------------
//  @file attributes.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "attributes.h"
namespace Attr
{
DefineString(PropertyName, 'PRNM', AccessMode::ReadOnly);
DefineString(PropertyType, 'PRTP', AccessMode::ReadOnly);
DefineString(PropertyDefaultValue, 'PRDV', AccessMode::ReadOnly);
} // namespace Attr
