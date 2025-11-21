#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::TerrainEditor

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"

namespace Presentation
{

class TerrainEditor : public BaseWindow
{
    __DeclareClass(TerrainEditor)
public:
    TerrainEditor();
    ~TerrainEditor();

    void Run(SaveMode save) override;

private:
};
__RegisterClass(TerrainEditor)

} // namespace Presentation

