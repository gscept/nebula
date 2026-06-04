//------------------------------------------------------------------------------
//  @file modelimporter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelimporter.h"
#include "io/filestream.h"
#include "io/uri.h"
#include "io/ioserver.h"
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

#include "toolkit-common/text.h"

using namespace Util;
namespace ToolkitUtil
{

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

    auto outputAssetPath = IO::URI(String::Sprintf("%s/%s.nasset", this->folder.AsCharPtr(), this->file.AsCharPtr()));
    ToolkitUtil::ModelAssetT modelAsset;

    // If the file exists, load back the old file
    Ptr<IO::Stream> stream = IO::CreateStream(outputAssetPath);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        const void* data = stream->MemoryMap();
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(modelAsset, (const uint8_t*)data);

        stream->Close();
    }

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
                modelAsset.scene = SceneWriter::GenerateGraphicsModel(
                    this->folder
                    , this->scene
                    , this->platform
                    , mergedMeshNodes
                    , importFlags
                );
            }
        }
        else
        {
            modelAsset.scene = SceneWriter::GenerateGraphicsModel(
                this->folder
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
            this->folder
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
    Util::String category = path.ExtractLastDirName();
    Util::String file = path.ExtractFileName();
    file.StripFileExtension();

    // Check if any of the output files are old
    Util::String model = "src:assets/" + category + "/" + file + ".namdl";
    Util::String physModel = "src:assets/" + category + "/" + file + ".actor";
    Util::String mesh = "src:assets/" + category + "/" + file + ".namsh";
    Util::String physMesh = "src:assets/" + category + "/" + file + "_ph.namsh";
    Util::String animation = "src:assets/" + category + "/" + file + ".naani";    

    // check if fbx is newer than model
    bool sourceNewer = AssetProcessorBase::NeedsConversion(path, model);

    // ...if the mesh is newer
    bool meshNewer = AssetProcessorBase::NeedsConversion(path, mesh);

    // check if physics settings were changed. no way to tell if we have a new physics mesh in it, so we just export it anyway
    bool physicsMeshNewer = AssetProcessorBase::NeedsConversion(path, physMesh);

    // return true if either is true
    return sourceNewer || meshNewer || physicsMeshNewer;
}

} // namespace ToolkitUtil
