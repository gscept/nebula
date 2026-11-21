//------------------------------------------------------------------------------
//  @file modifier.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "input/modifier.h"

namespace Input
{
namespace Modifier
{

//------------------------------------------------------------------------------
/**
    The modifiers as the combo string writes them: the groups, in the order a combo is written in. A
    single side is written as its group, which is what the file can read back.
*/
Util::String
ToString(Code mods)
{
    Util::String modsString;
    for (IndexT i = 0; i < NumGroups; i++)
    {
        if (mods & GroupMasks[i])
        {
            if (!modsString.IsEmpty())
                modsString.Append("+");
            modsString.Append(GroupNames[i]);
        }
    }
    return modsString;
}

//------------------------------------------------------------------------------
/**
*/
Code
FromString(const Util::String& token)
{
    for (IndexT i = 0; i < NumGroups; i++)
    {
        if (token == GroupNames[i])
            return GroupMasks[i];
    }
    return NoModifier;
}

} // namespace Modifier
} // namespace Input
//------------------------------------------------------------------------------
