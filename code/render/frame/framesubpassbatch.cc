//------------------------------------------------------------------------------
// framesubpassbatch.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassbatch.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "materials/materialserver.h"
#include "models/model.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "visibility/visibilitycontext.h"

using namespace Graphics;
using namespace CoreGraphics;
using namespace Materials;
using namespace Models;
using namespace Util;

#define NEBULA3_FRAME_LOG_ENABLED   (0)
#if NEBULA3_FRAME_LOG_ENABLED
#define FRAME_LOG(...) n_printf(__VA_ARGS__); n_printf("\n")
#else
#define FRAME_LOG(...)
#endif

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassBatch::FrameSubpassBatch()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassBatch::~FrameSubpassBatch()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubpassBatch::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->batch = this->batch;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::CompiledImpl::Run(const IndexT frameIndex)
{
	// now do usual render stuff
	ShaderServer* shaderServer = ShaderServer::Instance();
	MaterialServer* matServer = MaterialServer::Instance();

	// get current view and visibility draw list
	const Ptr<View>& view = Graphics::GraphicsServer::Instance()->GetCurrentView();
	const Visibility::ObserverContext::VisibilityDrawList& drawList = Visibility::ObserverContext::GetVisibilityDrawList(view->GetCamera());
	return;
	// start batch
	CoreGraphics::BeginBatch(FrameBatchType::Geometry);

	const Util::Array<MaterialType*>* types = matServer->GetMaterialTypesByBatch(this->batch);
	if (types != nullptr) for (IndexT typeIdx = 0; typeIdx < types->Size(); typeIdx++)
	{
		MaterialType* type = (*types)[typeIdx];
		if (MaterialBeginBatch(type, this->batch))
		{
			IndexT idx = drawList.FindIndex(type);
			if (idx != InvalidIndex)
			{
				auto& model = drawList.ValueAtIndex(type, idx);
				auto& it = model.Begin();
				auto& end = model.End();
				while (it != end)
				{
					Models::ModelNode* node = *it.key;

					const Util::Array<Models::ModelNode::Instance*>& instances = *it.val;
					if (instances.Size() > 0)
					{
						// apply node-wide state
						node->ApplyNodeState();

						IndexT i;
						for (i = 0; i < instances.Size(); i++)
						{
							Models::ModelNode::Instance* instance = instances[i];

							// apply instance state
							instance->ApplyNodeInstanceState();
							CoreGraphics::Draw();
						}
					}
					it++;
				}
			}
			MaterialEndBatch();
		}
	}
	/*

	if (matServer->HasMaterialsByBatchGroup(this->batch))
	{
		// get materials matching the batch type
		const Util::Array<Ptr<Material>>& materials = matServer->GetMaterialsByBatchGroup(this->batch);

		IndexT materialIndex;
		for (materialIndex = 0; materialIndex < materials.Size(); materialIndex++)
		{
			// get material
			const Ptr<Material>& material = materials[materialIndex];
			const Util::Array<Material::MaterialPass>& passes = material->GetPassesByCode(this->batch);

			IndexT passIndex;
			for (passIndex = 0; passIndex < passes.Size(); passIndex++)
			{
				// get pass
				const Material::MaterialPass& pass = passes[passIndex];
				const Ptr<Shader>& shader = pass.shader;
				const Util::Array<Ptr<Surface>>& surfaces = material->GetSurfaces();

				IndexT surfaceIndex;
				for (surfaceIndex = 0; surfaceIndex < surfaces.Size(); surfaceIndex++)
				{
					const Ptr<Surface>& surface = surfaces[surfaceIndex];

					// get models based on material, if we can't see any models, just ignore this surface...
					// hmm, would love to do this earlier so we can just skip unused materials
					const Array<Ptr<Model>>& models = visResolver->GetVisibleModels(surface->GetSurfaceCode());
					if (models.IsEmpty()) continue;

					// set the this shader to be the main active shader
					shaderServer->SetActiveShader(shader);

					// reset features, then set the features implemented by the material
					shaderServer->ResetFeatureBits();
					shaderServer->SetFeatureBits(pass.featureMask);

					// apply shared model state (mesh)
					//modelNode->ApplySharedState(frameIndex);

					// apply shader 
					shader->SelectActiveVariation(shaderServer->GetFeatureBits());
					shader->Apply();

					IndexT modelIndex;
					for (modelIndex = 0; modelIndex < models.Size(); modelIndex++)
					{
						FRAME_LOG("      FrameBatch::RenderBatch() model: %s", models[modelIndex]->GetResourceId().Value());

						// for each visible model node of the model...
						const Array<Ptr<ModelNode>>& modelNodes = visResolver->GetVisibleModelNodes(surface->GetSurfaceCode(), models[modelIndex]);
						IndexT modelNodeIndex;
						for (modelNodeIndex = 0; modelNodeIndex < modelNodes.Size(); modelNodeIndex++)
						{
							// render instances
							const Ptr<ModelNode>& modelNode = modelNodes[modelNodeIndex];
							const Array<Ptr<ModelNodeInstance>>& nodeInstances = visResolver->GetVisibleModelNodeInstances(surface->GetSurfaceCode(), modelNode);
							if (nodeInstances.IsEmpty()) continue;

							// apply shared model state (mesh)
							modelNode->ApplySharedState(frameIndex);
							FRAME_LOG("        FrameBatch::RenderBatch() node: %s", modelNode->GetName().Value());

#if NEBULA3_ENABLE_PROFILING
							modelNode->StartTimer();
#endif
							IndexT nodeInstIndex;
							for (nodeInstIndex = 0; nodeInstIndex < nodeInstances.Size(); nodeInstIndex++)
							{
								const Ptr<ModelNodeInstance>& nodeInstance = nodeInstances[nodeInstIndex];
								const Ptr<StateNodeInstance>& stateNode = nodeInstance.downcast<StateNodeInstance>();
#if NEBULA3_ENABLE_PROFILING
								nodeInstance->StartDebugTimer();
#endif  
								// render the node instance
								nodeInstance->ApplyState(frameIndex, pass.index);

								// render single
								nodeInstance->Render();
#if NEBULA3_ENABLE_PROFILING
								modelNode->IncrementDraws();
								nodeInstance->StopDebugTimer();
#endif  
							}

#if NEBULA3_ENABLE_PROFILING
							modelNode->StopTimer();
#endif
						}
					}
				}
			}
		}
	}
	*/

	// end batch
	CoreGraphics::EndBatch();
}

} // namespace Frame2