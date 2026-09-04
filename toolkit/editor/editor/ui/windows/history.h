#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::History

    Shows the command history

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class History : public BaseWindow
{
public:
    History();
    ~History();

    void Update();
    void Run(SaveMode save) override;
};

} // namespace Presentation

