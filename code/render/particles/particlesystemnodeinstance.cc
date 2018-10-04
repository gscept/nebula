//------------------------------------------------------------------------------
//  particlesystemmaterialnodeinstance.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particlesystemnodeinstance.h"
#include "particles/particlesystemnode.h"
#include "coregraphics/transformdevice.h"
#include "particles/particlerenderer.h"
#include "coregraphics/shadersemantics.h"
#include "models/model.h"
#include "models/modelnodeinstance.h"
#include "models/modelinstance.h"
#include "graphics/modelentity.h"
#include "coregraphics/shaderserver.h"
#include "particleserver.h"
#include "resources/resourcemanager.h"

namespace Particles
{
__ImplementClass(Particles::ParticleSystemNodeInstance, 'PSNI', Models::StateNodeInstance);

using namespace Models;
using namespace Util;
using namespace CoreGraphics;
using namespace Math;
using namespace Materials;
using namespace Graphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
ParticleSystemNodeInstance::ParticleSystemNodeInstance()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ParticleSystemNodeInstance::~ParticleSystemNodeInstance()
{
    n_assert(!this->particleSystemInstance.isvalid());
}    

//------------------------------------------------------------------------------
/**
*/
void 
ParticleSystemNodeInstance::OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distanceToViewer)
{
    // check if node is inside lod distances or if no lod is used
    const Ptr<TransformNode>& transformNode = this->modelNode.downcast<TransformNode>();
    if (transformNode->CheckLodDistance(distanceToViewer))
    {
		this->modelNode->AddVisibleNodeInstance(resolveIndex, this->surfaceInstance->GetCode(), this);
        ModelNodeInstance::OnVisibilityResolve(frameIndex, resolveIndex, distanceToViewer);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst)
{
    n_assert(!this->particleSystemInstance.isvalid());

    // up to parent class
    StateNodeInstance::Setup(inst, node, parentNodeInst);

    // setup a new particle system instance
    this->particleSystemInstance = ParticleSystemInstance::Create();
    const Ptr<ParticleSystemNode>& particleSystemNode = node.downcast<ParticleSystemNode>();

	// get mesh
	Ptr<Mesh> emitterMesh;
	if (particleSystemNode->GetEmitterMesh().isvalid())
	{
		emitterMesh = particleSystemNode->GetEmitterMesh()->GetMesh();
	}
	else
	{
		emitterMesh = ParticleServer::Instance()->GetDefaultEmitterMesh();
	}

	// setup and start particle system
    this->particleSystemInstance->Setup(emitterMesh, particleSystemNode->GetPrimitiveGroupIndex(), particleSystemNode->GetEmitterAttrs());
	this->particleSystemInstance->Start();

#if SHADER_MODEL_5
	ShaderServer* shdServer = ShaderServer::Instance();
	this->particleShader = shdServer->CreateShaderState("shd:particle", { NEBULA_SYSTEM_GROUP });
	this->emitterOrientationVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_EMITTERTRANSFORM);
	this->billBoardVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_BILLBOARD);
	this->bboxCenterVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_BBOXCENTER);
	this->bboxSizeVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_BBOXSIZE);
	this->animPhasesVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_ANIMPHASES);
	this->animsPerSecVar = this->particleShader->GetVariableByName(NEBULA_SEMANTIC_ANIMSPERSEC);
#else
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_EMITTERTRANSFORM))
	{
		this->emitterOrientation = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_EMITTERTRANSFORM);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BILLBOARD))
	{
		this->billBoard = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BILLBOARD);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BBOXCENTER))
	{
		this->bboxCenter = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BBOXCENTER);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BBOXSIZE))
	{
		this->bboxSize = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BBOXSIZE);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_TIME))
	{
		this->time = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_TIME);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_ANIMPHASES))
	{
		this->animPhases = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_ANIMPHASES);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_ANIMSPERSEC))
	{
		this->animsPerSec = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_ANIMSPERSEC);
	}
#endif

    if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_DEPTHBUFFER))
	{
		Ptr<Texture> depthTexture = Resources::ResourceManager::Instance()->CreateUnmanagedResource("DepthBuffer", Texture::RTTI).downcast<Texture>();
		this->depthBuffer = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_DEPTHBUFFER);
		this->depthBuffer->SetTexture(depthTexture);
	}

