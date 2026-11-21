#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Navigation

    Creates and bakes navmesh assets from the selected entities.

    The window edits one navmesh work file, work:folder/name.nanav. Creating it from the
    selection writes the sources of the selected entities, baking hands the file to the live
    batcher and the baked navmesh is loaded so it can be drawn. Rebuilding gathers the sources
    of the same entities again, by their GUIDs, after the level changed.

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class Navigation: public BaseWindow
{
public:
    Navigation();
    ~Navigation();

    void Run(SaveMode save) override;
    /// draws the area volumes of the level
    void Update() override;

private:
    Graphics::GraphicsEntityId defaultCamera;
};

} // namespace Presentation
