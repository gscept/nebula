//------------------------------------------------------------------------------
//  @file navmeshsources.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navmeshsources.h"
#include "navbuild/geometrysources.h"
#include "game/world.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "physicsfeature/components/physicsfeature.h"
#include "graphicsfeature/components/model.h"
#include "models/modelcontext.h"
#include "models/model.h"
#include "models/nodes/primitivenode.h"
#include "basegamefeature/components/scale.h"
#include "navigationfeature/components/navigation.h"
#include "editor/components/editorcomponents.h"
#include "dynui/im3d/im3dcontext.h"
#include <algorithm>

namespace Editor
{

//------------------------------------------------------------------------------
/**
    An entity is static if it, or an entity it hangs below, is marked Static or is a static body
*/
static bool
IsStatic(Game::World* world, Game::Entity entity)
{
    // the depth guards against cycles in broken hierarchies
    for (int depth = 0; depth < 64 && entity != Game::Entity::Invalid() && world->IsValid(entity); depth++)
    {
        if (world->HasComponent<Game::Static>(entity) || world->HasComponent<PhysicsFeature::StaticBody>(entity))
        {
            return true;
        }
        if (!world->HasComponent<Game::HTransform>(entity))
        {
            break;
        }
        entity = world->GetComponent<Game::HTransform>(entity).parent;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
static std::unique_ptr<Navigation::GeometryEntryT>
MakeEntry(const Math::mat4& transform, const Util::Guid& guid)
{
    auto entry = std::make_unique<Navigation::GeometryEntryT>();
    entry->transform = transform;
    entry->entity_guid = guid.AsString().AsCharPtr();
    return entry;
}

//------------------------------------------------------------------------------
/**
    One source per primitive node. Nodes which only show beyond a LOD distance are left out, the closest
    LOD stands for the model
*/
static SizeT
GatherModel(Graphics::GraphicsEntityId graphicsEntity, const Util::Guid& guid, std::vector<std::unique_ptr<Navigation::GeometryEntryT>>& out, Util::String& outReason)
{
    if (!Models::ModelContext::IsEntityRegistered(graphicsEntity) || !Models::ModelContext::IsLoaded(graphicsEntity))
    {
        outReason = "its model isn't loaded yet";
        return 0;
    }

    const Models::NodeInstanceRange& renderableRange = Models::ModelContext::GetModelRenderableRange(graphicsEntity);
    const Models::NodeInstanceRange& transformRange = Models::ModelContext::GetModelTransformableRange(graphicsEntity);
    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
    const Models::ModelContext::ModelInstance::Transformable& transformables = Models::ModelContext::GetModelTransformables();

    SizeT count = 0;
    for (SizeT i = renderableRange.begin; i < renderableRange.end; i++)
    {
        if (renderables.nodeTypes[i] != Models::PrimitiveNodeType)
        {
            continue;
        }
        const auto& [lodMin, lodMax] = renderables.nodeLodDistances[i];
        if (lodMin < FLT_MAX && lodMin > 0.0f)
        {
            continue;
        }
        const Models::PrimitiveNode* node = static_cast<const Models::PrimitiveNode*>(renderables.nodes[i]);
        const IO::Path& mesh = node->GetMeshPath();
        if (!mesh.IsFile())
        {
            continue;
        }

        // the node matrices are in world space, they are what the model renders with
        const Math::mat4& transform = transformables.nodeTransforms[transformRange.begin + renderables.nodeTransformIndex[i]];
        auto entry = MakeEntry(transform, guid);
        Navigation::MeshSourceT source;
        source.resource = mesh.AsString().AsCharPtr();
        source.mesh_index = node->GetMeshIndex();
        source.prim_group = node->GetPrimitiveGroupIndex();
        entry->geometry.Set(source);
        out.push_back(std::move(entry));
        count++;
    }
    if (count == 0)
    {
        outReason = "its model has no mesh nodes";
    }
    return count;
}

//------------------------------------------------------------------------------
/**
*/
bool
GatherNavMeshSources(
    Editor::Entity entity,
    const NavMeshGatherOptions& options,
    std::vector<std::unique_ptr<Navigation::GeometryEntryT>>& out,
    Util::String& outReason)
{
    outReason.Clear();
    if (entity.index >= (uint)state.editables.Size())
    {
        outReason = "it isn't an editor entity";
        return false;
    }
    const Editable& editable = state.editables[entity.index];
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    const Game::Entity gameEntity = editable.gameEntity;
    if (gameEntity == Game::Entity::Invalid() || !world->IsValid(gameEntity))
    {
        outReason = "it has no game entity";
        return false;
    }
    if (options.onlyStatic && !IsStatic(world, gameEntity))
    {
        outReason = "it isn't static";
        return false;
    }

    const SizeT before = out.size();
    const Math::vec3 position = world->GetComponent<Game::Position>(gameEntity);
    const Math::quat orientation = world->GetComponent<Game::Orientation>(gameEntity);

    if (options.colliders && world->HasComponent<PhysicsFeature::MeshCollider>(gameEntity))
    {
        const PhysicsFeature::MeshCollider& collider = world->GetComponent<PhysicsFeature::MeshCollider>(gameEntity);
        if (!collider.isTrigger && collider.resource.IsValid())
        {
            auto entry = MakeEntry(Navigation::Build::ColliderTransform(position, orientation, collider.localPosition, collider.localOrientation), editable.guid);
            Navigation::MeshSourceT source;
            source.resource = collider.resource.Value();
            source.mesh_index = 0;
            source.prim_group = collider.primGroup;
            entry->geometry.Set(source);
            out.push_back(std::move(entry));
        }
    }
    if (options.colliders && world->HasComponent<PhysicsFeature::HeightFieldCollider>(gameEntity))
    {
        const PhysicsFeature::HeightFieldCollider& collider = world->GetComponent<PhysicsFeature::HeightFieldCollider>(gameEntity);
        if (!collider.isTrigger && collider.resource.IsValid())
        {
            auto entry = MakeEntry(Navigation::Build::ColliderTransform(position, orientation, collider.localPosition, collider.localOrientation), editable.guid);
            Navigation::HeightFieldSourceT source;
            source.resource = collider.resource.Value();
            source.height_range = collider.heightRange;
            source.size_x = collider.targetSizeX;
            source.size_z = collider.targetSizeY;
            entry->geometry.Set(source);
            out.push_back(std::move(entry));
        }
    }

    Util::String modelReason;
    if (options.models && world->HasComponent<GraphicsFeature::Model>(gameEntity))
    {
        GatherModel(world->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId, editable.guid, out, modelReason);
    }

    if (out.size() == before)
    {
        outReason = modelReason.IsValid() ? modelReason : Util::String("it has no model, mesh collider or height field collider");
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Editor::Entity
FindEntityByGuid(const Util::Guid& guid)
{
    for (IndexT i = 0; i < state.editables.Size(); i++)
    {
        const Editable& editable = state.editables[i];
        if (editable.guid == guid && editable.gameEntity != Game::Entity::Invalid())
        {
            return ToEditorEntity(editable.gameEntity);
        }
    }
    return Editor::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<NavAreaVolume>
GetNavAreaVolumes()
{
    Util::Array<NavAreaVolume> volumes;
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::Filter filter = Game::FilterBuilder()
        .Including<const Game::Entity, const NavigationFeature::NavArea, const Editor::EditorEntity>()
        .Build();
    Game::Dataset dataset = world->Query(filter);
    for (IndexT v = 0; v < dataset.numViews; v++)
    {
        Game::Dataset::View const* view = dataset.views + v;
        for (IndexT i = 0; i < view->numInstances; i++)
        {
            const Game::Entity gameEntity = *((const Game::Entity*)view->buffers[0] + i);
            const NavigationFeature::NavArea& navArea = *((const NavigationFeature::NavArea*)view->buffers[1] + i);
            const Editor::EditorEntity& editorEntity = *((const Editor::EditorEntity*)view->buffers[2] + i);

            NavAreaVolume volume;
            volume.entity = editorEntity.id;
            volume.gameEntity = gameEntity;
            volume.area = navArea.area;
            volume.priority = navArea.priority;
            volume.transform = Math::trs(
                world->GetComponent<Game::Position>(gameEntity),
                world->GetComponent<Game::Orientation>(gameEntity),
                world->GetComponent<Game::Scale>(gameEntity));
            const Math::vec3 extents = Math::vec3(Math::abs(navArea.extents.x), Math::abs(navArea.extents.y), Math::abs(navArea.extents.z));
            volume.localBox.pmin = Math::point(-extents.x, -extents.y, -extents.z);
            volume.localBox.pmax = Math::point(extents.x, extents.y, extents.z);
            volume.pointTransform = Math::trs(
                world->GetComponent<Game::Position>(gameEntity),
                world->GetComponent<Game::Orientation>(gameEntity),
                Math::vec3(1.0f));
            const SizeT count = Math::min((SizeT)navArea.pointCount, (SizeT)navArea.points.size());
            for (SizeT p = 0; p < count; p++)
            {
                volume.points.Append(navArea.points[p]);
            }
            volumes.Append(volume);
        }
    }
    Game::DestroyFilter(filter);
    return volumes;
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox
NavAreaWorldBounds(const NavAreaVolume& volume)
{
    const Navigation::Build::AreaPrism prism = NavAreaPrism(volume);
    Math::bbox bounds;
    bounds.begin_extend();
    for (const Math::vec3& p : prism.outline)
    {
        bounds.extend(Math::vec3(p.x, prism.minY, p.z));
        bounds.extend(Math::vec3(p.x, prism.maxY, p.z));
    }
    return bounds;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::Build::AreaPrism
NavAreaPrism(const NavAreaVolume& volume)
{
    return Navigation::Build::MakeAreaPrism(volume.transform, volume.localBox, volume.pointTransform, volume.points.Begin(), volume.points.Size());
}

//------------------------------------------------------------------------------
/**
*/
void
DrawAreaPrism(const Navigation::Build::AreaPrism& prism, const Math::vec4& color)
{
    const SizeT count = prism.outline.Size();
    for (SizeT i = 0; i < count; i++)
    {
        const Math::vec3& a = prism.outline[i];
        const Math::vec3& b = prism.outline[(i + 1) % count];
        Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, prism.minY, a.z), Math::point(b.x, prism.minY, b.z)), 2.0f, color);
        Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, prism.maxY, a.z), Math::point(b.x, prism.maxY, b.z)), 2.0f, color);
        Im3d::Im3dContext::DrawLine(Math::line(Math::point(a.x, prism.minY, a.z), Math::point(a.x, prism.maxY, a.z)), 2.0f, color);
    }
}

//------------------------------------------------------------------------------
/**
    Recast marks areas in vertical prisms: the outline seen from above and a height span. A box which
    is tilted is marked by the prism around it, the convex hull of its corners and points seen from above
*/
std::unique_ptr<Navigation::NavAreaModifierT>
MakeAreaModifier(const NavAreaVolume& volume)
{
    const Navigation::Build::AreaPrism prism = NavAreaPrism(volume);
    const Util::Array<Math::vec3>& hull = prism.outline;
    const float minY = prism.minY;
    const float maxY = prism.maxY;

    auto modifier = std::make_unique<Navigation::NavAreaModifierT>();
    for (const Math::vec3& p : hull)
    {
        modifier->convex_area_points.push_back(p);
    }
    modifier->min_y = minY;
    modifier->max_y = maxY;
    modifier->area = volume.area.Value();
    modifier->priority = volume.priority;
    if (volume.entity.index < (uint)state.editables.Size())
    {
        modifier->entity_guid = state.editables[volume.entity.index].guid.AsString().AsCharPtr();
    }
    return modifier;
}

} // namespace Editor
