//------------------------------------------------------------------------------
//  @file modelexporter.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelexporter.h"
#include "io/ioserver.h"
#include "io/filestream.h"
#include "timing/timer.h"

#include "model/animutil/animbuildersaver.h"
#include "model/skeletonutil/skeletonbuildersaver.h"

#include "model/modelwriter.h"
#include "model/meshutil/meshbuildersaver.h"
#include "model/import/fbx/node/nfbxscene.h"
#include "model/import/gltf/node/ngltfscene.h"

#include "model/modelutil/modeldatabase.h"

#include "model/scenewriter.h"

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

    this->path = file;
    String localPath = file.GetHostAndLocalPath();
    this->file = localPath.ExtractFileName();
    this->file.StripFileExtension();
    this->category = localPath.ExtractLastDirName();
    n_printf("Exporting assets:\n        %s\n", localPath.AsCharPtr());

    // Get animations from scene
    Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(this->category + "/" + this->file);
    this->exportFlags = attributes->GetExportFlags();
    this->sceneScale = attributes->GetScale();

    // we want to see if the model file exists, because that's the only way to know if ALL the resources are old or new...
    if (!this->NeedsConversion(localPath))
    {
        n_printf("    [File has not changed, ignoring export]\n\n", localPath.AsCharPtr());
        return;
    }

    // Run implementation specific scene parsing
    if (!this->ParseScene())
        return;

    Timing::Timer timer;
    timer.Reset();
    timer.Start();

    // Merge meshes based on vertex component and material
    Util::Array<SceneNode*> mergedMeshNodes;
    Util::Array<SceneNode*> mergedCharacterNodes;
    Util::Array<MeshBuilderGroup> mergedGroups;
    Util::Array<MeshBuilder*> mergedMeshes;
    this->scene->OptimizeGraphics(mergedMeshNodes, mergedCharacterNodes, mergedGroups, mergedMeshes);

    String destinationFiles[] =
    {
        String::Sprintf("msh:%s/%s.nvx", this->category.AsCharPtr(), this->file.AsCharPtr()) // mesh
        , "" // physics
        , String::Sprintf("ani:%s/%s.nax", this->category.AsCharPtr(), this->file.AsCharPtr()) // animation
        , String::Sprintf("ske:%s/%s.nsk", this->category.AsCharPtr(), this->file.AsCharPtr())
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
        n_error("Failed to save NVX file: %s\n", destinationFiles[DestinationFile::Mesh].AsCharPtr());
    }

    // Delete merged meshes after export
    for (auto mesh : mergedMeshes)
        delete mesh;

    timer.Stop();

    // print info
    n_printf("[Generated graphics mesh: %s] (%.2f ms)\n", destinationFiles[DestinationFile::Mesh].AsCharPtr(), timer.GetTime() * 1000.0f);

    // get physics mesh
    Util::Array<SceneNode*> physicsNodes;
    MeshBuilder* physicsMesh = nullptr;
    MeshBuilderGroup group;
    scene->OptimizePhysics(physicsNodes, physicsMesh);

    // only save physics mesh if it exists
    if (physicsMesh && physicsMesh->GetNumTriangles() > 0)
    {
        destinationFiles[DestinationFile::Physics] = String::Sprintf("msh:%s/%s_ph.nvx", this->category.AsCharPtr(), this->file.AsCharPtr());

        // save mesh
        group.SetFirstTriangleIndex(0);
        group.SetNumTriangles(physicsMesh->GetNumTriangles());
        if (!MeshBuilderSaver::Save(destinationFiles[DestinationFile::Physics], { physicsMesh }, { group }, this->platform))
        {
            n_error("Failed to save physics NVX file: %s\n", destinationFiles[DestinationFile::Physics].AsCharPtr());
        }

        // print info
        n_printf("[Generated physics mesh: %s]\n", destinationFiles[DestinationFile::Physics].AsCharPtr());
    }

    if (this->scene->animations.Size() > 0)
    {
        // Cleanup animations
        for (auto& anim : this->scene->animations)
        {
            anim.BuildVelocityCurves(AnimationFrameRate);
        }

        // now save actual animation
        if (!AnimBuilderSaver::Save(destinationFiles[DestinationFile::Animation], this->scene->animations, this->platform))
        {
            n_error("Failed to save animation file: %s\n", destinationFiles[DestinationFile::Animation].AsCharPtr());
        }
        n_printf("[Generated animation: %s]\n", destinationFiles[DestinationFile::Animation].AsCharPtr());
    }

    if (this->scene->skeletons.Size() > 0)
    {
        if (!SkeletonBuilderSaver::Save(destinationFiles[DestinationFile::Skeleton], this->scene->skeletons, this->platform))
        {
            n_error("Failed to save skeleton file: %s\n", destinationFiles[DestinationFile::Skeleton].AsCharPtr());
        }
        n_printf("[Generated skeleton: %s]\n", destinationFiles[DestinationFile::Skeleton].AsCharPtr());
    }

    n_printf("\n");

    // Finally, output model hierarchy to n3
    SceneWriter::GenerateModels(
        Util::String::Sprintf("src:assets/%s/", this->category.AsCharPtr())
        , this->scene
        , this->platform
        , mergedMeshNodes
        , physicsNodes
        , mergedCharacterNodes
        , destinationFiles[DestinationFile::Physics]
        , this->exportFlags
    );

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
    Util::String physModel = "phys:" + category + "/" + file + ".np3";
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
