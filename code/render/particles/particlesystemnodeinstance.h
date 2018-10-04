#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNodeInstance
    
    Wraps a ParticleSystemInstance into a ModelNodeInstance using materials.
    
    (C) 2011-2018 Individual contributors, see AUTHORS file
*/
#include "models/nodes/statenodeinstance.h"
#include "particles/particlesysteminstance.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shaderstate.h"
#include "materials/materialvariable.h"

//------------------------------------------------------------------------------
namespace Particles
{
class ParticleSystemNodeInstance : public Models::StateNodeInstance
{
    __DeclareClass(ParticleSystemNodeInstance);
public:
    /// constructor
    ParticleSystemNodeInstance();
    /// destructor
    virtual ~ParticleSystemNodeInstance();

    /// called from ModelEntity::OnRenderBefore
    virtual void OnRenderBefore(IndexT frameIndex, Timing::Time time);
    /// called during visibility resolve
	virtual void OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distanceToViewer);
    /// apply per-instance state prior to rendering
	virtual void ApplyState(IndexT frameIndex, const IndexT& pass);
    /// perform rendering
    virtual void Render();
	/// perform instanced rendering
	virtual void RenderInstanced(SizeT numInstances);

	/// set surface instance
	virtual void SetSurfaceInstance(const Ptr<Materials::SurfaceInstance>& material);

    /// get the node's particle system instance
    const Ptr<ParticleSystemInstance>& GetParticleSystemInstance() const;

protected:
    /// called when attached to ModelInstance
    virtual void Setup(const Ptr<Models::ModelInstance>& inst, const Ptr<Models::ModelNode>& node, const Ptr<Models::ModelNodeInstance>& parentNodeInst);
    /// called when removed from ModelInstance
    virtual void Discard();
    /// render node specific debug shape
    virtual void RenderDebug();
    /// called when the node becomes visible with current time
    virtual void OnShow(Timing::Time time);
    /// called when the node becomes invisible
    virtual void OnHide(Timing::Time time);

    Ptr<ParticleSystemInstance> particleSystemInstance;    

#if SHADER_MODEL_5
	Ptr<CoreGraphics::ShaderState> particleShader;
	Ptr<CoreGraphics::ShaderVariable> emitterOrientationVar;
	Ptr<CoreGraphics::ShaderVariable> billBoardVar;
	Ptr<CoreGraphics::ShaderVariable> bboxCenterVar;
	Ptr<CoreGraphics::ShaderVariable> bboxSizeVar;
	Ptr<CoreGraphics::ShaderVariable> animPhasesVar;
	Ptr<CoreGraphics::ShaderVariable> animsPerSecVar;
	IndexT particleObjectBufferIndex;
#else
	Ptr<Materials::SurfaceConstant> emitterOrientation;
	Ptr<Materials::SurfaceConstant> billBoard;
	Ptr<Materials::SurfaceConstant> bboxCenter;
	Ptr<Materials::SurfaceConstant> bboxSize;
	Ptr<Materials::SurfaceConstant> time;
	Ptr<Materials::SurfaceConstant> animPhases;
	Ptr<Materials::SurfaceConstant> animsPerSec;
#endif


    Ptr<Materials::SurfaceConstant> depthBuffer;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ParticleSystemInstance>&
ParticleSystemNodeInstance::GetParticleSystemInstance() const
{
    return this->particleSystemInstance;
}

} // namespace Particles
//------------------------------------------------------------------------------
    