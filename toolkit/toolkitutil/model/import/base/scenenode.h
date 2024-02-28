#pragma once
//------------------------------------------------------------------------------
/**
    Internal node representation

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/quat.h"
#include "math/vec4.h"
#include "util/string.h"
#include "model/animutil/animbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "math/bbox.h"
#include "model/meshutil/meshbuilder.h"
#include "jobs/jobs.h"
#include "util/set.h"

struct ufbx_node;
namespace ToolkitUtil
{

class SkinFragment;
class SceneNode
{
public:
    enum class NodeType
    {
        Mesh,           // actual mesh node
        Joint,          // joint transform node
        Transform,      // plain transform node
        Light,          // light node
        Lod,            // level of detail node

        Locator,        // locator node (unused)
        Effector,       // effector node (unused)

        UnknownType,    // unknown type, unasserted

        NumNodeTypes
    };

    enum LightType
    {
        Spotlight,
        Pointlight,
        AreaLight,
        InvalidLight,

        NumLights
    };

    /// Setup 
    void Setup(const NodeType type);

    /// Calculate global transforms recursively
    void CalculateGlobalTransforms();

private:
#ifdef FBXSDK
    friend class NFbxNode;
    friend class NFbxJointNode;
    friend class NFbxLightNode;
    friend class NFbxMeshNode;
    friend class NFbxTransformNode;
    friend class NFbxScene;
#endif

    friend class NglTFScene;
    friend class NglTFNode;
    friend class NglTFMesh;

    friend class SceneWriter;
    friend class Scene;
    
    friend class ModelExporter;
    friend void MeshPrimitiveFunc(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

    NodeType type;
    struct
    {
        Util::String                    name;

        Math::quat                      rotation;
        Math::vec3                      translation;
        Math::vec3                      scale;
        Math::point                     pivot;
        Math::bbox                      boundingBox;

        Math::mat4                      globalTransform;

        bool                            isPhysics = false;
        bool                            isAnimated = false;
        bool                            isRoot = false;
        bool                            isSkin = false;

        ToolkitUtil::ExportFlags        exportFlags;

        Util::Array<SceneNode*>         children;
        SceneNode*                      parent = nullptr;
    } base;

    
    struct
    {
        ToolkitUtil::AnimBuilderCurve translationCurve, rotationCurve, scaleCurve;
        IndexT animIndex = InvalidIndex;
    } anim;


    struct
    {
        Math::mat4                  bindMatrix;
        bool                        isSkeletonRoot = false;
        IndexT                      parentIndex = InvalidIndex;
        IndexT                      jointIndex = InvalidIndex;
        IndexT                      skeletonIndex = InvalidIndex;
    } skeleton;

    struct
    {
        Util::Array<IndexT> skinFragments;
        Util::Array<Util::Set<IndexT>> jointLookup;
    } skin;

    struct
    {
        LightType                   lightType;
    } light;

    struct
    {
        float                                           minLodDistance;
        float                                           maxLodDistance;
        ToolkitUtil::MeshFlags                          meshFlags;
        IndexT                                          groupId = InvalidIndex;
        Util::String                                    material;
        IndexT                                          lodIndex = InvalidIndex;
        IndexT                                          meshIndex = InvalidIndex;
        //ToolkitUtil::MeshBuilderVertex::ComponentMask   components;
        //ToolkitUtil::MeshBuilder                        mesh;
    } mesh;

#ifdef FBXSDK
    struct
    {
        ufbx_node* node;
        Util::Set<double> translationKeyTimes, rotationKeyTimes, scaleKeyTimes;
    } fbx;
#endif
};
} // namespace ToolkitUtil
