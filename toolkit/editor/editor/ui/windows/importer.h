#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Physics

    Physics related settings and dialogs

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class Importer : public BaseWindow
{
    __DeclareClass(Importer)
public:
    Importer();
    ~Importer();

    void Run(SaveMode save) override;
};
__RegisterClass(Importer)

} // namespace Presentation

