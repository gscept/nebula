//------------------------------------------------------------------------------
// framesubpassbatch.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassbatch.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "materials/materialserver.h"
#include "models/model.h"
#include "models/nodes/shaderstatenode.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "visibility/visibilitycontext.h"

#include "models/nodes/particlesystemnode.h"
#include "models/nodes/primitivenode.h"

using namespace Graphics;
using namespace CoreGraphics;
using namespace Materials;
using namespace Models;
using namespace Util;

#define NEBULA_FRAME_LOG_ENABLED   (0)
#if NEBULA_FRAME_LOG_ENABLED
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
FrameSubpassBatch::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->batch = this->batch;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpassBatch::DrawBatch(CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id)
{
	// now do usual render stuff
	ShaderServer* shaderServer = ShaderServer::Instance();
	MaterialServer* matServer = MaterialServer::Instance();

	// get current view and visibility draw list
	const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

	// start batch
	CoreGraphics::BeginBatch(FrameBatchType::Geometry);

	const Util::Array<MaterialType*>* types = matServer->GetMaterialTypesByBatch(batch);
	if ((types != nullptr) && (drawList != nullptr)) for (IndexT typeIdx = 0; typeIdx < types->Size(); typeIdx++)
	{
		MaterialType* materialType = (*types)[typeIdx];
		IndexT idx = drawList->FindIndex(materialType);
		if (idx != InvalidIndex)
		{

#if NEBULA_GRAPHICS_DEBUG
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_DARK_GREEN, materialType->GetName().AsCharPtr());
#endif

			// if BeginBatch returns true if this material type has a shader for this batch
			if (Materials::MaterialBeginBatch(materialType, batch))
			{
				auto& model = drawList->ValueAtIndex(materialType, idx);
                auto& it = model.Begin();
				auto& end = model.End();

				while (true)
				{
					Models::ModelNode* node = *it.key;
					Models::ShaderStateNode* stateNode = reinterpret_cast<Models::ShaderStateNode*>(node);
					Models::NodeType type = node->GetType();

					// only continue if we have instances
					const Util::Array<Models::ModelNode::DrawPacket*>& instances = *it.val;
					if (instances.Size() > 0)
					{
#if NEBULA_GRAPHICS_DEBUG
						CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, node->GetName().Value());
#endif
						// apply node-wide state
						node->ApplyNodeState();
						
						// bind graphics pipeline
						CoreGraphics::SetGraphicsPipeline();

						// apply surface
						Materials::MaterialApplySurface(materialType, stateNode->GetSurface());
						Models::NodeType type = node->GetType();

						IndexT i;
						for (i = 0; i < instances.Size(); i++)
						{
							Models::ModelNode::DrawPacket* instance = instances[i];
							instance->Apply(materialType);

							if (type != ParticleSystemNodeType)
							{
								Models::PrimitiveNode::Instance* pinst = reinterpret_cast<Models::PrimitiveNode::Instance*>(instance->node);
								pinst->Draw(1, instance);
							}
							else
							{
								Models::ParticleSystemNode::Instance* pinst = reinterpret_cast<Models::ParticleSystemNode::Instance*>(instance->node);
								pinst->Draw(1, instance);
							}
						}
					}
					if (it == end) break;
					it++;
				}
			}
			Materials::MaterialEndBatch(materialType);

#if NEBULA_GRAPHICS_DEBUG
			CommandBufferEndMarker(GraphicsQueueType);
#endif
		}
	}

	// end batch
	CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpassBatch::DrawBatch(CoreGraphics::BatchGroup::Code batch, const Graphics::GraphicsEntityId id, SizeT numInstances)
{
	// now do usual render stuff
	ShaderServer* shaderServer = ShaderServer::Instance();
	MaterialServer* matServer = MaterialServer::Instance();

	// get current view and visibility draw list
	const Visibility::ObserverContext::VisibilityDrawList* drawList = Visibility::ObserverContext::GetVisibilityDrawList(id);

	// start batch
	CoreGraphics::BeginBatch(FrameBatchType::Geometry);

	const Util::Array<MaterialType*>* types = matServer->GetMaterialTypesByBatch(batch);
	if ((types != nullptr) && (drawList != nullptr)) for (IndexT typeIdx = 0; typeIdx < types->Size(); typeIdx++)
	{
		MaterialType* materialType = (*types)[typeIdx];
		IndexT idx = drawList->FindIndex(materialType);
		if (idx != InvalidIndex)
		{

#if NEBULA_GRAPHICS_DEBUG
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_DARK_GREEN, materialType->GetName().AsCharPtr());
#endif

			// if BeginBatch returns true if this material type has a shader for this batch
			if (Materials::MaterialBeginBatch(materialType, batch))
			{
				auto& model = drawList->ValueAtIndex(materialType, idx);
				auto& it = model.Begin();
				auto& end = model.End();
				while (it != end)
				{
					Models::ModelNode* node = *it.key;
					Models::ShaderStateNode* stateNode = reinterpret_cast<Models::ShaderStateNode*>(node);

					// only continue if we have instances
					const Util::Array<Models::ModelNode::DrawPacket*>& instances = *it.val;
					if (instances.Size() > 0)
					{

#if NEBULA_GRAPHICS_DEBUG
						CommandBufferInsertMarker(GraphicsQueueType, NEBULA_MARKER_DARK_DARK_GREEN, node->GetName().Value());
#endif
						// apply node-wide state
						node->ApplyNodeState();

						// bind graphics pipeline
						CoreGraphics::SetGraphicsPipeline();

						// apply surface level resources
						Materials::MaterialApplySurface(materialType, stateNode->GetSurface());
						Models::NodeType type = node->GetType();

						IndexT i;
						for (i = 0; i < instances.Size(); i++)
						{
							Models::ModelNode::DrawPacket* instance = instances[i];
							instance->Apply(materialType);

							if (type != ParticleSystemNodeType)
							{
								Models::PrimitiveNode::Instance* pinst = reinterpret_cast<Models::PrimitiveNode::Instance*>(instance->node);
								pinst->Draw(numInstances, instance);
							}
							else
							{
								Models::ParticleSystemNode::Instance* pinst = reinterpret_cast<Models::ParticleSystemNode::Instance*>(instance->node);
								pinst->Draw(numInstances, instance);
							}
						}
					}
					it++;
				}
			}
			Materials::MaterialEndBatch(materialType);

#if NEBULA_GRAPHICS_DEBUG
			CommandBufferEndMarker(GraphicsQueueType);
#endif
		}
	}

	// end batch
	CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassBatch::CompiledImpl::Run(const IndexT frameIndex)
{
	const Ptr<View>& view = Graphics::GraphicsServer::Instance()->GetCurrentView();
	FrameSubpassBatch::DrawBatch(this->batch, view->GetCamera());
}

} // namespace Frame2