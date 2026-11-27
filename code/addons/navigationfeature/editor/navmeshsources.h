#pragma once
//------------------------------------------------------------------------------
/**
    @file navmeshsources.h
    What a scene gives a navmesh: the geometry of its entities, and its area volumes.

    An entity gives sources for
    - a MeshCollider or HeightFieldCollider, placed like physics places the shape
    - a Model, one source per primitive node of the closest LOD, placed like it renders

    Every entry keeps the GUID of the entity it came from, so a navmesh can be gathered again after
    the scene changed. The geometry is read from the sim world, where the models are loaded; the
    entities are the scene's authored ones.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "game/entity.h"
#include "math/bbox.h"
#include "math/mat4.h"
#include "util/array.h"
#include "util/guid.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "navbuild/geometrysources.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/navigation/navmesh.h"
#include <memory>
#include <vector>

namespace Editor
{
class Scene;
}

namespace NavigationFeature
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

/// gather the sources of an authored entity. Returns false and the reason if the entity gives none
bool GatherNavMeshSources(
    Editor::Scene* scene,
    Game::Entity entity,
    const NavMeshGatherOptions& options,
    std::vector<std::unique_ptr<Navigation::GeometryEntryT>>& out,
    Util::String& outReason);

/// the authored entity with a GUID, invalid if the scene has none
Game::Entity FindEntityByGuid(Editor::Scene* scene, const Util::Guid& guid);
/// the name the scene gives an entity, '?' for one it does not know
Util::String EntityName(Editor::Scene* scene, Game::Entity entity);

/// a NavArea volume in the scene
struct NavAreaVolume
{
    Game::Entity entity;            // authored entity
    Util::StringAtom area;
    int priority = 0;
    Math::mat4 transform;           // entity position, orientation and scale
    Math::bbox localBox;            // the box in the space of the entity, centered with the half extents
    Math::mat4 pointTransform;      // entity position and orientation, the points are not scaled
    Util::Array<Math::vec3> points; // points which replace the footprint of the box, relative to pointTransform
};

/// all NavArea volumes of the scene
Util::Array<NavAreaVolume> GetNavAreaVolumes(Editor::Scene* scene);
/// the vertical prism a volume marks, the box or the outline of its points
Navigation::Build::AreaPrism NavAreaPrism(const NavAreaVolume& volume);
/// draw the outline of a prism at its bottom and top, with the vertical edges
void DrawAreaPrism(const Navigation::Build::AreaPrism& prism, const Math::vec4& color);
/// turn a volume into the area modifier of a work file, a vertical prism
std::unique_ptr<Navigation::NavAreaModifierT> MakeAreaModifier(Editor::Scene* scene, const NavAreaVolume& volume);

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
