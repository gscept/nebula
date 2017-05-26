//------------------------------------------------------------------------------
//  d3d11instanceserver.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "d3d11instanceserver.h"
#include "d3d11instancerenderer.h"
#include "models/nodes/transformnodeinstance.h"
#include "framesync/framesynctimer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"

using namespace Util;
using namespace Direct3D11;
using namespace CoreGraphics;
using namespace Models;
namespace Instancing
{
__ImplementSingleton(Instancing::D3D11InstanceServer);
__ImplementClass(Instancing::D3D11InstanceServer, 'D1IS', Instancing::InstanceServerBase);
//------------------------------------------------------------------------------
/**
*/
D3D11InstanceServer::D3D11InstanceServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11InstanceServer::~D3D11InstanceServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool 
D3D11InstanceServer::Open()
{
	if (InstanceServerBase::Open())
	{
		this->renderer = D3D11InstanceRenderer::Create();
		this->instancingFeatureBits = ShaderServer::Instance()->FeatureStringToMask("Instanced");
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11InstanceServer::Close()
{
	this->renderer = 0;
	InstanceServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11InstanceServer::Render()
{
	n_assert(this->IsOpen());
	n_assert(this->modelNode.isvalid());
	n_assert(this->renderer.isvalid());

	// get render device
	RenderDevice* renderDev = RenderDevice::Instance();
	ShaderServer* shaderServer = ShaderServer::Instance();

	// get frame index
	IndexT frameIndex = FrameSync::FrameSyncTimer::Instance()->GetFrameIndex();

	// abort early if we don't have any instances
	if (this->instancesByCode.Size() == 0)
	{
		return;
	}

	// set shader in renderer
	const Ptr<ShaderInstance>& shader = shaderServer->GetActiveShaderInstance();
	this->renderer->SetShader(shader);

	// get string of active variation
	shaderServer->SetFeatureBits(this->instancingFeatureBits);
	shader->SelectActiveVariation(shaderServer->GetFeatureBits());

	// begin pass of shader
	shader->Begin();
	shader->BeginPass(0);

	// go through each code instance, update transforms in batches based on their code
	IndexT codeIndex;
	for (codeIndex = 0; codeIndex < this->instancesByCode.Size(); codeIndex++)
	{
		// get array of nodes
		const Array<Ptr<ModelNodeInstance> >& codeInstances = this->instancesByCode.ValueAtIndex(codeIndex);

		// start by activating renderer
		this->renderer->BeginUpdateTransforms(codeInstances.Size());

		// go through each model instance and add it to renderer
		IndexT i;
		for (i = 0; i < codeInstances.Size(); i++)
		{
			// get node
			const Ptr<ModelNodeInstance>& nodeInstance = codeInstances[i];
			
			// upcast to transform node
			Ptr<TransformNodeInstance> transNode = nodeInstance.downcast<TransformNodeInstance>();

			// add transform to renderer
			this->renderer->UpdateTransforms(transNode->GetModelTransform());		
		}

		// finish updating transforms
		this->renderer->EndUpdateTransforms();

		// apply the state for the first node, this will then be active for all of them
		codeInstances[0]->ApplyState();

		// apply wireframe if needed
		if (renderDev->GetRenderWireframe())
		{
			renderDev->ApplyWireframe();
		}

		// now render
		this->renderer->Render();
	}

	// end pass of shader
	shader->EndPass();
	shader->End();
}

} // namespace Instancing
