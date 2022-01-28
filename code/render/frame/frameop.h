#pragma once
//------------------------------------------------------------------------------
/**
    A frame op is a base class for frame operations, use as base class for runnable
    sequences within a frame script.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/barrier.h"
#include "coregraphics/event.h"
#include "coregraphics/semaphore.h"
#include "memory/arenaallocator.h"
namespace Frame
{

enum class DependencyIntent
{
    Read,   // reading means we must wait if we are writing
    Write   // writing always means we must wait for previous writes and reads to finish
};


class FrameOp
{
public:

    /// constructor
    FrameOp();
    /// destructor
    virtual ~FrameOp();

    /// discard operation
    virtual void Discard();

    /// set name
    void SetName(const Util::StringAtom& name);
    /// get name
    const Util::StringAtom& GetName() const;

    /// set if operation should be enabled
    void SetEnabled(bool b);
    /// get if operation is enabled
    const bool GetEnabled() const;

    /// add child operation
    void AddChild(FrameOp* op);
    /// get children
    const Util::Array<Frame::FrameOp*>& GetChildren();

    /// handle display resizing
    virtual void OnWindowResized();

protected:
    friend class FrameScriptLoader;
    friend class FrameScript;
    friend class FramePass;
    friend class FrameSubpass;
    friend class FrameSubmission;

    // inherit this class to implement the compiled runtime for the frame operation
    struct Compiled
    {
        Compiled() 
            : numWaitEvents(0)
            , waitEvents(nullptr)
            , numSignalEvents(0)
            , signalEvents(nullptr)
            , numBarriers(0)
            , barriers(nullptr)
        {
        }

        virtual void UpdateResources(const IndexT frameIndex, const IndexT bufferIndex);
        virtual void RunJobs(const IndexT frameIndex, const IndexT bufferIndex);
        virtual void Run(const IndexT frameIndex, const IndexT bufferIndex) = 0;
        virtual void Discard();

        virtual void QueuePreSync();
        virtual void QueuePostSync();

        SizeT numWaitEvents;
        struct
        {
            CoreGraphics::EventId event;
            CoreGraphics::BarrierStage waitStage;
            CoreGraphics::BarrierStage signalStage;
            CoreGraphics::QueueType queue;
        } *waitEvents;

        SizeT numSignalEvents;
        struct
        {
            CoreGraphics::EventId event;
            CoreGraphics::BarrierStage stage;
            CoreGraphics::QueueType queue;
        } *signalEvents;

        SizeT numBarriers;
        struct
        {
            CoreGraphics::BarrierId barrier;
            CoreGraphics::QueueType queue;
        } *barriers;

        CoreGraphics::QueueType queue;
    };

    struct TextureDependency
    {
        FrameOp::Compiled* op;
        CoreGraphics::QueueType queue;
        CoreGraphics::ImageLayout layout;
        CoreGraphics::BarrierStage stage;
        CoreGraphics::BarrierAccess access;
        DependencyIntent intent;
        IndexT index;
        CoreGraphics::ImageSubresourceInfo subres;
    };

    struct BufferDependency
    {
        FrameOp::Compiled* op;
        CoreGraphics::QueueType queue;
        CoreGraphics::BarrierStage stage;
        CoreGraphics::BarrierAccess access;
        DependencyIntent intent;
        IndexT index;

        CoreGraphics::BufferSubresourceInfo subres;
    };

    static void AnalyzeAndSetupTextureBarriers(
        struct FrameOp::Compiled* op,
        CoreGraphics::TextureId tex,
        const Util::StringAtom& textureName,
        DependencyIntent readOrWrite,
        CoreGraphics::BarrierAccess access,
        CoreGraphics::BarrierStage stage,
        CoreGraphics::ImageLayout layout,
        CoreGraphics::BarrierDomain domain,
        const CoreGraphics::ImageSubresourceInfo& subres,
        IndexT fromIndex,
        CoreGraphics::QueueType fromQueue,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
        Util::Array<FrameOp::TextureDependency>& renderTextureDependencies);

    static void AnalyzeAndSetupBufferBarriers(
        struct FrameOp::Compiled* op,
        CoreGraphics::BufferId buf,
        const Util::StringAtom& bufferName,
        DependencyIntent readOrWrite,
        CoreGraphics::BarrierAccess access,
        CoreGraphics::BarrierStage stage,
        CoreGraphics::BarrierDomain domain,
        const CoreGraphics::BufferSubresourceInfo& subres,
        IndexT fromIndex,
        CoreGraphics::QueueType fromQueue,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
        Util::Dictionary<Util::Tuple<IndexT, IndexT, CoreGraphics::BarrierStage, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
        Util::Array<FrameOp::BufferDependency>& bufferDependencies);

    /// allocate instance of compiled
    virtual Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator) = 0;

    /// build operation
    virtual void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures,
#if NEBULA_ENABLE_MT_DRAW
        CoreGraphics::CommandBufferPoolId commandBufferPool = CoreGraphics::InvalidCommandBufferPoolId
#endif
        );

    /// setup synchronization
    void SetupSynchronization(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures);

    CoreGraphics::BarrierDomain domain;
    CoreGraphics::QueueType queue;
    Util::Dictionary<CoreGraphics::TextureId, Util::Tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::ImageSubresourceInfo, CoreGraphics::ImageLayout>> textureDeps;
    Util::Dictionary<CoreGraphics::BufferId, Util::Tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::BufferSubresourceInfo>> rwBufferDeps;

    Util::Array<FrameOp*> children;
    Util::Dictionary<Util::StringAtom, FrameOp*> childrenByName;
    Compiled* compiled;
    Util::StringAtom name;
    IndexT index;
    bool enabled;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameOp::SetName(const Util::StringAtom& name)
{
    this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
FrameOp::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FrameOp::SetEnabled(bool b)
{
    this->enabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
FrameOp::GetEnabled() const
{
    return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FrameOp::AddChild(FrameOp* op)
{
    this->children.Append(op);
    this->childrenByName.Add(op->GetName(), op);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Frame::FrameOp*>& 
FrameOp::GetChildren()
{
    return this->children;
}

} // namespace Frame2
