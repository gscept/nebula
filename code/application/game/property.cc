//------------------------------------------------------------------------------
//  property.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "property.h"

namespace Game
{
__ImplementAbstractClass(Game::Property, 'GmPr', Core::RefCounted);

Property::Property()
{
}

Property::~Property()
{
}

} // namespace Game
