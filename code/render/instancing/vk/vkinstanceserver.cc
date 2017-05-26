//------------------------------------------------------------------------------
// vkinstanceserver.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkinstanceserver.h"
#include "instancing/instancerenderer.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"
#include "models/nodes/transformnodeinstance.h"
#include "models/modelinstance.h"

using namespace Util;
using namespace CoreGraphics;
using namespace Instancing;
using namespace Models;
namespace Vulkan
{

__ImplementSingleton(Vulkan::VkInstanceServer);
__ImplementClass(Vulkan::VkInstanceServer, 'VKIS', Base::InstanceServerBase);
//------------------------------------------------------------------------------
/**
*/
VkInstanceServer::VkInstanceServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkInstanceServer::~VkInstanceServer()
{
	__DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
bool
VkInstanceServer::Open()
{
	if (InstanceServerBase::Open())
	{
		this->renderer = InstanceRenderer::Create();
		this->renderer->Setup();
		this->instancingFeatureBits = ShaderServer::Instance()->FeatureStringToMask("Instanced");
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
VkInstanceServer::Close()
{
	this->renderer->Close();
	this->renderer = 0;
	InstanceServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkInstanceServer::Render(IndexT frameIndex)
{
	n_assert(this->IsOpen());
	n_assert(this->modelNode.isvalid());
	n_assert(this->renderer.isvalid());

	// get render device
	RenderDevice* renderDev = RenderDevice::Instance();
	ShaderServer* shaderServer = ShaderServer::Instance();

	// abort early if we don't have any instances
	if (this->instancesByCode.Size() == 0)
	{
		return;
	}

	// set shader in renderer
	this->renderer->SetShader(this->shader);

	// get string of active variation
	shaderServer->SetFeatureBits(this->instancingFeatureBits);
	this->shader->SelectActiveVariation(shaderServer->GetFeatureBits());

	// begin pass of shader
	if (this->instancesByCode.Size() > 0) this->shader->Apply();

	// go through each code instance, update transforms in batches based on their code
	IndexT codeIndex;
	for (codeIndex = 0; codeIndex < this->instancesByCode.Size(); codeIndex++)
	{
		// get array of nodes
		const Array<Ptr<ModelNodeInstance> >& nodeInstances = this->instancesByCode.ValueAtIndex(codeIndex);

		// start by activating renderer
		this->renderer->BeginUpdate(nodeInstances.Size());

		// go through each model instance and add it to renderer
		IndexT i;
		for (i = 0; i < nodeInstances.Size(); i++)
		{
			// get node
			const Ptr<ModelNodeInstance>& nodeInstance = nodeInstances[i];

			// upcast to transform node
			Ptr<TransformNodeInstance> transNode = nodeInstance.downcast<TransformNodeInstance>();

			// add transform to renderer
			this->renderer->AddTransform(transNode->GetModelTransform());
			this->renderer->AddId(transNode->GetModelInstance()->GetPickingId());
		}

		// finish updating transforms
		this->renderer->EndUpdate();

		// apply the state for the first node, this will then be active for all of them
		nodeInstances[0]->ApplyState(frameIndex, this->pass);

		// now render
		this->renderer->Render(this->multiplier);
	}
}

} // namespace Vulkan