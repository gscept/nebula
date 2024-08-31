#pragma once
//------------------------------------------------------------------------------
/**
    Browser for resources

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class ResourceBrowser : public BaseWindow
{
    __DeclareClass(ResourceBrowser)
public:
    ResourceBrowser();
    ~ResourceBrowser();

    /// Render
    void Run(SaveMode save) override;
private:
};

__RegisterClass(ResourceBrowser)

} // namespace Presentation