#if __PS3__
    const Util::Array<Util::KeyValuePair<Util::StringAtom, Util::Variant> > &shaderParams = particleSystemNode->GetShaderParameter();
    IndexT i;
    int numAnimPhases = -1;
    float animFramesPerSecond = -1.0f;
    for (i = 0; i < shaderParams.Size(); i++)
    {
        if(shaderParams[i].Key() == "ALPHAREF")
        {
            numAnimPhases = shaderParams[i].Value().GetInt();
        }
        else if(shaderParams[i].Key() == "INTENSITY1")
        {
            animFramesPerSecond = shaderParams[i].Value().GetFloat();
        }
    }
    n_assert(-1 != numAnimPhases);
    n_assert(-1.0f != animFramesPerSecond);
    this->particleSystemInstance->SetNumAnimPhases(numAnimPhases);
    this->particleSystemInstance->SetAnimFramesPerSecond(animFramesPerSecond);
#endif

}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::Discard()
{
    n_assert(this->particleSystemInstance.isvalid());

    // discard material clone
	//this->surfaceClone->Unload();
	this->particleShader = 0;
	this->emitterOrientationVar = 0;
	this->billBoardVar = 0;
	this->bboxCenterVar = 0;
	this->bboxSizeVar = 0;
	this->animPhasesVar = 0;
	this->animsPerSecVar = 0;
    this->depthBuffer = 0;

    // discard our particle system instance
    this->particleSystemInstance->Discard();
    this->particleSystemInstance = 0;

    // up to parent-class
    StateNodeInstance::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::OnRenderBefore(IndexT frameIndex, Timing::Time time)
{
    // call parent class
    StateNodeInstance::OnRenderBefore(frameIndex, time);    
    this->particleSystemInstance->OnRenderBefore();

    // update particle system with new model transform
    this->particleSystemInstance->SetTransform(this->modelTransform);

    // updating happens in 2 stages:
    // 1) within activity distance: particles are emitted and updated
    // 2) in view volume: particle system is added for rendering
    const point& eyePos = TransformDevice::Instance()->GetInvViewTransform().get_position();
    const point& myPos  = this->modelTransform.get_position();
    float dist = float4(myPos - eyePos).length();
    float activityDist = this->particleSystemInstance->GetParticleSystem()->GetEmitterAttrs().GetFloat(EmitterAttrs::ActivityDistance);
    if (dist <= activityDist)
    {
        // alright, we're within the activity distance, update the particle system
        this->particleSystemInstance->Update(time);

        // check if we're also in the view volume, and if yes, 
        // register the particle system for rendering
        // FIXME: use actual particle bounding box!!!
        const bbox& localBox = this->particleSystemInstance->GetBoundingBox();
		const Ptr<Graphics::ModelEntity>& modelEntity = this->modelInstance->GetModelEntity();
		modelEntity->ExtendLocalBoundingBox(localBox);
		
        /*
        // get model entity so that we can retrieve its local bounding box
        const Ptr<Graphics::ModelEntity>& modelEntity = this->modelInstance->GetModelEntity();

        // get transform from model
        // inverse transform
        // now apply to the global bounding box of the particles, the particles should now be in local space
        matrix44 trans = modelEntity->GetTransform();
        trans = matrix44::inverse(trans);
        bbox localBox = modelEntity->GetLocalBoundingBox();
        bbox localParticleBox = globalBox;
        localParticleBox.transform(trans);

        // extend with local particle box and update model entity local bounding box
        localBox.extend(localParticleBox);
        modelEntity->SetLocalBoundingBox(localBox);
        */
		
        const matrix44& viewProj = TransformDevice::Instance()->GetViewProjTransform();
		if (ClipStatus::Outside != localBox.clipstatus(viewProj))
        {
            // yes, we're visible
            ParticleRenderer::Instance()->AddVisibleParticleSystem(this->particleSystemInstance);
        }
        else
        {
            // FIXME DEBUG
            // n_printf("%f: Particle system invisible (clip) %s!\n", time, this->modelNode->GetName().Value());
        }
    }
    else
    {
        // FIXME DEBUG
        //n_printf("Particle system invisible (activity dist) %s!\n", this->modelNode->GetName().Value());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::ApplyState(IndexT frameIndex, const IndexT& pass)
{
	const Ptr<Particles::ParticleSystemInstance>& inst = this->GetParticleSystemInstance();
	const Ptr<Particles::ParticleSystem>& system = inst->GetParticleSystem();
	bool billboard = system->GetEmitterAttrs().GetBool(EmitterAttrs::Billboard);
#if SHADER_MODEL_5
	// avoid shuffling buffers if we are in the same frame
	if (this->particleObjectBufferIndex != frameIndex)
	{
		// apply transforms
		if (billboard)
		{
			const Math::matrix44 billboardTransform = Math::matrix44::multiply(this->GetParticleSystemInstance()->GetTransform(), TransformDevice::Instance()->GetInvViewTransform());
			this->emitterOrientationVar->SetMatrix(billboardTransform);
		}
		else
		{
			this->emitterOrientationVar->SetMatrix(this->GetParticleSystemInstance()->GetTransform());
		}
		
		this->bboxCenterVar->SetFloat4(inst->GetBoundingBox().center());
		this->bboxSizeVar->SetFloat4(inst->GetBoundingBox().extents());
		this->animPhasesVar->SetInt(system->GetEmitterAttrs().GetInt(EmitterAttrs::AnimPhases));
		this->animsPerSecVar->SetFloat(system->GetEmitterAttrs().GetFloat(EmitterAttrs::PhasesPerSecond));
		this->billBoardVar->SetBool(billboard);
		this->particleObjectBufferIndex = frameIndex;
	}
	this->particleShader->Commit();
#else
	// set variables
	if (billboard)
	{
		// use inverse view matrix for orientation 
		this->emitterOrientationVar->SetMatrix(TransformDevice::Instance()->GetInvViewTransform());
	}
	else
	{
		// otherwise, use the matrix of the particle system
		this->emitterOrientationVar->SetMatrix(this->GetParticleSystemInstance()->GetTransform());
	}

	//billBoard->SetBool(this->GetParticleSystemInstance()->GetParticleSystem()->GetEmitterAttrs().GetBool(EmitterAttrs::Billboard));	
	//this->bboxCenter->SetFloat4(this->GetParticleSystemInstance()->GetBoundingBox().center());
	//this->bboxSize->SetFloat4(this->GetParticleSystemInstance()->GetBoundingBox().extents());
	this->animPhasesVar->SetInt(this->GetParticleSystemInstance()->GetParticleSystem()->GetEmitterAttrs().GetInt(EmitterAttrs::AnimPhases));
	this->animsPerSecVar->SetInt(this->GetParticleSystemInstance()->GetParticleSystem()->GetEmitterAttrs().GetFloat(EmitterAttrs::PhasesPerSecond));
#endif

	// call base class (applies time)
    StateNodeInstance::ApplyState(frameIndex, pass);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleSystemNodeInstance::Render()
{
	// call base class
    StateNodeInstance::Render();

	// render particle system
    ParticleRenderer::Instance()->RenderParticleSystem(this->particleSystemInstance);
}

//------------------------------------------------------------------------------
/**
	Just render as normal, and assume the shader can handle the instancing
*/
void
ParticleSystemNodeInstance::RenderInstanced(SizeT numInstances)
{
	StateNodeInstance::RenderInstanced(numInstances);
	ParticleRenderer::Instance()->RenderParticleSystem(this->particleSystemInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::RenderDebug()
{
    StateNodeInstance::RenderDebug();
    this->particleSystemInstance->RenderDebug();
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::OnShow(Timing::Time time)
{
    this->particleSystemInstance->Start();
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::OnHide(Timing::Time time)
{
    // FIXME: should stop immediately?
    if (this->particleSystemInstance->IsPlaying())
    {
        this->particleSystemInstance->Stop();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNodeInstance::SetSurfaceInstance(const Ptr<Materials::SurfaceInstance>& material)
{
	StateNodeInstance::SetSurfaceInstance(material);

#if !SHADER_MODEL_5
	// setup variables again
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_EMITTERTRANSFORM))
	{
		this->emitterOrientation = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_EMITTERTRANSFORM);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BILLBOARD))
	{
		this->billBoard = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BILLBOARD);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BBOXCENTER))
	{
		this->bboxCenter = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BBOXCENTER);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_BBOXSIZE))
	{
		this->bboxSize = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_BBOXSIZE);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_TIME))
	{
		this->time = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_TIME);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_ANIMPHASES))
	{
		this->animPhases = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_ANIMPHASES);
	}
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_ANIMSPERSEC))
	{
		this->animsPerSec = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_ANIMSPERSEC);
	}

#endif
	if (this->surfaceInstance->HasConstant(NEBULA_SEMANTIC_DEPTHBUFFER))
	{
		Ptr<Texture> depthTexture = Resources::ResourceManager::Instance()->CreateUnmanagedResource("DepthBuffer", Texture::RTTI).downcast<Texture>();
		this->depthBuffer = this->surfaceInstance->GetConstant(NEBULA_SEMANTIC_DEPTHBUFFER);
		this->depthBuffer->SetTexture(depthTexture);
	}

}

} // namespace Particles
