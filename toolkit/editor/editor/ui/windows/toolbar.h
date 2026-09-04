#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Toolbar

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class Toolbar : public BaseWindow
{
public:
    Toolbar();
    ~Toolbar();

    void Run(SaveMode save) override;

private:
};

} // namespace Presentation

