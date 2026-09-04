#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::StyleEditor

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class StyleEditor : public BaseWindow
{
public:
    StyleEditor();
    ~StyleEditor();

    void Run(SaveMode save) override;

private:
};

} // namespace Presentation

