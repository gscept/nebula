#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Outline

    Shows an outline of the world and its entities.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class Outline : public BaseWindow
{
    __DeclareClass(Outline)
public:
    Outline();
    ~Outline();

    void Run();

private:
};
__RegisterClass(Outline)

} // namespace Presentation

