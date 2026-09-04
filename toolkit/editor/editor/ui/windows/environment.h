#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Environment

    Environment 

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Environment : public BaseWindow
{
public:
    Environment();
    ~Environment();

    void Update();
    void Run(SaveMode save) override;

private:
    
};

} // namespace Presentation

