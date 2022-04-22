#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::History

    Shows the command history

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class History : public BaseWindow
{
    __DeclareClass(History)
public:
    History();
    ~History();

    void Update();
    void Run();
};
__RegisterClass(History)

} // namespace Presentation

