//------------------------------------------------------------------------------
// frameop.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescript.h"
#include "frameop.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameOp::FrameOp() :
	queue(CoreGraphicsQueueType::GraphicsQueueType),
	domain(CoreGraphics::BarrierDomain::Global)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::~FrameOp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameOp::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::OnWindowResized()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps,
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& rwTextures,
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, BufferDependency>& rwBuffers,
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& renderTextures)
{
	// create compiled version of this op, FramePass and FrameSubpass implement this differently than ordinary ops
	this->compiled = this->AllocCompiled(allocator);
	compiledOps.Append(this->compiled);

	this->SetupSynchronization(allocator, events, barriers, rwTextures, rwBuffers, renderTextures);
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameOp::SetupSynchronization(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<CoreGraphics::EventId>& events, 
	Util::Array<CoreGraphics::BarrierId>& barriers, 
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& rwTextures, 
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, BufferDependency>& rwBuffers, 
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& renderTextures)
{
	n_assert(this->compiled != nullptr);
	IndexT i;

	if (!this->rwTextureDeps.IsEmpty() || !this->rwBufferDeps.IsEmpty() || !this->renderTextureDeps.IsEmpty())
	{
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::EventCreateInfo> waitEvents;
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::BarrierCreateInfo> barriers;
		Util::Dictionary<std::tuple<IndexT, IndexT>, FrameOp::Compiled*> signalEvents;
		uint numOutputs = 0;

		// go through texture dependencies
		for (i = 0; i < this->rwTextureDeps.Size(); i++)
		{
			const CoreGraphics::ShaderRWTextureId& tex = this->rwTextureDeps.KeyAtIndex(i);
			IndexT idx = rwTextures.FindIndex(this->rwTextureDeps.KeyAtIndex(i));

			// right dependency set
			const Util::StringAtom& name = std::get<0>(this->rwTextureDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierAccess& access = std::get<1>(this->rwTextureDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierStage& stage = std::get<2>(this->rwTextureDeps.ValueAtIndex(i));
			const CoreGraphics::ImageSubresourceInfo& subres = std::get<3>(this->rwTextureDeps.ValueAtIndex(i));
			const CoreGraphicsImageLayout& layout = std::get<4>(this->rwTextureDeps.ValueAtIndex(i));

			DependencyIntent readOrWrite = DependencyIntent::Read;
			switch (access)
			{
				case CoreGraphics::BarrierAccess::ShaderWrite:
				case CoreGraphics::BarrierAccess::ColorAttachmentWrite:
				case CoreGraphics::BarrierAccess::DepthAttachmentWrite:
				case CoreGraphics::BarrierAccess::HostWrite:
				case CoreGraphics::BarrierAccess::MemoryWrite:
				case CoreGraphics::BarrierAccess::TransferWrite:
				readOrWrite = DependencyIntent::Write;
				numOutputs++;
				break;
			}

			// if true, it means someone else created a dependency on this resource
			if (idx != InvalidIndex)
			{
				// left dependency set
				const Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>& deps = rwTextures.ValueAtIndex(idx);
				bool createNew = true;
				for (IndexT j = 0; j < deps.Size(); j++)
				{
					const CoreGraphics::ImageSubresourceInfo& info = std::get<0>(deps[j]);
					TextureDependency& dep = std::get<1>(deps[j]);

					if (info.Overlaps(subres))
					{
						createNew = false;
						const std::tuple<IndexT, IndexT> pair = std::make_pair(this->index, dep.index);

						// if we are reading, and the previous operation was a read, we can skip this update
						if (dep.intent == DependencyIntent::Read && readOrWrite == DependencyIntent::Read && dep.layout == layout);
						else
						{
							// create semaphore
							if (dep.queue != this->queue)
							{
								n_error("FRAME SCRIPT BUILD ERROR: Cross queue synchronization must be done using submissions!\n");
							}
							else
							{
								// create event
								if (dep.index - this->index > 1)
								{
									CoreGraphics::EventCreateInfo& info = waitEvents.AddUnique(pair);
									info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
									info.createSignaled = false;
									info.leftDependency = dep.stage;
									info.rightDependency = stage;
									info.rwTextures.Append(CoreGraphics::RWTextureBarrier{ tex, subres, dep.layout, layout, dep.access, access });
									signalEvents.AddUnique(pair) = dep.op;
								}
								else // create barrier
								{
									CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(pair);
									info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
									info.domain = this->domain;
									info.leftDependency = dep.stage;
									info.rightDependency = stage;
									info.rwTextures.Append(CoreGraphics::RWTextureBarrier{ tex, subres, dep.layout, layout, dep.access, access });
								}
							}
						}

						// update texture dependency
						dep.op = this->compiled;
						dep.access = access;
						dep.layout = layout;
						dep.stage = stage;
						dep.index = this->index;
						dep.queue = queue;
						dep.intent = readOrWrite;
					}
					if (createNew)
					{
						// no dependency for this subresource, create one
						Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>& deps = rwTextures[this->rwTextureDeps.KeyAtIndex(i)];
						deps.Append(std::make_tuple(subres, TextureDependency{ this->compiled, this->queue, layout, stage, access, readOrWrite, this->index }));
					}
				}
			}
			else
			{
				// get original layout
				CoreGraphicsImageLayout origLayout = CoreGraphics::ShaderRWTextureGetLayout(tex);

				// there is no entry, but we missmatch the default layout for a render texture, so insert a barrier
				if (layout != origLayout)
				{
					CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(std::make_pair(-1, this->index));
					info.domain = CoreGraphics::BarrierDomain::Global;
					info.leftDependency = CoreGraphics::BarrierStage::ComputeShader;
					info.rightDependency = stage;
					info.rwTextures.Append(CoreGraphics::RWTextureBarrier{ tex, subres, origLayout, layout, CoreGraphics::BarrierAccess::ShaderRead, access });
				}

				// no dependency for this resource at all, so insert one now
				Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>> deps;
				deps.Append(std::make_tuple(subres, TextureDependency{ this->compiled, this->queue, layout, stage, access, readOrWrite, this->index }));
				rwTextures.Add(this->rwTextureDeps.KeyAtIndex(i), deps);
			}
		}

		// go through texture dependencies
		for (i = 0; i < this->renderTextureDeps.Size(); i++)
		{
			const CoreGraphics::RenderTextureId& tex = this->renderTextureDeps.KeyAtIndex(i);
			IndexT idx = renderTextures.FindIndex(this->renderTextureDeps.KeyAtIndex(i));

			// right dependency set
			const Util::StringAtom& name = std::get<0>(this->renderTextureDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierAccess& access = std::get<1>(this->renderTextureDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierStage& stage = std::get<2>(this->renderTextureDeps.ValueAtIndex(i));
			const CoreGraphics::ImageSubresourceInfo& subres = std::get<3>(this->renderTextureDeps.ValueAtIndex(i));
			const CoreGraphicsImageLayout& layout = std::get<4>(this->renderTextureDeps.ValueAtIndex(i));

			DependencyIntent readOrWrite = DependencyIntent::Read;
			switch (access)
			{
				case CoreGraphics::BarrierAccess::ShaderWrite:
				case CoreGraphics::BarrierAccess::ColorAttachmentWrite:
				case CoreGraphics::BarrierAccess::DepthAttachmentWrite:
				case CoreGraphics::BarrierAccess::HostWrite:
				case CoreGraphics::BarrierAccess::MemoryWrite:
				case CoreGraphics::BarrierAccess::TransferWrite:
				readOrWrite = DependencyIntent::Write;
				numOutputs++;
				break;
			}

			// if true, it means someone else created a dependency on this resource
			if (idx != InvalidIndex)
			{
				// left dependency set
				const Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>& deps = renderTextures.ValueAtIndex(idx);
				bool createNew = true;
				for (IndexT j = 0; j < deps.Size(); j++)
				{
					const CoreGraphics::ImageSubresourceInfo& info = std::get<0>(deps[j]);
					TextureDependency& dep = std::get<1>(deps[j]);

					if (info.Overlaps(subres))
					{
						createNew = false;
						const std::tuple<IndexT, IndexT> pair = std::make_pair(this->index, dep.index);

						// if we are reading, and the previous operation was a read, we can skip this update
						if (dep.intent == DependencyIntent::Read && readOrWrite == DependencyIntent::Read && dep.layout == layout);
						else
						{
							// create semaphore
							if (dep.queue != this->queue)
							{
								n_error("FRAME SCRIPT BUILD ERROR: Cross queue synchronization must be done using submissions!\n");
							}
							else
							{
								// create event
								if (dep.index - this->index > 1)
								{
									CoreGraphics::EventCreateInfo& info = waitEvents.AddUnique(pair);
									info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
									info.createSignaled = false;
									info.leftDependency = dep.stage;
									info.rightDependency = stage;
									info.renderTextures.Append(CoreGraphics::RenderTextureBarrier{ tex, subres, dep.layout, layout, dep.access, access });
									signalEvents.AddUnique(pair) = dep.op;
								}
								else // create barrier
								{
									CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(pair);
									info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
									info.domain = this->domain;
									info.leftDependency = dep.stage;
									info.rightDependency = stage;
									info.renderTextures.Append(CoreGraphics::RenderTextureBarrier{ tex, subres, dep.layout, layout, dep.access, access });
								}
							}
						}

						// update texture dependency
						dep.op = this->compiled;
						dep.access = access;
						dep.layout = layout;
						dep.stage = stage;
						dep.queue = queue;
						dep.intent = readOrWrite;
					}
				}
				if (createNew)
				{
					// no dependency for this subresource, create one
					Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>& deps = renderTextures[this->renderTextureDeps.KeyAtIndex(i)];
					deps.Append(std::make_tuple(subres, TextureDependency{ this->compiled, this->queue, layout, stage, access, readOrWrite, this->index }));
				}
			}
			else
			{
				// get original layout
				CoreGraphicsImageLayout origLayout = CoreGraphics::RenderTextureGetLayout(tex);

				// there is no entry, but we missmatch the default layout for a render texture, so insert a barrier
				if (layout != origLayout)
				{
					CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(std::make_pair(-1, this->index));
					info.domain = CoreGraphics::BarrierDomain::Global;
					info.leftDependency = CoreGraphics::BarrierStage::PixelShader;
					info.rightDependency = stage;
					info.renderTextures.Append(CoreGraphics::RenderTextureBarrier{ tex, subres, origLayout, layout, subres.aspect == CoreGraphicsImageAspect::ColorBits ? CoreGraphics::BarrierAccess::ShaderRead : CoreGraphics::BarrierAccess::DepthAttachmentRead, access });
				}

				// no dependency for this resource at all, so insert one now
				Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>> deps;
				deps.Append(std::make_tuple(subres, TextureDependency{ this->compiled, this->queue, layout, stage, access, readOrWrite, this->index }));
				renderTextures.Add(this->renderTextureDeps.KeyAtIndex(i), deps);
			}
		}

		// go through buffer dependencies
		for (i = 0; i < this->rwBufferDeps.Size(); i++)
		{
			const CoreGraphics::ShaderRWBufferId& buf = this->rwBufferDeps.KeyAtIndex(i);
			IndexT idx = rwBuffers.FindIndex(this->rwBufferDeps.KeyAtIndex(i));

			// right dependency set
			const Util::StringAtom& name = std::get<0>(this->rwBufferDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierAccess& access = std::get<1>(this->rwBufferDeps.ValueAtIndex(i));
			const CoreGraphics::BarrierStage& stage = std::get<2>(this->rwBufferDeps.ValueAtIndex(i));

			DependencyIntent readOrWrite = DependencyIntent::Read;
			switch (access)
			{
				case CoreGraphics::BarrierAccess::ShaderWrite:
				case CoreGraphics::BarrierAccess::ColorAttachmentWrite:
				case CoreGraphics::BarrierAccess::DepthAttachmentWrite:
				case CoreGraphics::BarrierAccess::HostWrite:
				case CoreGraphics::BarrierAccess::MemoryWrite:
				case CoreGraphics::BarrierAccess::TransferWrite:
				readOrWrite = DependencyIntent::Write;
				numOutputs++;
				break;
			}

			if (idx != InvalidIndex)
			{
				// left dependency set
				BufferDependency& dep = rwBuffers.ValueAtIndex(idx);
				const std::tuple<IndexT, IndexT> pair = std::make_pair(this->index, dep.index);

				// if we are reading, and the previous operation was a read, we can skip this update
				if (dep.intent == DependencyIntent::Read && readOrWrite == DependencyIntent::Read);
				else
				{
					// create semaphore
					if (dep.queue != this->queue)
					{
						n_error("FRAME SCRIPT BUILD ERROR: Cross queue synchronization must be done using submissions!\n");
					}
					else
					{
						// create event if distance is more than 1 op and if there is an op dependency
						if (dep.index - this->index > 1 && dep.op != nullptr)
						{
							CoreGraphics::EventCreateInfo& info = waitEvents.AddUnique(pair);
							info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
							info.createSignaled = false;
							info.leftDependency = dep.stage;
							info.rightDependency = stage;
							info.rwBuffers.Append(CoreGraphics::BufferBarrier{ buf, dep.access, access, 0, -1 });
							signalEvents.AddUnique(pair) = dep.op;
						}
						else // create barrier
						{
							CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(pair);
							info.name = info.name.IsValid() ? info.name.AsString() + " + " + name.AsString() : name.AsString();
							info.domain = this->domain;
							info.leftDependency = dep.stage;
							info.rightDependency = stage;
							info.rwBuffers.Append(CoreGraphics::BufferBarrier{ buf, dep.access, access, 0, -1 });
						}
					}
				}

				// update texture dependency
				dep.op = this->compiled;
				dep.access = access;
				dep.stage = stage;
				dep.index = this->index;
				dep.queue = queue;
				dep.intent = readOrWrite;
			}
			else
			{
				// no dependency yet, so insert one now
				rwBuffers.Add(this->rwBufferDeps.KeyAtIndex(i), BufferDependency{ this->compiled, this->queue, stage, access, readOrWrite, this->index });
			}
		}

#pragma push_macro("CreateEvent")
#undef CreateEvent

		// allocate inputs, which is what we wait for or if we immediately trigger a barrier _before_ we execute the command
		this->compiled->waitEvents = waitEvents.Size() > 0 ? (decltype(this->compiled->waitEvents))allocator.Alloc(sizeof(*this->compiled->waitEvents) * waitEvents.Size()) : nullptr;
		this->compiled->barriers = barriers.Size() > 0 ? (decltype(this->compiled->barriers))allocator.Alloc(sizeof(*this->compiled->barriers) * barriers.Size()) : nullptr;

		// allocate for possible output (this will allocate a signal event slot for each output, which can only be signaled once)
		this->compiled->signalEvents = numOutputs > 0 ? (decltype(this->compiled->signalEvents))allocator.Alloc(sizeof(*this->compiled->signalEvents) * numOutputs) : nullptr;

		// create pre-execution events and barriers
		for (i = 0; i < waitEvents.Size(); i++)
		{
			waitEvents.ValueAtIndex(i).name = waitEvents.ValueAtIndex(i).name.AsString() + " <Frame Event>";
			CoreGraphics::EventId ev = CreateEvent(waitEvents.ValueAtIndex(i));
			events.Append(ev);
			this->compiled->waitEvents[i].event = ev;
			this->compiled->waitEvents[i].queue = this->queue;

			// get parent and add signaling event to it
			FrameOp::Compiled* parent = signalEvents.ValueAtIndex(i);
			parent->signalEvents[parent->numSignalEvents++].event = ev;
		}
		this->compiled->numWaitEvents = waitEvents.Size();

		for (i = 0; i < barriers.Size(); i++)
		{
			barriers.ValueAtIndex(i).name = barriers.ValueAtIndex(i).name.AsString() + " <Frame Barrier>";
			CoreGraphics::BarrierId bar = CreateBarrier(barriers.ValueAtIndex(i));
			this->compiled->barriers[i].barrier = bar;
			this->compiled->barriers[i].queue = this->queue;
		}
		this->compiled->numBarriers = barriers.Size();

#pragma pop_macro("CreateEvent")
	}
}


//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Compiled::Discard()
{
	// do nothing, the script is responsible for keeping track of the resources
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Compiled::QueuePreSync()
{
	IndexT i;
	for (i = 0; i < this->numWaitEvents; i++)
	{
		CoreGraphics::EventWaitAndReset(this->waitEvents[i].event, this->waitEvents[i].queue);
	}
	for (i = 0; i < this->numBarriers; i++)
	{
		CoreGraphics::BarrierReset(this->barriers[i].barrier);
		CoreGraphics::BarrierInsert(this->barriers[i].barrier, this->barriers[i].queue);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Compiled::QueuePostSync()
{
	IndexT i;
	for (i = 0; i < this->numSignalEvents; i++)
	{
		CoreGraphics::EventSignal(this->signalEvents[i].event, this->signalEvents[i].queue);
	}
}

} // namespace Frame2