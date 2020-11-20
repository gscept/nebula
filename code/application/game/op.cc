//------------------------------------------------------------------------------
//  op.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "op.h"

namespace Game
{
namespace Op
{

Memory::ArenaAllocator<1024> AddProperty::allocator = Memory::ArenaAllocator<1024>();

} // namespace Op
} // namespace Game
