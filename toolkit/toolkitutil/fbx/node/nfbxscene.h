#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxNodeRegistry
    
    Parses an FBX scene and allocates Nebula-style nodes which can then be retrieved within the parsers
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "core/refcounted.h"
#include "nfbxmeshnode.h"
#include "nfbxjointnode.h"
#include "nfbxtransformnode.h"
#include "meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class NFbxScene : public Core::RefCounted
{
    __DeclareSingleton(NFbxScene);  
    __DeclareClass(NFbxScene);
public:
    /// constructor
    NFbxScene();
    /// destructor
    virtual ~NFbxScene();
    /// open the FbxNodeRegistry
    bool Open();
    /// close the FbxNodeRegistry
    void Close();
    /// return if FbxNodeRegistry is open
    bool IsOpen() const;

    /// sets the name of the scene
    void SetName(const Util::String& name);
    /// gets the name of the scene
    const Util::String& GetName() const;
    /// sets the category for the scene
    void SetCategory(const Util::String& category);
    /// gets the category for the scene
    const Util::String& GetCategory() const;

    /// sets up the scene
    void Setup(FbxScene* scene, const ToolkitUtil::ExportFlags& exportFlags, const ToolkitUtil::ExportMode& exportMode, float scale);
    /// clears the scene, do this when all parsing is done
    void Cleanup();

    /// gets the current scene
    FbxScene* GetScene() const;
    /// get the scale of the scene
    const float GetScale() const;
    /// converts export mode to string
    const Util::String GetSceneFeatureString();

    /// returns general node
    NFbxNode* GetNode(FbxNode* key);
    /// returns true if general node is registered
    bool HasNode(FbxNode* key);
    /// returns reference to full list of nodes
    const Util::Dictionary<FbxNode*, Ptr<NFbxNode> >& GetNodes();
    /// returns reference to list of root nodes
    const Util::Array<Ptr<NFbxNode> >& GetRootNodes();
    /// returns mesh node
    NFbxMeshNode* GetMeshNode(FbxNode* key);
    /// returns true if mesh node is registered
    bool HasMeshNode(FbxNode* key);
    /// returns reference to dictionary of mesh nodes
    const Util::Array<Ptr<NFbxMeshNode> > GetMeshNodes() const;
    /// removes mesh node from main node dictionary and mesh node dictionary
    void RemoveMeshNode(const Ptr<NFbxMeshNode>& node);
    /// returns joint node
    NFbxJointNode* GetJointNode(FbxNode* key);
    /// returns true if joint node is registered
    bool HasJointNode(FbxNode* key);
    /// returns reference to dictionary of joint nodes
    const Util::Array<Ptr<NFbxJointNode> > GetJointNodes() const;
    /// removes joint node from main node dictionary and joint node dictionary
    void RemoveJointNode(const Ptr<NFbxJointNode>& node);
    /// returns reference to list of skeleton roots
    const Util::Array<Ptr<NFbxJointNode> > GetSkeletonRoots() const;
    /// returns transform node
    NFbxTransformNode* GetTransformNode(FbxNode* key);
    /// returns true if transform node is registered
    bool HasTransformNode(FbxNode* key);
    /// returns reference to dictionary of transform nodes
    const Util::Array<Ptr<NFbxTransformNode> > GetTransformNodes() const;
    /// removes transform node from main node dictionary and transform node dictionary
    void RemoveTransformNode(const Ptr<NFbxTransformNode>& node);

    /// returns reference to array of animations
    const Util::Array<ToolkitUtil::AnimBuilder> GetAnimations() const;

    /// flattens all hierarchical structures except for joints and lights
    void Flatten();
    /// splits skins if necessary
    void FragmentSkins();
    /// returns mesh for scene
    ToolkitUtil::MeshBuilder* GetMesh() const;
    /// returns physics mesh for scene
    ToolkitUtil::MeshBuilder* GetPhysicsMesh() const;
    /// returns list of physics nodes
    const Util::Array<Ptr<NFbxMeshNode> >& GetPhysicsNodes() const;

private:

    /// converts FBX time mode to FPS
    float TimeModeToFPS(const FbxTime::EMode& timeMode);

    Util::Dictionary<FbxNode*, Ptr<NFbxMeshNode> > meshNodes;
    Util::Dictionary<FbxNode*, Ptr<NFbxJointNode> > jointNodes;
    Util::Dictionary<FbxNode*, Ptr<NFbxTransformNode> > transformNodes;
    Util::Dictionary<FbxNode*, Ptr<NFbxNode> > lodNodes;
    Util::Array<Ptr<NFbxMeshNode> > physicsNodes;

    Util::Dictionary<FbxNode*, Ptr<NFbxNode> > nodes;   // this list is only for use
    Util::Array<Ptr<NFbxNode> > rootNodes;
    Util::Array<Ptr<NFbxJointNode> > skeletonRoots;
    ToolkitUtil::MeshBuilder* mesh;
    ToolkitUtil::MeshBuilder* physicsMesh;
    FbxScene* scene;
    Util::String name;
    Util::String category;
    ToolkitUtil::ExportMode mode;
    ToolkitUtil::ExportFlags flags;
    float scale;
    bool isOpen;
}; 

//------------------------------------------------------------------------------
/**
*/
inline bool
NFbxScene::IsOpen() const
{
    return this->isOpen;
}


//------------------------------------------------------------------------------
/**
*/
inline FbxScene* 
NFbxScene::GetScene() const
{
    return this->scene;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxScene::SetName( const Util::String& name )
{
    n_assert(name.IsValid())
    this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
NFbxScene::GetName() const
{
    return this->name;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxScene::SetCategory( const Util::String& category )
{
    n_assert(category.IsValid());
    this->category = category;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
NFbxScene::GetCategory() const
{
    return this->category;
}


//------------------------------------------------------------------------------
/**
*/
inline ToolkitUtil::MeshBuilder*
NFbxScene::GetMesh() const
{
    return this->mesh;
}


//------------------------------------------------------------------------------
/**
*/
inline ToolkitUtil::MeshBuilder* 
NFbxScene::GetPhysicsMesh() const
{
    return this->physicsMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<NFbxMeshNode> >& 
NFbxScene::GetPhysicsNodes() const
{
    return this->physicsNodes;
}

//------------------------------------------------------------------------------
/**
*/
inline const float 
NFbxScene::GetScale() const
{
    return this->scale;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------