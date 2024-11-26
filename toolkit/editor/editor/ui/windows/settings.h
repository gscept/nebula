#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Settings

    Editor settings

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Settings : public BaseWindow
{
    __DeclareClass(Settings)
public:
    Settings();
    ~Settings();

    void Update();
    void Run(SaveMode save) override;

private:
    
};
__RegisterClass(Settings)

} // namespace Presentation

