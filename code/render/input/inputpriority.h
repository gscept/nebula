#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::InputPriority
    
    Input priorities for input handlers.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Input
{
class InputPriority
{
public:
    enum Code
    {
		DynUi = 0,
        Gui,
        Game,
        Other,
    };
};

} // namespace Input
//------------------------------------------------------------------------------

