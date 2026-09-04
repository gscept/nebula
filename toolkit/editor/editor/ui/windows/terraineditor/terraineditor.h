#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::TerrainEditor

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
#include "editor/tools/toolinterface.h"

namespace Presentation
{

class TerrainEditor : public BaseWindow
{
public:
    TerrainEditor();
    ~TerrainEditor();

    void Run(SaveMode save) override;

    bool ShouldRun() override;

private:
};
} // namespace Presentation


namespace Tools
{

class TerrainEditorTool : public ToolInterface
{
public:
    /// Call before Update
    void Render(Presentation::Modules::Viewport* viewport) override;
    /// Call after render
    void Update(Presentation::Modules::Viewport* viewport) override;

    bool IsModifying() const override;
};

} // namespace Tools
