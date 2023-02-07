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

private:
    friend class NFbxNode;
    friend class NFbxJointNode;
    friend class NFbxLightNode;
    friend class NFbxMeshNode;
    friend class NFbxTransformNode;
    friend class NFbxScene;

    friend class NglTFScene;
    friend class NglTFNode;
    friend class NglTFMesh;

    friend class SceneWriter;
    friend class Scene;
    
    friend class ModelExporter;
    friend void SetupPrimitiveGroupJobFunc(Jobs::JobFuncContext const& context);

    NodeType type;
    struct
    {
        Util::String                    name;

        Math::quat                      rotation;
        Math::vec3                      position;
        Math::vec3                      scale;
        Math::mat4                      transform;
        Math::point                     pivot;
        Math::bbox                      boundingBox;

        bool                            isPhysics = false;
        bool                            isAnimated = false;
        bool                            isRoot = false;
        bool                            isSkin = false;

        ToolkitUtil::ExportFlags        exportFlags;

        Util::Array<SceneNode*>         children;
        SceneNode*                      parent;
    } base;

    
    struct
    {
        Util::String take;
        int span;
        ToolkitUtil::AnimBuilderCurve translationCurve, rotationCurve, scaleCurve;
        CoreAnimation::InfinityType::Code preInfinity, postInfinity;
        IndexT animIndex = InvalidIndex;
    } anim;


    struct
    {
        Math::mat4                  globalMatrix;
        bool                        matrixIsGlobal = false;
        bool                        isSkeletonRoot = false;
        IndexT                      parentIndex = InvalidIndex;
        IndexT                      jointIndex = InvalidIndex;
        IndexT                      skeletonIndex = InvalidIndex;

        IndexT                      childSetupCounter = 0;
    } skeleton;

    struct
    {
        Util::Array<IndexT> skinFragments;
        Util::Array<Util::Array<IndexT>> jointLookup;
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
};
} // namespace ToolkitUtil
