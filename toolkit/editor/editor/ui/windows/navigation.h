#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Navigation

    Navigation related settings and dialogs

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class Navigation: public BaseWindow
{
public:
    Navigation();
    ~Navigation();

    void Run(SaveMode save) override;

private:
    Graphics::GraphicsEntityId defaultCamera;
};

} // namespace Presentation

