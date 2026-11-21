#pragma once
//------------------------------------------------------------------------------
/**
    Editor navmesh sources

    Turns editor entities into the geometry entries of a navmesh work file.

    An entity gives sources for
    - a MeshCollider or HeightFieldCollider, placed like physics places the shape
    - a Model, one source per primitive node of the closest LOD, placed like it renders

    Every entry keeps the GUID of the entity it came from, so a navmesh can be gathered
    again after the level changed.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"
#include "math/bbox.h"
#include "navbuild/geometrysources.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/navigation/navmesh.h"
#include <memory>
#include <vector>

namespace Editor
{

struct NavMeshGatherOptions
{
    /// only use entities which are static, or below a static body
    bool onlyStatic = true;
    /// use the render geometry of models
    bool models = true;
    /// use mesh and height field colliders
    bool colliders = true;
};

/// gather the sources of an editor entity. Returns false and the reason if the entity gives none
bool GatherNavMeshSources(
    Editor::Entity entity,
    const NavMeshGatherOptions& options,
    std::vector<std::unique_ptr<Navigation::GeometryEntryT>>& out,
    Util::String& outReason);

/// find the editor entity with a GUID, invalid if there is none
Editor::Entity FindEntityByGuid(const Util::Guid& guid);

/// a NavArea volume in the level
struct NavAreaVolume
{
    Editor::Entity entity;          // editor entity
    Game::Entity gameEntity;
    Util::StringAtom area;
    int priority = 0;
    Math::mat4 transform;           // entity position, orientation and scale
    Math::bbox localBox;            // the box in the space of the entity, centered with the half extents
    Math::mat4 pointTransform;      // entity position and orientation, the points are not scaled
    Util::Array<Math::vec3> points; // points which extend the box, relative to pointTransform
};

/// how many points a NavArea can be extended with
static constexpr SizeT MaxNavAreaPoints = 32;

/// all NavArea volumes of the level
Util::Array<NavAreaVolume> GetNavAreaVolumes();
/// the world space bounds of a volume
Math::bbox NavAreaWorldBounds(const NavAreaVolume& volume);
/// the vertical prism a volume marks, the box extended by its points
Navigation::Build::AreaPrism NavAreaPrism(const NavAreaVolume& volume);
/// draw the outline of a prism at its bottom and top, with the vertical edges
void DrawAreaPrism(const Navigation::Build::AreaPrism& prism, const Math::vec4& color);
/// turn a volume into the area modifier of a work file, a vertical prism around the box
std::unique_ptr<Navigation::NavAreaModifierT> MakeAreaModifier(const NavAreaVolume& volume);

} // namespace Editor
