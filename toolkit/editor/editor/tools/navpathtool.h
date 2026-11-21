#pragma once
//------------------------------------------------------------------------------
/**
    @file navpathtool.h

    Tests paths on a loaded navmesh in the viewport.

    A click on the navmesh places the start, shift and a click places the end. The path
    between them is searched again every frame, so it follows a rebake or a change of the
    area costs right away. Escape ends the tool.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolinterface.h"
#include "navigationfeature/navquery.h"

namespace Tools
{

class NavPathTool : public ToolInterface
{
public:
    void Render(Presentation::Modules::Viewport* viewport) override;
    void Update(Presentation::Modules::Viewport* viewport) override;
    bool IsModifying() const override;
    void Abort() override;

    /// start testing paths on a navmesh
    static void Begin(Navigation::NavMeshId navMesh);
    /// stop testing
    static void End();
    /// true while the tool runs
    static bool Active();
    /// the navmesh paths are tested on
    static Navigation::NavMeshId NavMesh();
    /// the filter the path is searched with, the costs can be changed while the tool runs
    static NavigationFeature::QueryFilter& Filter();
    /// the path between the start and the end, Failed until both are placed
    static const NavigationFeature::NavPath& Path();
    /// true once the start, or the end, is placed
    static bool HasStart();
    static bool HasEnd();
    /// clear the start and the end
    static void Clear();
};

} // namespace Tools
