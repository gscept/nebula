#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Environment

    Environment 

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Environment : public BaseWindow
{
    __DeclareClass(Environment)
public:
    Environment();
    ~Environment();

    void Update();
    void Run(SaveMode save) override;

private:
    
};
__RegisterClass(Environment)

} // namespace Presentation

