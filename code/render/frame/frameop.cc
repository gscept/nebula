//------------------------------------------------------------------------------
// frameop.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
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
    queue(CoreGraphics::QueueType::GraphicsQueueType),
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
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
    // create compiled version of this op, FramePass and FrameSubpass implement this differently than ordinary ops
    this->compiled = this->AllocCompiled(allocator);
    compiledOps.Append(this->compiled);

    this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
}

//------------------------------------------------------------------------------
/**
*/
void
ImageSubresourceHelper(
    const CoreGraphics::ImageSubresourceInfo& fromSubres,
    const CoreGraphics::ImageSubresourceInfo& toSubres,
    Util::Array<CoreGraphics::ImageSubresourceInfo>& subresources)
{
    
}

//------------------------------------------------------------------------------
/**
    Analyze and setup barriers if needed
*/
void
FrameOp::AnalyzeAndSetupTextureBarriers(
    struct FrameOp::Compiled* op,
    CoreGraphics::TextureId tex,
    const Util::StringAtom& textureName,
    DependencyIntent readOrWrite,
    CoreGraphics::BarrierAccess access,
    CoreGraphics::BarrierStage stage,
    CoreGraphics::ImageLayout layout,
    CoreGraphics::BarrierDomain domain,
    const CoreGraphics::ImageSubresourceInfo& subres,
    IndexT toIndex,
    CoreGraphics::QueueType toQueue,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
    Util::Array<FrameOp::TextureDependency>& textureDependencies)
{
    Util::Array<CoreGraphics::ImageSubresourceInfo> subresources{ subres };

    // walk backwards in dependency list
    for (IndexT j = textureDependencies.Size() - 1; j >= 0 && subresources.Size() > 0; j--)
    {
        FrameOp::TextureDependency& dep = textureDependencies[j];
        const CoreGraphics::ImageSubresourceInfo& currentSubres = subresources.Front();
        const CoreGraphics::ImageSubresourceInfo& depSubres = dep.subres;

        // check if the dependency touches the same subresource (the framescript guarantees we will have at least one dependency which overlaps)
        if (currentSubres.Overlaps(depSubres))
        {
            // remove subresource that is overlapped
            subresources.EraseFront();

            // if these criteria are met, we need no barrier
            if (dep.intent == DependencyIntent::Read            // previous invocation was just reading
                && readOrWrite == DependencyIntent::Read        // we are just reading
                && dep.layout == layout                         // layouts are similar
                && dep.stage == stage                           // previous stage was conservative
                && dep.queue == toQueue)                        // we are on the same queue
            {
                // do nothing
            }
            else
            {
                // create semaphore
                if (dep.queue != toQueue)
                {
                    n_warning("Synchronization happens between queues, and should be handled with submissions explicitly!");
                }
                else
                {
                    // construct pair between ops
                    const Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage> tuple = Util::MakeTuple(toIndex, dep.index, dep.stage);
                    CoreGraphics::TextureBarrier barrier{ tex, subres, dep.layout, layout, dep.access, access };

                    const bool enableEvent = false;

                    // create event if gap between operations is bigger than 1
                    if (dep.index != InvalidIndex && toIndex - dep.index > 1 && enableEvent)
                    {
                        n_assert(dep.op != nullptr);
                        CoreGraphics::EventCreateInfo& info = waitEvents.AddUnique(tuple);
                        info.name = info.name.IsValid() ? info.name.AsString() + " + " + textureName.AsString() : textureName.AsString();
                        info.createSignaled = false;
                        info.textures.Append(barrier);
                        signalEvents.AddUnique(tuple) = dep.op;
                    }
                    else // create barrier
                    {
                        CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(tuple);
                        info.name = info.name.IsValid() ? info.name.AsString() + " + " + textureName.AsString() : textureName.AsString();
                        info.domain = domain;
                        info.leftDependency = dep.stage;
                        info.rightDependency = stage;
                        info.textures.Append(barrier);
                    }
                }

                // split barriers if we have a partially overlapping subresource on the mips
                if (currentSubres.mip < depSubres.mip || currentSubres.mipCount > depSubres.mipCount)
                {
                    uint leftStart = currentSubres.mip;
                    uint leftEnd = leftStart - depSubres.mip;
                    if (leftStart < leftEnd)
                    {
                        CoreGraphics::ImageSubresourceInfo leftRes = currentSubres;
                        leftRes.mipCount = leftEnd - leftStart;
                        subresources.Append(leftRes);
                    }

                    uint rightStart = depSubres.mip + depSubres.mipCount;
                    uint rightEnd = currentSubres.mip + currentSubres.mipCount;
                    if (rightStart < rightEnd)
                    {
                        CoreGraphics::ImageSubresourceInfo rightRes = depSubres;
                        rightRes.mip = rightStart;
                        rightRes.mipCount = rightEnd - rightStart;
                        subresources.Append(rightRes);
                    }
                }

                // split barriers if we have partially overlapping subresources on the layers
                if (currentSubres.layer < depSubres.layer || currentSubres.layerCount > depSubres.layerCount)
                {
                    uint leftStart = currentSubres.layer;
                    uint leftEnd = leftStart - depSubres.layer;
                    if (leftStart < leftEnd)
                    {
                        CoreGraphics::ImageSubresourceInfo leftRes = currentSubres;
                        leftRes.layerCount = leftEnd - leftStart;
                        subresources.Append(leftRes);
                    }

                    uint rightStart = depSubres.layer + depSubres.layerCount;
                    uint rightEnd = currentSubres.layer + currentSubres.layerCount;
                    if (rightStart < rightEnd)
                    {
                        CoreGraphics::ImageSubresourceInfo rightRes = depSubres;
                        rightRes.layer = rightStart;
                        rightRes.layerCount = rightEnd - rightStart;
                        subresources.Append(rightRes);
                    }
                }

                // add new dependency to list
                FrameOp::TextureDependency newDep;
                newDep.op = op;
                newDep.queue = toQueue;
                newDep.layout = layout;
                newDep.stage = stage;
                newDep.access = access;
                newDep.intent = readOrWrite;
                newDep.index = toIndex;
                newDep.subres = subres;
                textureDependencies.Append(newDep);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameOp::AnalyzeAndSetupBufferBarriers(
    struct FrameOp::Compiled* op,
    CoreGraphics::BufferId buf,
    const Util::StringAtom& bufferName,
    DependencyIntent readOrWrite,
    CoreGraphics::BarrierAccess access,
    CoreGraphics::BarrierStage stage,
    CoreGraphics::BarrierDomain domain,
    const CoreGraphics::BufferSubresourceInfo& subres,
    IndexT toIndex,
    CoreGraphics::QueueType toQueue,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
    Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
    Util::Array<FrameOp::BufferDependency>& bufferDependencies)
{
    Util::Array<CoreGraphics::BufferSubresourceInfo> subresources{ subres };

    // left dependency set
    for (IndexT j = bufferDependencies.Size() - 1; j >= 0 && subresources.Size() > 0; j--)
    {
        FrameOp::BufferDependency& dep = bufferDependencies[j];
        const CoreGraphics::BufferSubresourceInfo& currentSubres = subresources.Front();
        const CoreGraphics::BufferSubresourceInfo& depSubres = dep.subres;

        // check if the dependency touches the same subresource (the framescript guarnatees we will have at least one dependency which overlaps)
        if (currentSubres.Overlaps(depSubres))
        {
            subresources.EraseFront();

            // if these criteria are met, we need no barrier
            if (dep.intent == DependencyIntent::Read
                && readOrWrite == DependencyIntent::Read
                && dep.stage == stage                       // check if previous stage was conservative enough
                && dep.queue == toQueue)
                return; // we found a previous synch which satisfied our needs, so return
            else
            {
                // create semaphore
                if (dep.queue != toQueue)
                {
                    n_warning("Synchronization happens between queues, and should be handled with submissions explicitly!");
                }
                else
                {
                    // construct pair between ops
                    const Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage> tuple = Util::MakeTuple(toIndex, dep.index, dep.stage);
                    CoreGraphics::BufferBarrier barrier{ buf, dep.access, access, (IndexT)subres.offset, (IndexT)subres.size };

                    const bool enableEvent = false;

                    // create event
                    if (dep.index != InvalidIndex && toIndex - dep.index > 1 && enableEvent)
                    {
                        n_assert(dep.op != nullptr);
                        CoreGraphics::EventCreateInfo& info = waitEvents.AddUnique(tuple);
                        info.name = info.name.IsValid() ? info.name.AsString() + " + " + bufferName.AsString() : bufferName.AsString();
                        info.createSignaled = false;
                        info.rwBuffers.Append(barrier);
                        signalEvents.AddUnique(tuple) = dep.op;
                    }
                    else // create barrier
                    {
                        CoreGraphics::BarrierCreateInfo& info = barriers.AddUnique(tuple);
                        info.name = info.name.IsValid() ? info.name.AsString() + " + " + bufferName.AsString() : bufferName.AsString();
                        info.domain = domain;
                        info.leftDependency = dep.stage;
                        info.rightDependency = stage;
                        info.rwBuffers.Append(barrier);
                    }
                }
            }

            // split barriers if we have partially overlapping subresources on the layers
            if (currentSubres.offset < depSubres.offset || currentSubres.size > depSubres.size)
            {
                uint leftStart = currentSubres.offset;
                uint leftEnd = leftStart - depSubres.offset;
                if (leftStart < leftEnd)
                {
                    CoreGraphics::BufferSubresourceInfo leftRes = currentSubres;
                    leftRes.size = leftEnd - leftStart;
                    subresources.Append(leftRes);
                }

                uint rightStart = depSubres.offset + depSubres.size;
                uint rightEnd = currentSubres.offset + currentSubres.size;
                if (rightStart < rightEnd)
                {
                    CoreGraphics::BufferSubresourceInfo rightRes = depSubres;
                    rightRes.size = rightEnd - rightStart;
                    subresources.Append(rightRes);
                }
            }

            // add new buffer dependency
            FrameOp::BufferDependency newDep;
            newDep.op = op;
            newDep.queue = toQueue;
            newDep.stage = stage;
            newDep.access = access;
            newDep.intent = readOrWrite;
            newDep.index = toIndex;
            newDep.subres = subres;
            bufferDependencies.Append(newDep);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameOp::SetupSynchronization(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator,
    Util::Array<CoreGraphics::EventId>& events, 
    Util::Array<CoreGraphics::BarrierId>& barriers, 
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
    n_assert(this->compiled != nullptr);
    IndexT i;

    if (!this->textureDeps.IsEmpty() || !this->rwBufferDeps.IsEmpty())
    {
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo> waitEvents;
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo> barriers;
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, FrameOp::Compiled*> signalEvents;
        uint numOutputs = 0;

        // go through texture dependencies
        for (i = 0; i < this->textureDeps.Size(); i++)
        {
            const CoreGraphics::TextureId& tex = this->textureDeps.KeyAtIndex(i);
            const CoreGraphics::TextureId& alias = TextureGetAlias(tex);

            // right dependency set
            const Util::StringAtom& name = Util::Get<0>(this->textureDeps.ValueAtIndex(i));
            const CoreGraphics::BarrierAccess& access = Util::Get<1>(this->textureDeps.ValueAtIndex(i));
            const CoreGraphics::BarrierStage& stage = Util::Get<2>(this->textureDeps.ValueAtIndex(i));
            const CoreGraphics::ImageSubresourceInfo& subres = Util::Get<3>(this->textureDeps.ValueAtIndex(i));
            const CoreGraphics::ImageLayout& layout = Util::Get<4>(this->textureDeps.ValueAtIndex(i));

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

            // dependencies currently on the texture
            IndexT idx = textures.FindIndex(this->textureDeps.KeyAtIndex(i));
            Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);

            // analyze if synchronization is required and setup appropriate barriers and/or events
            AnalyzeAndSetupTextureBarriers(
                this->compiled, tex, name, readOrWrite, access, stage, layout, this->domain, subres, this->index, this->queue, barriers, waitEvents, signalEvents, deps);

            // if alias, also make sure to visit the alias
            if (alias != CoreGraphics::InvalidTextureId)
            {
                IndexT idx = textures.FindIndex(alias);
                Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);
                AnalyzeAndSetupTextureBarriers(
                    this->compiled, alias, name, readOrWrite, access, stage, layout, this->domain, subres, this->index, this->queue, barriers, waitEvents, signalEvents, deps);
            }
                
        }

        // go through buffer dependencies
        for (i = 0; i < this->rwBufferDeps.Size(); i++)
        {
            const CoreGraphics::BufferId& buf = this->rwBufferDeps.KeyAtIndex(i);
            IndexT idx = rwBuffers.FindIndex(this->rwBufferDeps.KeyAtIndex(i));

            // right dependency set
            const Util::StringAtom& name = Util::Get<0>(this->rwBufferDeps.ValueAtIndex(i));
            const CoreGraphics::BarrierAccess& access = Util::Get<1>(this->rwBufferDeps.ValueAtIndex(i));
            const CoreGraphics::BarrierStage& stage = Util::Get<2>(this->rwBufferDeps.ValueAtIndex(i));
            const CoreGraphics::BufferSubresourceInfo& subres = Util::Get<3>(this->rwBufferDeps.ValueAtIndex(i));

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
            
            // dependencies currently on the texture
            Util::Array<BufferDependency>& deps = rwBuffers.ValueAtIndex(idx);

            AnalyzeAndSetupBufferBarriers(
                this->compiled, buf, name, readOrWrite, access, stage, this->domain, subres, this->index, this->queue, barriers, waitEvents, signalEvents, deps);
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
FrameOp::Compiled::UpdateResources(const IndexT frameIndex, const IndexT bufferIndex)
{
    // implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Compiled::RunJobs(const IndexT frameIndex, const IndexT bufferIndex)
{
    // implement in subclass
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
        CoreGraphics::EventWaitAndReset(this->waitEvents[i].event, this->waitEvents[i].queue, this->waitEvents[i].waitStage, this->waitEvents[i].signalStage);
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
        CoreGraphics::EventSignal(this->signalEvents[i].event, this->signalEvents[i].queue, this->signalEvents[i].stage);
    }
}

} // namespace Frame2
