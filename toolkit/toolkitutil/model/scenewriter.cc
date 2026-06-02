//------------------------------------------------------------------------------
//  @file scenewriter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scenewriter.h"
#include "io/memorystream.h"
#include "io/ioserver.h"
#include "model/modelutil/modeldatabase.h"
#include "model/modelutil/modelconstants.h"
#include "model/modelutil/modelbuilder.h"
#include "util/crc.h"
#include "io/xmlwriter.h"

namespace ToolkitUtil
{

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
std::unique_ptr<SceneResourceT>
SceneWriter::GenerateGraphicsModel(
    const Util::String& basePath
    , const Scene* scene
    , const Platform::Code platform
    , const Util::Array<SceneNode*>& graphicsNodes
    , const ToolkitUtil::ImportFlags& flags
)
{
    n_assert(scene != nullptr);

    // make sure category directory exists
    IO::IoServer::Instance()->CreateDirectory(basePath);

    // extract file path from export path
    String file = scene->GetName();
    String category = basePath.ExtractLastDirName();

    // Loop through bounding boxes and get our scene bounding box
    Math::bbox globalBox;
    IndexT meshIndex;
    globalBox.begin_extend();
    for (meshIndex = 0; meshIndex < graphicsNodes.Size(); meshIndex++)
    {
        const SceneNode* mesh = graphicsNodes[meshIndex];
        globalBox.extend(mesh->base.boundingBox);
    }
    globalBox.end_extend();

    // format animation resource
    String animRes;
    animRes.Format("ani:%s/%s.nax", category.AsCharPtr(), file.AsCharPtr());

    // format skeleton resource
    String skeletonRes;
    skeletonRes.Format("ske:%s/%s.nsk", category.AsCharPtr(), file.AsCharPtr());

    // create mesh name
    String meshResource;
    meshResource.Format("msh:%s/%s.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    auto model = std::make_unique<ToolkitUtil::SceneResourceT>();
    model->name = scene->GetName();
    IndexT i;
    for (i = 0; i < graphicsNodes.Size(); i++)
    {
        // get mesh node
        const SceneNode* mesh = graphicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        // create and add a shape node
        if (mesh->base.isSkin)
        {
            for (IndexT j = 0; j < mesh->skin.skinFragments.Size(); j++)
            {
                auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
                transform->name = mesh->base.name;
                transform->translation = mesh->base.translation;
                transform->rotation = mesh->base.rotation;
                transform->scale = mesh->base.scale;

                auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
                shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
                shape->lod_min = mesh->mesh.minLodDistance;
                shape->lod_max = mesh->mesh.maxLodDistance;
                shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
                shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
                shape->prim_group = mesh->skin.skinFragments[j];
                shape->mesh_resource = meshResource;
                shape->mesh_index = mesh->mesh.meshIndex;
                shape->material = mesh->mesh.material;
                shape->transform = std::move(transform);

                auto skinFragment = std::make_unique<ToolkitUtil::SkinFragmentNodeT>();
                const Util::Array<IndexT>& indices = mesh->skin.jointLookup[j].KeysAsArray();
                skinFragment->joints = std::vector<int32_t>(indices.Begin(), indices.End());
                skinFragment->shape = std::move(shape);
                model->skins.push_back(std::move(skinFragment));
            }
        }
        else
        {
            auto transform = std::make_unique<ToolkitUtil::TransformNodeInfoT>();
            transform->name = mesh->base.name;
            transform->translation = mesh->base.translation;
            transform->rotation = mesh->base.rotation;
            transform->scale = mesh->base.scale;

            auto shape = std::make_unique<ToolkitUtil::ShapeNodeT>();
            shape->use_lod = mesh->mesh.lodIndex != InvalidIndex;
            shape->lod_min = mesh->mesh.minLodDistance;
            shape->lod_max = mesh->mesh.maxLodDistance;
            shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
            shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
            shape->prim_group = mesh->mesh.groupId;
            shape->mesh_resource = meshResource;
            shape->mesh_index = mesh->mesh.meshIndex;
            shape->material = mesh->mesh.material;
            shape->transform = std::move(transform);
            model->shapes.push_back(std::move(shape));

        }
    }

    return model;
}

//------------------------------------------------------------------------------
/**
*/
std::unique_ptr<PhysicsResourceT>
SceneWriter::GeneratePhysicsModel(
    const Util::String& basePath, 
    const Scene* scene, 
    const Platform::Code platform, 
    const Util::Array<SceneNode*>& physicsNodes, 
    const ToolkitUtil::ImportFlags& flags
)
{
    n_assert(scene != nullptr);

    // make sure category directory exists
    IO::IoServer::Instance()->CreateDirectory(basePath);

    // extract file path from export path
    String file = scene->GetName();
    String category = basePath.ExtractLastDirName();

    // Loop through bounding boxes and get our scene bounding box
    Math::bbox globalBox;
    IndexT meshIndex;
    globalBox.begin_extend();
    for (meshIndex = 0; meshIndex < physicsNodes.Size(); meshIndex++)
    {
        const SceneNode* mesh = physicsNodes[meshIndex];
        globalBox.extend(mesh->base.boundingBox);
    }
    globalBox.end_extend();

    // format animation resource
    String animRes;
    animRes.Format("ani:%s/%s.nax", category.AsCharPtr(), file.AsCharPtr());

    // format skeleton resource
    String skeletonRes;
    skeletonRes.Format("ske:%s/%s.nsk", category.AsCharPtr(), file.AsCharPtr());

    // create mesh name
    String meshResource;
    meshResource.Format("msh:%s/%s.nvx", category.AsCharPtr(), scene->GetName().AsCharPtr());

    auto model = std::make_unique<ToolkitUtil::PhysicsResourceT>();
    model->name = scene->GetName();
    IndexT i;
    for (i = 0; i < physicsNodes.Size(); i++)
    {
        // get mesh node
        const SceneNode* mesh = physicsNodes[i];

        // create full node path
        String nodePath;
        nodePath.Format("root/%s", mesh->base.name.AsCharPtr());

        // create and add a shape node
        if (mesh->base.isSkin)
        {
            for (IndexT j = 0; j < mesh->skin.skinFragments.Size(); j++)
            {
                auto transform = std::make_unique<ToolkitUtil::PhysicsTransformNodeInfoT>();
                transform->name = mesh->base.name;
                transform->translation = mesh->base.translation;
                transform->rotation = mesh->base.rotation;
                transform->scale = mesh->base.scale;

                auto shape = std::make_unique<ToolkitUtil::PhysicsNodeT>();
                shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
                shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
                shape->prim_group = mesh->skin.skinFragments[j];
                shape->mesh_resource = meshResource;
                shape->mesh_index = mesh->mesh.meshIndex;
                shape->is_render_mesh = false;
                shape->material = mesh->mesh.material;
                shape->transform = std::move(transform);

                auto skinFragment = std::make_unique<ToolkitUtil::PhysicsSkinFragmentNodeT>();
                const Util::Array<IndexT>& indices = mesh->skin.jointLookup[j].KeysAsArray();
                skinFragment->joints = std::vector<int32_t>(indices.Begin(), indices.End());
                skinFragment->shape = std::move(shape);
                model->skins.push_back(std::move(skinFragment));
            }
        }
        else
        {
            auto transform = std::make_unique<ToolkitUtil::PhysicsTransformNodeInfoT>();
            transform->name = mesh->base.name;
            transform->translation = mesh->base.translation;
            transform->rotation = mesh->base.rotation;
            transform->scale = mesh->base.scale;

            auto shape = std::make_unique<ToolkitUtil::PhysicsNodeT>();
            shape->bbox_min = xyz(mesh->base.boundingBox.pmin);
            shape->bbox_max = xyz(mesh->base.boundingBox.pmax);
            shape->prim_group = mesh->mesh.groupId;
            shape->mesh_resource = meshResource;
            shape->mesh_index = mesh->mesh.meshIndex;
            shape->material = mesh->mesh.material;
            shape->transform = std::move(transform);
            model->nodes.push_back(std::move(shape));

        }
    }
    return model;
}

} // namespace ToolkitUtil
