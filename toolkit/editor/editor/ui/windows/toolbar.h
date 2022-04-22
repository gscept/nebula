#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Toolbar

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class Toolbar : public BaseWindow
{
    __DeclareClass(Toolbar)
public:
    Toolbar();
    ~Toolbar();

    void Run();

private:
};
__RegisterClass(Toolbar)

} // namespace Presentation

