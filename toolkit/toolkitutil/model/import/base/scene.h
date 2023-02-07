#pragma once
//------------------------------------------------------------------------------
/**
    Scene base

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "scenenode.h"
#include "toolkit-common/base/exporttypes.h"
namespace ToolkitUtil
{

extern int KeysPerMS;
extern float SceneScale;
extern float AdjustedScale;
class ModelAttributes;
struct SkeletonBuilder;
class MeshBuilder;
class Scene
{
public:

    /// constructor
    Scene();
    /// destructor
    virtual ~Scene();

    /// sets the name of the scene
    void SetName(const Util::String& name);
    /// gets the name of the scene
    const Util::String& GetName() const;
    /// sets the category for the scene
    void SetCategory(const Util::String& category);
    /// gets the category for the scene
    const Util::String& GetCategory() const;

    /// Merges the scene into a set of nodes and their corresponding primitive groups, and a set of meshes
    void OptimizeGraphics(Util::Array<SceneNode*>& outMeshNodes, Util::Array<SceneNode*>& outCharacterNodes, Util::Array<MeshBuilderGroup>& outGroups, Util::Array<MeshBuilder*>& outMeshes);
    /// Merges physics nodes and meshes into a single mesh builder and a set of nodes
    void OptimizePhysics(Util::Array<SceneNode*>& outNodes, MeshBuilder*& outMesh);

    /// Get nodes
    const Util::Array<SceneNode>& GetNodes() const;

protected:
    friend class ModelExporter;

    /// Splits animation curves, returns true if splitter has any rules for this take
    /// Operates on the root node of an animated node hierarchy
    static bool SplitAnimationCurves(SceneNode* node, const Util::Array<AnimBuilderCurve>& curves, ToolkitUtil::AnimBuilder& anim, const Ptr<ToolkitUtil::ModelAttributes>& attributes);

    Util::Array<MeshBuilder> meshes;
    Util::Array<SkeletonBuilder> skeletons;
    Util::Array<AnimBuilder> animations;

    Util::Array<SceneNode> nodes;
    Util::String name;
    Util::String category;
    ToolkitUtil::ExportFlags flags;
    float scale;
};


//------------------------------------------------------------------------------
/**
*/
inline void
Scene::SetName(const Util::String& name)
{
    n_assert(name.IsValid());
    this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Scene::GetName() const
{
    return this->name;
}


//------------------------------------------------------------------------------
/**
*/
inline void
Scene::SetCategory(const Util::String& category)
{
    n_assert(category.IsValid());
    this->category = category;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Scene::GetCategory() const
{
    return this->category;
}

//------------------------------------------------------------------------------
/**
*/
inline const 
Util::Array<SceneNode>& Scene::GetNodes() const
{
    return this->nodes;
}

} // namespace ToolkitUtil
