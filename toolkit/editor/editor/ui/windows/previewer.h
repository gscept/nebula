#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Previewer

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
#include "graphics/view.h"
#include "graphics/stage.h"

namespace Presentation
{

class Previewer : public BaseWindow
{
    __DeclareClass(Previewer)
public:
    Previewer();
    ~Previewer();

    void Update();
    void Run();

    Modules::Viewport viewport;
    Ptr<Graphics::View> graphicsView;
    Ptr<Graphics::Stage> graphicsStage;
};
__RegisterClass(Previewer)

} // namespace Presentation

