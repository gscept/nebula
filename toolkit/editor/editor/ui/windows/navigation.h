#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Navigation

    Navigation related settings and dialogs

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

/// Ensure navigation runtime UI hook is loaded and ready.
bool EnsureNavigationUiHookLoaded();

class Navigation: public BaseWindow
{
    __DeclareClass(Navigation)
public:
    Navigation();
    ~Navigation();

    void Run(SaveMode save) override;

private:
    Graphics::GraphicsEntityId defaultCamera;
    bool missingHookWarningShown = false;
};
__RegisterClass(Navigation)

} // namespace Presentation

