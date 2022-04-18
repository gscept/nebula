#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFScene

    Parses a glTF scene and allocates Nebula-style nodes which can then be retrieved within the parsers

    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "core/singleton.h"
#include "core/refcounted.h"
#include "ngltfmesh.h"
#include "ngltfnode.h"
#include "meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "gltf/gltfdata.h"
#include "meshprimitive.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class NglTFScene : public Core::RefCounted
{
    __DeclareSingleton(NglTFScene);
    __DeclareClass(NglTFScene);
public:
    /// constructor
    NglTFScene();
    /// destructor
    virtual ~NglTFScene();
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
    void Setup(const Gltf::Document* scene, const ToolkitUtil::ExportFlags& exportFlags, const ToolkitUtil::ExportMode& exportMode, float scale);
    /// clears the scene, do this when all parsing is done
    void Cleanup();
        
    /// gets the current scene
    const Gltf::Document* GetScene() const;
    /// get the scale of the scene
    const float GetScale() const;
    /// converts export mode to string
    const Util::String GetSceneFeatureString();
        
    /// returns general node
    NglTFNode* GetNode(Gltf::Node const* key);
    /// returns true if general node is registered
    bool HasNode(Gltf::Node const* key);
    /// returns reference to full list of nodes
    const Util::Dictionary<Gltf::Node const*, Ptr<NglTFNode> >& GetNodes();
    
    /// returns mesh node
    NglTFMesh* GetMeshNode(Gltf::Mesh const* key);
    /// returns true if mesh node is registered
    bool HasMeshNode(Gltf::Mesh const* key);
    /// returns reference to dictionary of mesh nodes
    const Util::Array<Ptr<NglTFMesh> > GetMeshNodes() const;
    /// removes mesh node from main node dictionary and mesh node dictionary
    void RemoveMeshNode(const Ptr<NglTFMesh>& node);
    
    ExportMode GetExportMode() const;
    ExportFlags GetExportFlags() const;
        
    /// flattens all hierarchical structures except for joints and lights, and join meshes into one builder
    void Flatten();
    /// returns mesh for scene
    ToolkitUtil::MeshBuilder* GetMesh() const;
    
private:
    void ExtractMeshNodes(const Gltf::Node* node, Math::mat4 parentTransform);
    ///Gathers all mesh nodes and adds them to meshnodes and nodes
    void GatherMeshNodes();

    /*
        GLTF meshes are categories of primitives, primitives are stored as NglTFPrimitiveGroup.
        We then merge all the primitives into one huge mesh, to reduce drawcalls

        We can merge all primitive groups that has the same material into one if we choose.
    */

    Util::Dictionary<Gltf::Mesh const*, Ptr<NglTFMesh> > meshNodes;
    Util::Dictionary<Gltf::Node const*, Ptr<NglTFNode> > nodes;
    
    ToolkitUtil::MeshBuilder* mesh;
    ToolkitUtil::MeshBuilder* physicsMesh;
    
    Gltf::Document const* scene;
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
NglTFScene::IsOpen() const
{
    return this->isOpen;
}


//------------------------------------------------------------------------------
/**
*/
inline Gltf::Document const*
NglTFScene::GetScene() const
{
    return this->scene;
}


//------------------------------------------------------------------------------
/**
*/
inline void
NglTFScene::SetName(const Util::String& name)
{
    n_assert(name.IsValid())
        this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
NglTFScene::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFScene::SetCategory(const Util::String& category)
{
    n_assert(category.IsValid());
    this->category = category;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
NglTFScene::GetCategory() const
{
    return this->category;
}


//------------------------------------------------------------------------------
/**
*/
inline ToolkitUtil::MeshBuilder*
NglTFScene::GetMesh() const
{
    return this->mesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const float
NglTFScene::GetScale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Gltf::Node const*, Ptr<NglTFNode> >&
NglTFScene::GetNodes()
{
    return this->nodes;
}

//------------------------------------------------------------------------------
/**
*/
inline ExportFlags
NglTFScene::GetExportFlags() const
{
    return this->flags;
}

//------------------------------------------------------------------------------
/**
*/
inline ExportMode
NglTFScene::GetExportMode() const
{
    return this->mode;
}


} // namespace ToolkitUtil
//------------------------------------------------------------------------------