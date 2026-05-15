//------------------------------------------------------------------------------
//  @file modelimporter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelimporter.h"
#include "io/filestream.h"
#include "io/uri.h"
#include "timing/timer.h"

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
ModelImporter::ImportFile(const IO::URI& file, ToolkitUtil::ImportFlags importFlags, float scale)
{
    // Reset all unique strings
    UniqueString::Reset();

    outputFiles.Clear();

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
    if (!this->ParseScene(importFlags, scale))
        return;

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
    Util::Array<SceneNode*> mergedCharacterNodes;
    Util::Array<MeshBuilder*> mergedMeshes;
    this->scene->OptimizeGraphics(this->logger, mergedMeshNodes, mergedCharacterNodes, mergedMeshes);

    Util::String assetPath = IO::URI("src:assets").LocalPath();
    Util::String relativePath = file.LocalPath().StripSubpath(assetPath);
    IO::URI destinationFiles[] =
    {
        IO::URI(String::Sprintf("%s/%s.namsh", this->folder.AsCharPtr(), this->file.AsCharPtr())) // mesh
        , IO::URI(String::Sprintf("%s/%s_ph.namsh", this->folder.AsCharPtr(), this->file.AsCharPtr())) // physics
        , IO::URI(String::Sprintf("%s/%s.naani", this->folder.AsCharPtr(), this->file.AsCharPtr())) // animation
        , IO::URI(String::Sprintf("%s/%s.naske", this->folder.AsCharPtr(), this->file.AsCharPtr())) // skeleton
    };

    enum DestinationFile
    {
        Mesh
        , Physics
        , Animation
        , Skeleton
    };

    // save mesh to file
    if (!MeshBuilderSaver::SaveImport(destinationFiles[DestinationFile::Mesh], mergedMeshes, this->platform))
    {
        this->logger->Error("Failed to save mesh file : % s\n", destinationFiles[DestinationFile::Mesh].LocalPath().AsCharPtr());
    }
    else
    {
        this->UpdateResourceMapping("urn:msh:" + relativePath + "/" + this->file, file.LocalPath(), destinationFiles[DestinationFile::Mesh].LocalPath());
        outputFiles.Append(destinationFiles[DestinationFile::Mesh]);
    }
    

    // Delete merged meshes after export
    for (auto mesh : mergedMeshes)
        delete mesh;

    timer.Stop();

    // print info
    this->logger->Print("%s %s\n", Format("Generated mesh: %s", Text(destinationFiles[DestinationFile::Mesh].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());

    // get physics mesh
    Util::Array<SceneNode*> physicsNodes;
    MeshBuilder* physicsMesh = nullptr;
    MeshBuilderGroup group;
    scene->OptimizePhysics(physicsNodes, physicsMesh);

    // only save physics mesh if it exists
    if (physicsMesh && physicsMesh->GetNumTriangles() > 0)
    {
        timer.Reset();
        timer.Start();

        // save mesh
        if (!MeshBuilderSaver::SaveImport(destinationFiles[DestinationFile::Physics], { physicsMesh }, this->platform))
        {
            this->logger->Error("Failed to save physics mesh file : % s\n", destinationFiles[DestinationFile::Physics].LocalPath().AsCharPtr());
        }
        else
        {
            this->UpdateResourceMapping("urn:phy:" + relativePath + "/" + this->file, file.LocalPath(), destinationFiles[DestinationFile::Physics].LocalPath());
            outputFiles.Append(destinationFiles[DestinationFile::Physics]);
        }

        timer.Stop();

        // print info
        this->logger->Print("%s %s\n", Format("Generated physics: %s", Text(destinationFiles[DestinationFile::Physics].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());

    }

    if (this->scene->animations.Size() > 0)
    {
        timer.Reset();
        timer.Start();

        // Cleanup animations
        for (auto& anim : this->scene->animations)
        {
            anim.BuildVelocityCurves(AnimationFrameRate);
        }

        // now save actual animation
        if (!AnimBuilderSaver::SaveImport(destinationFiles[DestinationFile::Animation], this->scene->animations, this->platform))
        {
            this->logger->Error("Failed to save animation file: %s\n", destinationFiles[DestinationFile::Animation].LocalPath().AsCharPtr());
        }
        else
        {
            this->UpdateResourceMapping("urn:ani:" + relativePath + "/" + this->file, file.LocalPath(), destinationFiles[DestinationFile::Animation].LocalPath());
            outputFiles.Append(destinationFiles[DestinationFile::Animation]);
        }

        timer.Stop();
        this->logger->Print("%s %s\n", Format("Generated animation: %s", Text(destinationFiles[DestinationFile::Animation].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());
    }

    if (this->scene->skeletons.Size() > 0)
    {
        timer.Reset();
        timer.Start();

        if (!SkeletonBuilderSaver::SaveImport(destinationFiles[DestinationFile::Skeleton], this->scene->skeletons, this->platform))
        {
            this->logger->Error("Failed to save skeleton file: %s\n", destinationFiles[DestinationFile::Skeleton].LocalPath().AsCharPtr());
        }
        else
        {
            this->UpdateResourceMapping("urn:ske:" + relativePath + "/" + this->file, file.LocalPath(), destinationFiles[DestinationFile::Skeleton].LocalPath());
            outputFiles.Append(destinationFiles[DestinationFile::Skeleton]);
        }

        timer.Stop(); 
        this->logger->Print("%s %s\n", Format("Generated skeleton: %s", Text(destinationFiles[DestinationFile::Skeleton].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());
    }

    // Finally, output model hierarchy to n3
    SceneWriter::GenerateModels(
        Util::String::Sprintf("%s/", this->folder.AsCharPtr())
        , this->scene
        , this->platform
        , mergedMeshNodes
        , physicsNodes
        , mergedCharacterNodes
        , importFlags
    );

    WriteIntermediateFile(file, outputFiles);

    totalTime.Stop();
    this->logger->Unindent();
    this->logger->Print("%s %s\n\n", "Done"_text.Color(TextColor::Green).Style(FontMode::Bold).AsCharPtr(), Format("(%.2f ms)", totalTime.GetTime() * 1000).AsCharPtr());

    delete this->scene;
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
    bool sourceNewer = ImporterBase::NeedsConversion(path, model);

    // ...if the mesh is newer
    bool meshNewer = ImporterBase::NeedsConversion(path, mesh);

    // check if physics settings were changed. no way to tell if we have a new physics mesh in it, so we just export it anyway
    bool physicsMeshNewer = ImporterBase::NeedsConversion(path, physMesh);

    // return true if either is true
    return sourceNewer || meshNewer || physicsMeshNewer;
}

} // namespace ToolkitUtil
