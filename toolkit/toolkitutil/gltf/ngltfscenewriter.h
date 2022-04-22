#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFSceneWriter

    Uses an GltfScene to write the appropriate N3 models

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "n3util/n3writer.h"
#include "toolkit-common/platform.h"
#include "toolkit-common/base/exporttypes.h"
#include "toolkitutil/modelutil/modelattributes.h"
#include "node/ngltfscene.h"
//------------------------------------------------------------------------------
namespace ToolkitUtil
{

struct NglTFSceneInfo
{
    Util::Array<ModelConstants::ShapeNode> shapes;
    //Util::Array<ModelConstants::SkinNode> skins;
};

class NglTFSceneWriter : public Core::RefCounted
{
    __DeclareClass(NglTFSceneWriter);
public:
    /// constructor
    NglTFSceneWriter();
    /// destructor
    virtual ~NglTFSceneWriter();

    /// sets the platform
    void SetPlatform(const ToolkitUtil::Platform::Code& platform);

    void SetSurfaceExportPath(const Util::String& path);

    void SetScene(const Ptr<NglTFScene>& nScene);

    void SetForce(bool force);

    /// uses GltfScene to write models
    void GenerateModels(const Util::String& basePath, const ToolkitUtil::ExportFlags& flags, const ToolkitUtil::ExportMode& mode);

private:
    /// model creation entry point for merged static meshes
    void CreateStaticModel(const Ptr<ToolkitUtil::N3Writer>& modelWriter, const Util::Array<Ptr<NglTFMesh> >& meshes, const Util::String& path);

    /// convenience function for writing constants-files
    void UpdateConstants(const Util::String& file, const Ptr<ToolkitUtil::ModelConstants>& constants);
    /// convenience function for writing attributes-files
    void UpdateAttributes(const Util::String& file, const Ptr<ToolkitUtil::ModelAttributes>& attributes);
#if PHYSEXPORT
    /// convenience function for writing physics-files
    void UpdatePhysics(const Util::String& file, const Ptr<ToolkitUtil::ModelPhysics>& physics);
#endif

    /// set to true if you want to force override attributes and constants from the GLTF
    bool force = false;

    Ptr<NglTFScene> scene;

    Util::String surExportPath;

    ToolkitUtil::Platform::Code platform;
};

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFSceneWriter::SetPlatform(const ToolkitUtil::Platform::Code& platform)
{
    this->platform = platform;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFSceneWriter::SetSurfaceExportPath(const Util::String& path)
{
    this->surExportPath = path;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFSceneWriter::SetScene(const Ptr<NglTFScene>& nScene)
{
    this->scene = nScene;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFSceneWriter::SetForce(bool force)
{
    this->force = force;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------