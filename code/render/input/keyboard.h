#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::Keyboard
    
    An input handler which represents a keyboard for polling.

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__ || __LINUX__
#include "input/base/keyboardbase.h"
namespace Input
{
class Keyboard : public Base::KeyboardBase
{
    __DeclareClass(Keyboard);
};
}
#else
#error "Keyboard class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

