//------------------------------------------------------------------------------
//  @file modelimporter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelimporter.h"
#include "io/filestream.h"
#include "io/uri.h"
#include "io/ioserver.h"
#include "io/assignregistry.h"
#include "timing/timer.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"

#include "model/animutil/animbuildersaver.h"
#include "model/skeletonutil/skeletonbuildersaver.h"

#include "model/modelwriter.h"
#include "model/meshutil/meshbuildersaver.h"

#include "model/import/gltf/node/ngltfscene.h"
#include "model/import/base/uniquestring.h"

#include "model/modelutil/modeldatabase.h"

#include "model/scenewriter.h"

#include "toolkitutil/text.h"
#include "toolkitutil/asset/assetimporter.h"

using namespace Util;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
    Gives the shapes of a regenerated scene the materials the shapes of the previous scene had, so reimporting keeps
    the materials assigned since. Shapes are matched by node name and primitive group, then by node name alone.
    Skinned models are packaged as a single skin node that shares one material, so skins take the first one found.
*/
static void
KeepMaterials(const SceneResourceT& oldScene, SceneResourceT& newScene)
{
    Util::Dictionary<Util::String, Util::String> byNodeAndGroup, byNode;
    const auto remember = [&byNodeAndGroup, &byNode](const ShapeNodeT* shape)
    {
        if (shape == nullptr || shape->transform == nullptr || shape->material.IsEmpty())
            return;
        const Util::String nodeAndGroup = Util::Format("%s/%d", shape->transform->name.AsCharPtr(), shape->prim_group);
        if (!byNodeAndGroup.Contains(nodeAndGroup))
            byNodeAndGroup.Add(nodeAndGroup, shape->material);
        if (!byNode.Contains(shape->transform->name))
            byNode.Add(shape->transform->name, shape->material);
    };
    const auto restore = [&byNodeAndGroup, &byNode](ShapeNodeT* shape)
    {
        if (shape == nullptr || shape->transform == nullptr)
            return;
        const Util::String nodeAndGroup = Util::Format("%s/%d", shape->transform->name.AsCharPtr(), shape->prim_group);
        IndexT i = byNodeAndGroup.FindIndex(nodeAndGroup);
        if (i != InvalidIndex)
        {
            shape->material = byNodeAndGroup.ValueAtIndex(i);
            return;
        }
        i = byNode.FindIndex(shape->transform->name);
        if (i != InvalidIndex)
        {
            shape->material = byNode.ValueAtIndex(i);
        }
    };

    for (const auto& shape : oldScene.shapes)
        remember(shape.get());
    for (auto& shape : newScene.shapes)
        restore(shape.get());

    Util::String skinMaterial;
    for (const auto& skin : oldScene.skins)
    {
        if (skin != nullptr && skin->shape != nullptr && !skin->shape->material.IsEmpty())
        {
            skinMaterial = skin->shape->material;
            break;
        }
    }
    if (!skinMaterial.IsEmpty())
    {
        for (auto& skin : newScene.skins)
        {
            if (skin != nullptr && skin->shape != nullptr)
                skin->shape->material = skinMaterial;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
ModelImporter::ModelImporter()
    : scene(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelImporter::~ModelImporter()
{
    n_assert(this->scene == nullptr);
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelImporter::ParseScene(ToolkitUtil::ImportFlags importFlags, float scale)
{
    n_error("Implement in subclass");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelImporter::ProcessFile(const IO::URI& file, ToolkitUtil::ImportFlags importFlags, float scale)
{
    // Reset all unique strings
    UniqueString::Reset();

    this->outputFiles.Clear();
    this->path = file;
    String localPath = file.GetHostAndLocalPath();
    if (!this->NeedsConversion(localPath))
    {
        this->logger->Print("Skipping %s\n", Text(localPath).Color(TextColor::Blue).AsCharPtr());
        return;
    }
    this->logger->Print("%s\n", Text(localPath).Color(TextColor::Blue).AsCharPtr());
    this->logger->Indent();

    Timing::Timer timer;
    Timing::Timer totalTime;
    totalTime.Start();

    timer.Start();

    // Run implementation specific scene parsing
    this->file = file.LocalPath().ExtractFileName();
    this->file.StripFileExtension();
    if (!this->ParseScene(importFlags, scale))
    {
        this->logger->Unindent();
        return;
    }

    this->logger->Print("%s %s (%.2f ms)\n", "Parsing...", Text("done").Color(TextColor::Green).AsCharPtr(), timer.GetTime() * 1000.0f);
    timer.Stop();

    timer.Reset();
    timer.Start();

    // calculate node global transforms for all nodes
    for (auto& node : this->scene->nodes)
    {
        if (node.base.parent == nullptr)
            node.CalculateGlobalTransforms();        
    }

    // Merge meshes based on vertex component and material
    Util::Array<SceneNode*> mergedMeshNodes;
    Util::Array<MeshBuilder*> mergedMeshes;
    this->scene->OptimizeGraphics(this->logger, mergedMeshNodes, mergedMeshes);

    // get physics mesh
    Util::Array<SceneNode*> physicsNodes;
    Util::Array<MeshBuilder*> physicsMeshes;
    MeshBuilderGroup group;
    scene->OptimizePhysics(physicsNodes, physicsMeshes);

    Util::String assetPath = IO::URI("src:assets").LocalPath();
    Util::String relativePath = file.LocalPath().StripSubstring(assetPath);
    relativePath.StripFileExtension();

    auto outputAssetPath = IO::URI(String::Sprintf("%s/%s.nasset", this->sourceDir.AsCharPtr(), this->file.AsCharPtr()));
    ToolkitUtil::ModelAssetT modelAsset;

    // If the file exists, load back the old file
    Ptr<IO::Stream> stream = IO::CreateStream(outputAssetPath);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        const void* data = stream->MemoryMap();
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset, (const uint8_t*)data);

        stream->MemoryUnmap();
        stream->Close();
    }

    // The mesh is always packaged again with these flags, so they're stored whether the scene is replaced or not,
    // which is what reimporting and the import window go by
    uint flags = 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::CalcNormals) ? ToolkitUtil::OptimizationFlags_CalculateNormals : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::CalcRigidSkin) ? ToolkitUtil::OptimizationFlags_CalculateRigidSkinning : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::RemoveRedundant) ? ToolkitUtil::OptimizationFlags_RemoveRedundantVertices : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::FlipUVs) ? ToolkitUtil::OptimizationFlags_FlipUVs : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::ImportColors) ? ToolkitUtil::OptimizationFlags_ImportVertexColors : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::ImportSecondaryUVs) ? ToolkitUtil::OptimizationFlags_ImportSecondaryUVs : 0;
    flags |= AllBits(importFlags, ToolkitUtil::ImportFlags::CalcTangents) ? ToolkitUtil::OptimizationFlags_CalculateTangentFrame : 0;
    modelAsset.flags = (ToolkitUtil::OptimizationFlags)flags;
    modelAsset.scale = scale;

    // remember where the asset comes from, relative to the project when it's in there so it survives moving the project
    modelAsset.source = GetSourceReference(file);

    if (mergedMeshes.Size() > 0)
    {
        // save mesh to file
        modelAsset.mesh = MeshBuilderSaver::PackImport(mergedMeshes, physicsMeshes, this->platform);
    }

    if (this->scene->animations.Size() > 0)
    {
        // Cleanup animations
        for (auto& anim : this->scene->animations)
        {
            anim.BuildVelocityCurves(AnimationFrameRate);
        }

        // now save actual animation
        modelAsset.animation = AnimBuilderSaver::PackImport(this->scene->animations, this->platform);
    }

    if (this->scene->skeletons.Size() > 0)
    {
        modelAsset.skeleton = SkeletonBuilderSaver::PackImport(this->scene->skeletons, this->platform);
    }

    // Save model file, potentially destructive as model might have material assigned
    if (mergedMeshNodes.Size() > 0)
    {
        if (IO::FileExists(outputAssetPath))
        {
            if (importFlags & ImportFlags::ReplaceExistingMesh)
            {
                std::unique_ptr<SceneResourceT> oldScene = std::move(modelAsset.scene);
                modelAsset.scene = SceneWriter::GenerateGraphicsModel(
                    this->sourceDir
                    , this->scene
                    , this->platform
                    , mergedMeshNodes
                    , importFlags
                );
                if (oldScene != nullptr && modelAsset.scene != nullptr)
                {
                    KeepMaterials(*oldScene, *modelAsset.scene);
                }
            }
        }
        else
        {
            modelAsset.scene = SceneWriter::GenerateGraphicsModel(
                this->sourceDir
                , this->scene
                , this->platform
                , mergedMeshNodes
                , importFlags
            );
        }
    }
    
    if (physicsNodes.Size() > 0)
    {
        modelAsset.physics = SceneWriter::GeneratePhysicsModel(
            this->sourceDir
            , this->scene
            , this->platform
            , physicsNodes
            , importFlags
        );
    }
    
    timer.Stop();

    IO::CreateDirectory(outputAssetPath.LocalPath().ExtractDirName());
    stream = IO::CreateStream(outputAssetPath);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        this->logger->Print("%s %s\n", Format("Generated asset: %s", Text(outputAssetPath.LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());

        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset);
        stream->Write(data.GetPtr(), data.Size());
        stream->Close();

        totalTime.Stop();
        this->logger->Unindent();
        this->logger->Print("%s %s\n\n", "Done"_text.Color(TextColor::Green).Style(FontMode::Bold).AsCharPtr(), Format("(%.2f ms)", totalTime.GetTime() * 1000).AsCharPtr());

        this->outputFiles.Append(outputAssetPath);
    }
    else
    {
        this->logger->Print("%s %s\n", Format("Failed to create: %s", Text(outputAssetPath.LocalPath()).Color(TextColor::Red).Style(FontMode::Underline).AsCharPtr()).AsCharPtr());
    }


    delete this->scene;

    // Delete merged meshes after export
    for (auto mesh : mergedMeshes)
        delete mesh;

    this->scene = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelImporter::NeedsConversion(const Util::String& path)
{
    // the import writes a single .nasset, along with the materials and textures extracted at the same time
    return AssetProcessorBase::NeedsConversion(path, this->GetOutputAssetPath(path));
}

//------------------------------------------------------------------------------
/**
*/
Util::String
ModelImporter::GetOutputAssetPath(const Util::String& path) const
{
    Util::String file = path.ExtractFileName();
    file.StripFileExtension();
    return Util::String::Sprintf("%s/%s.nasset", this->sourceDir.AsCharPtr(), file.AsCharPtr());
}

} // namespace ToolkitUtil
