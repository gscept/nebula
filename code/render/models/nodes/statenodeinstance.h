#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::MaterialStateNodeInstance

    Holds and applies per-instance shader states for models with materials attached
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/config.h"
#include "models/nodes/transformnodeinstance.h"
#include "materials/materialvariable.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "materials/surfaceinstance.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/constantbuffer.h"

//#define STATE_NODE_USE_PER_OBJECT_BUFFER
//------------------------------------------------------------------------------
namespace Models
{
class StateNodeInstance : public TransformNodeInstance
{
    __DeclareClass(StateNodeInstance);
public:
    /// constructor
    StateNodeInstance();
    /// destructor
    virtual ~StateNodeInstance();

	/// apply per-instance state for a shader prior to rendering
	virtual void ApplyState(IndexT frameIndex, const IndexT& pass);

    /// set surface material on node, be careful to clear up any references to SurfaceConstantInstances if this is done
    virtual void SetSurfaceInstance(const Ptr<Materials::SurfaceInstance>& material);
    /// get surface on node
    const Ptr<Materials::SurfaceInstance>& GetSurfaceInstance() const;

protected:

	/// setup state node
	virtual void Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst);
    /// called when removed from ModelInstance
    virtual void Discard();
	/// applies global variables
	virtual void ApplySharedVariables();

#if SHADER_MODEL_5
    Ptr<CoreGraphics::ShaderState> sharedShader;
    Ptr<CoreGraphics::ShaderVariable> modelShaderVar;
    Ptr<CoreGraphics::ShaderVariable> invModelShaderVar;
    Ptr<CoreGraphics::ShaderVariable> modelViewProjShaderVar;
    Ptr<CoreGraphics::ShaderVariable> modelViewShaderVar;
    Ptr<CoreGraphics::ShaderVariable> objectIdShaderVar;
    IndexT objectBufferUpdateIndex;
#endif

    Ptr<Materials::SurfaceInstance> surfaceInstance;
    Util::Dictionary<Util::StringAtom, Ptr<Materials::SurfaceConstant>> sharedConstants;
    //Util::Dictionary<Util::StringAtom, Ptr<Materials::SurfaceConstantInstance>> surfaceConstantInstanceByName;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Materials::SurfaceInstance>&
StateNodeInstance::GetSurfaceInstance() const
{
    return this->surfaceInstance;
}

} // namespace Models
//------------------------------------------------------------------------------
