#pragma once
//------------------------------------------------------------------------------
/**
    @file navigationeditor.h
    The navigation feature's part of the editor.

    - the navmesh asset type, edited in the asset editor, and dropped on the viewport as an entity
      that loads it
    - the inspector fields of NavArea and NavAgent: the area and the agent kind are picked from what
      the project has, and an area volume's outline is listed and edited
    - the navigation panel: what the viewport shows, the project's area table, and path tests
    - what the viewport shows and takes the clicks of while an outline is edited or paths are tested

    The outline is edited in the viewport: a click on the surface adds a point to the edge closest
    to it, dragging a point moves it along the surface, Delete over a point or ctrl and a click on
    it removes it, Escape stops. Points land on the loaded navmeshes, or on a level plane where the
    mouse misses them.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "game/entity.h"
#include "navigationfeature/navquery.h"

namespace NavigationFeature
{

/// register what the feature gives the editor. The components are registered by then
void RegisterEditorContributions();
/// take it back, before the feature's caches are gone
void ClearEditorContributions();

/// what the viewport draws of navigation
struct NavigationDisplay
{
    /// the loaded navmeshes and the agents walking them
    bool navMeshes = true;
    /// the area volumes of the scene, not only the selected ones
    bool areaVolumes = true;
};
NavigationDisplay& GetNavigationDisplay();

/// start editing the outline of a NavArea entity, the authored one
void BeginOutlineEdit(Game::Entity entity);
/// stop editing
void EndOutlineEdit();
/// the entity whose outline is edited, invalid when none is
Game::Entity OutlineEditTarget();
/// the outline point under the mouse in the viewport, or InvalidIndex
IndexT OutlineHoveredPoint();
/// highlight an outline point from its row in the inspector, InvalidIndex for none
void SetOutlineHighlightedPoint(IndexT index);

/// start testing paths on a navmesh, a click places the start and shift and a click the end
void BeginPathTest(Navigation::NavMeshId navMesh);
/// stop testing
void EndPathTest();
bool IsPathTesting();
/// the navmesh paths are tested on
Navigation::NavMeshId PathTestNavMesh();
/// the filter the path is searched with, the costs can be changed while it runs
QueryFilter& PathTestFilter();
/// the path between the start and the end, Failed until both are placed
const NavPath& PathTestPath();
bool PathTestHasStart();
bool PathTestHasEnd();
/// clear the start and the end
void ClearPathTest();
/// place the start or the end where a script says, for testing without a mouse
void SetPathTestPoint(bool end, const Math::point& point);

/// make an area volume of an area in the active scene and select it
void CreateAreaVolume(const char* area);

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
