//------------------------------------------------------------------------------
//  @file modelexporter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelexporter.h"
#include "io/ioserver.h"
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
ModelExporter::ModelExporter()
    : scene(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelExporter::~ModelExporter()
{
    n_assert(this->scene == nullptr);
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelExporter::ParseScene()
{
    n_error("Implement in subclass");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelExporter::ExportFile(const IO::URI& file)
{
    IO::IoServer* ioServer = IO::IoServer::Instance();

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
    this->file = localPath.ExtractFileName();
    this->file.StripFileExtension();
    this->category = localPath.ExtractLastDirName();
    this->logger->Print("%s\n", Text(localPath).Color(TextColor::Blue).AsCharPtr());
    this->logger->Indent();

    // Get animations from scene
    Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(this->category + "/" + this->file);
    this->exportFlags = attributes->GetExportFlags();
    this->sceneScale = attributes->GetScale();

    Timing::Timer timer;
    Timing::Timer totalTime;
    totalTime.Start();

    timer.Start();

    // Run implementation specific scene parsing
    if (!this->ParseScene())
        return;

    this->logger->Print("%s %s (%.2f ms)\n", "Parsing...", Text("done").Color(TextColor::Green).AsCharPtr(), timer.GetTime() * 1000.0f);
    //n_printf("  [Parsed source] (%.2f ms)\n", timer.GetTime() * 1000.0f);
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
    Util::Array<MeshBuilderGroup> mergedGroups;
    Util::Array<MeshBuilder*> mergedMeshes;
    this->scene->OptimizeGraphics(mergedMeshNodes, mergedCharacterNodes, mergedGroups, mergedMeshes);

    Util::String physicsMeshExportName = String::Sprintf("msh:%s/%s_ph.nvx", this->category.AsCharPtr(), this->file.AsCharPtr());
    IO::URI destinationFiles[] =
    {
        IO::URI(String::Sprintf("msh:%s/%s.nvx", this->category.AsCharPtr(), this->file.AsCharPtr())) // mesh
        , IO::URI(physicsMeshExportName) // physics
        , IO::URI(String::Sprintf("ani:%s/%s.nax", this->category.AsCharPtr(), this->file.AsCharPtr())) // animation
        , IO::URI(String::Sprintf("ske:%s/%s.nsk", this->category.AsCharPtr(), this->file.AsCharPtr())) // skeleton
    };

    enum DestinationFile
    {
        Mesh
        , Physics
        , Animation
        , Skeleton
    };

    // save mesh to file
    if (!MeshBuilderSaver::Save(destinationFiles[DestinationFile::Mesh], mergedMeshes, mergedGroups, this->platform))
    {
        this->logger->Error("Failed to save NVX file : % s\n", destinationFiles[DestinationFile::Mesh].LocalPath().AsCharPtr());
    }
    else
    {
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
        group.SetFirstTriangleIndex(0);
        group.SetNumTriangles(physicsMesh->GetNumTriangles());
        if (!MeshBuilderSaver::Save(destinationFiles[DestinationFile::Physics], { physicsMesh }, { group }, this->platform))
        {
            this->logger->Error("Failed to save physics NVX file : % s\n", destinationFiles[DestinationFile::Physics].LocalPath().AsCharPtr());
        }
        else
        {
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
        if (!AnimBuilderSaver::Save(destinationFiles[DestinationFile::Animation], this->scene->animations, this->platform))
        {
            this->logger->Error("Failed to save animation file: %s\n", destinationFiles[DestinationFile::Animation].LocalPath().AsCharPtr());
        }
        else
        {
            outputFiles.Append(destinationFiles[DestinationFile::Animation]);
        }

        timer.Stop();
        this->logger->Print("%s %s\n", Format("Generated animation: %s", Text(destinationFiles[DestinationFile::Animation].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());
    }

    if (this->scene->skeletons.Size() > 0)
    {
        timer.Reset();
        timer.Start();

        if (!SkeletonBuilderSaver::Save(destinationFiles[DestinationFile::Skeleton], this->scene->skeletons, this->platform))
        {
            this->logger->Error("Failed to save skeleton file: %s\n", destinationFiles[DestinationFile::Skeleton].LocalPath().AsCharPtr());
        }
        else
        {
            outputFiles.Append(destinationFiles[DestinationFile::Skeleton]);
        }

        timer.Stop(); 
        this->logger->Print("%s %s\n", Format("Generated skeleton: %s", Text(destinationFiles[DestinationFile::Skeleton].LocalPath()).Color(TextColor::Green).Style(FontMode::Underline).AsCharPtr()).AsCharPtr(), Format("(%.2f ms)", timer.GetTime() * 1000.0f).AsCharPtr());
    }

    // Finally, output model hierarchy to n3
    SceneWriter::GenerateModels(
        Util::String::Sprintf("src:assets/%s/", this->category.AsCharPtr())
        , this->scene
        , this->platform
        , mergedMeshNodes
        , physicsNodes
        , mergedCharacterNodes
        , physicsMeshExportName
        , this->exportFlags
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
ModelExporter::NeedsConversion(const Util::String& path)
{
    Util::String category = path.ExtractLastDirName();
    Util::String file = path.ExtractFileName();
    file.StripFileExtension();

    // check both if FBX is newer than .n3
    Util::String model = "mdl:" + category + "/" + file + ".n3";
    Util::String physModel = "phys:" + category + "/" + file + ".actor";
    Util::String mesh = "msh:" + category + "/" + file + ".nvx";
    Util::String physMesh = "msh:" + category + "/" + file + "_ph.nvx";
    Util::String animation = "ani:" + category + "/" + file + ".nax";
    Util::String constants = "src:assets/" + category + "/" + file + ".constants";
    Util::String attributes = "src:assets/" + category + "/" + file + ".attributes";
    Util::String physics = "src:assets/" + category + "/" + file + ".physics";

    // check if fbx is newer than model
    bool sourceNewer = ExporterBase::NeedsConversion(path, model);

    // and if the .constants is older than the fbx
    bool constantsNewer = ExporterBase::NeedsConversion(constants, model);

    // and if the .attributes is older than the n3 (attributes controls both model, and animation resource)
    bool attributesNewer = ExporterBase::NeedsConversion(attributes, model);
    // ...and if the .physics is older than the np3
    bool physicsNewer = ExporterBase::NeedsConversion(physics, physModel);

    // ...if the mesh is newer
    bool meshNewer = ExporterBase::NeedsConversion(path, mesh);

    // check if physics settings were changed. no way to tell if we have a new physics mesh in it, so we just export it anyway
    bool physicsMeshNewer = ExporterBase::NeedsConversion(physics, mesh);

    // return true if either is true
    return sourceNewer || constantsNewer || attributesNewer || physicsNewer || meshNewer || physicsMeshNewer;
}

} // namespace ToolkitUtil
