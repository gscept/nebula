#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Scene

    Shows the current scene and its entities.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
namespace Presentation
{

class Scene : public BaseWindow
{
    __DeclareClass(Scene)
public:
    Scene();
    ~Scene();

    void Update();
    void Run();

    Modules::Viewport viewPort;
};
__RegisterClass(Scene)

} // namespace Presentation

