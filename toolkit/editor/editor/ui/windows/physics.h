#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Physics

    Physics related settings and dialogs

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class Physics: public BaseWindow
{
public:
    Physics();
    ~Physics();

    void Run(SaveMode save) override;

private:
    Graphics::GraphicsEntityId defaultCamera;
};

} // namespace Presentation

