#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFNode

    Encapsulates an gltf node as a Nebula-friendly object

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/quat.h"
#include "math/vec4.h"
#include "model/animutil/animbuilderclip.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "model/animutil/animbuilder.h"
#include "model/modelutil/modelattributes.h"
#include "model/import/gltf/gltfdata.h"
#include "core/weakptr.h"

#define KEYS_PER_MS 40

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class SceneNode;
class NglTFScene;
class NglTFMesh;
class NglTFNode : public Core::RefCounted
{
    __DeclareClass(NglTFNode);
public:
    /// constructor
    NglTFNode();
    /// destructor
    virtual ~NglTFNode();

    static void Setup(const Gltf::Node* gltfNode, SceneNode* node, SceneNode* parent);
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------