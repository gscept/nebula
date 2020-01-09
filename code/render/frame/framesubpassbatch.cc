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
		MaterialType* type = (*types)[typeIdx];
		IndexT idx = drawList->FindIndex(type);
		if (idx != InvalidIndex)
		{
			// if BeginBatch returns true if this material type has a shader for this batch
			if (Materials::MaterialBeginBatch(type, batch))
			{
				auto& model = drawList->ValueAtIndex(type, idx);
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
						// apply node-wide state
						node->ApplyNodeState();

						// bind graphics pipeline
						CoreGraphics::SetGraphicsPipeline();

						if (Materials::MaterialBeginSurface(stateNode->GetSurface()))
						{
							IndexT i;
							for (i = 0; i < instances.Size(); i++)
							{
								Models::ModelNode::DrawPacket* instance = instances[i];

								// apply instance state
								instance->Apply();

								//instance->Update();
								CoreGraphics::Draw();
							}

							// end surface
							Materials::MaterialEndSurface();
						}
					}
					it++;
				}
			}
			Materials::MaterialEndBatch();
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
		MaterialType* type = (*types)[typeIdx];
		IndexT idx = drawList->FindIndex(type);
		if (idx != InvalidIndex)
		{
			// if BeginBatch returns true if this material type has a shader for this batch
			if (Materials::MaterialBeginBatch(type, batch))
			{
				auto& model = drawList->ValueAtIndex(type, idx);
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
						// apply node-wide state
						node->ApplyNodeState();

						// bind graphics pipeline
						CoreGraphics::SetGraphicsPipeline();

						if (Materials::MaterialBeginSurface(stateNode->GetSurface()))
						{
							IndexT i;
							for (i = 0; i < instances.Size(); i++)
							{
								Models::ModelNode::DrawPacket* instance = instances[i];

								// apply instance state
								instance->Apply();

								//instance->Update();
								CoreGraphics::DrawInstanced(numInstances, 0);
							}

							// end surface
							Materials::MaterialEndSurface();
						}
					}
					it++;
				}
			}
			Materials::MaterialEndBatch();
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