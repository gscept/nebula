#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::StyleEditor

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class StyleEditor : public BaseWindow
{
    __DeclareClass(StyleEditor)
public:
    StyleEditor();
    ~StyleEditor();

    void Run();

private:
};
__RegisterClass(StyleEditor)

} // namespace Presentation

