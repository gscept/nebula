#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxSceneWriter
    
    Uses an FbxScene to write the appropriate N3 models
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "node/nfbxscene.h"
#include "n3util/n3writer.h"
#include "toolkit-common/platform.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class NFbxSceneWriter : public Core::RefCounted
{
    __DeclareClass(NFbxSceneWriter);
public:
    /// constructor
    NFbxSceneWriter();
    /// destructor
    virtual ~NFbxSceneWriter();

    /// set the scene pointer
    void SetScene(const Ptr<NFbxScene>& scene);
    /// get the scene pointer
    const Ptr<NFbxScene>& GetScene() const;

    /// sets the platform
    void SetPlatform(const ToolkitUtil::Platform::Code& platform);

    /// uses FbxScene to write models
    void GenerateModels(const Util::String& basePath, const ToolkitUtil::ExportFlags& flags, const ToolkitUtil::ExportMode& mode);

private:
    /// model creation entry point for merged static meshes
    void CreateStaticModel(const Ptr<ToolkitUtil::N3Writer>& modelWriter, const Util::Array<Ptr<NFbxMeshNode> >& meshes, const Util::String& path);
    /// model creation entry point for static meshes
    void CreateSkeletalModel(const Ptr<ToolkitUtil::N3Writer>& modelWriter, const Util::Array<Ptr<NFbxMeshNode> >& meshes, const Util::String& path);

    /// convenience function for writing constants-files
    void UpdateConstants(const Util::String& file, const Ptr<ToolkitUtil::ModelConstants>& constants);
    /// convenience function for writing attributes-files
    void UpdateAttributes(const Util::String& file, const Ptr<ToolkitUtil::ModelAttributes>& attributes);
    /// convenience function for writing physics-files
    void UpdatePhysics(const Util::String& file, const Ptr<ToolkitUtil::ModelPhysics>& physics);

    ToolkitUtil::Platform::Code platform;
    Ptr<NFbxScene> scene;   
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxSceneWriter::SetScene( const Ptr<NFbxScene>& scene )
{
    n_assert(scene.isvalid());
    this->scene = scene;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<NFbxScene>& 
NFbxSceneWriter::GetScene() const
{
    return this->scene;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxSceneWriter::SetPlatform( const ToolkitUtil::Platform::Code& platform )
{
    this->platform = platform;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------