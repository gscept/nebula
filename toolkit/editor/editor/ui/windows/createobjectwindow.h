#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::CreateObjectWindow

    Opens when the user requests to create an object and add it to the scene

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class CreateObjectWindow : public BaseWindow
{
    __DeclareClass(CreateObjectWindow)
public:
    CreateObjectWindow();
    ~CreateObjectWindow();

    void Run(SaveMode save) override;

private:
};
__RegisterClass(CreateObjectWindow)

} // namespace Presentation

