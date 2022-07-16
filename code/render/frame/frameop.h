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
#include "util/tupleutility.h"
#include "coregraphics/config.h"
#include "coregraphics/barrier.h"
#include "coregraphics/event.h"
#include "coregraphics/semaphore.h"
#include "memory/arenaallocator.h"
#include "coregraphics/commandbuffer.h"
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

    CoreGraphics::BarrierDomain domain;
    CoreGraphics::QueueType queue;
    Util::Dictionary<CoreGraphics::TextureId, Util::Tuple<Util::StringAtom, CoreGraphics::PipelineStage, CoreGraphics::ImageSubresourceInfo>> textureDeps;
    Util::Dictionary<CoreGraphics::BufferId, Util::Tuple<Util::StringAtom, CoreGraphics::PipelineStage, CoreGraphics::BufferSubresourceInfo>> bufferDeps;

protected:
    friend class FrameScriptLoader;
    friend class FrameScript;
    friend class FramePass;
    friend class FrameSubpass;
    friend class FrameSubmission;
    friend class FrameSubgraph;

    // inherit this class to implement the compiled runtime for the frame operation
    struct Compiled
    {
        Compiled() 
        {
        }

        /// Run operation on a specific command buffer
        virtual void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) = 0;
        /// Discard operation
        virtual void Discard();

        /// Perform synchronization prior to execution of operation
        virtual void QueuePreSync(const CoreGraphics::CmdBufferId cmdBuf);

        Util::Array<CoreGraphics::BarrierId> barriers;
        CoreGraphics::QueueType queue;
    };

    struct TextureDependency
    {
        CoreGraphics::PipelineStage stage;
        DependencyIntent intent;
        CoreGraphics::ImageSubresourceInfo subres;
        CoreGraphics::QueueType queue;
    };

    struct BufferDependency
    {
        CoreGraphics::PipelineStage stage;
        DependencyIntent intent;
        CoreGraphics::BufferSubresourceInfo subres;
        CoreGraphics::QueueType queue;
    };

    static void AnalyzeAndSetupTextureBarriers(
        struct FrameOp::Compiled* op,
        CoreGraphics::TextureId tex,
        const Util::StringAtom& textureName,
        DependencyIntent readOrWrite,
        CoreGraphics::PipelineStage stage,
        CoreGraphics::BarrierDomain domain,
        const CoreGraphics::ImageSubresourceInfo& subres,
        CoreGraphics::QueueType fromQueue,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, CoreGraphics::BarrierCreateInfo>& barriers,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, CoreGraphics::EventCreateInfo>& waitEvents,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, struct FrameOp::Compiled*>& signalEvents,
        Util::Array<FrameOp::TextureDependency>& renderTextureDependencies);

    static void AnalyzeAndSetupBufferBarriers(
        struct FrameOp::Compiled* op,
        CoreGraphics::BufferId buf,
        const Util::StringAtom& bufferName,
        DependencyIntent readOrWrite,
        CoreGraphics::PipelineStage stage,
        CoreGraphics::BarrierDomain domain,
        const CoreGraphics::BufferSubresourceInfo& subres,
        CoreGraphics::QueueType fromQueue,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, CoreGraphics::BarrierCreateInfo>& barriers,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, CoreGraphics::EventCreateInfo>& waitEvents,
        Util::Dictionary<Util::Tuple<CoreGraphics::PipelineStage, CoreGraphics::PipelineStage>, struct FrameOp::Compiled*>& signalEvents,
        Util::Array<FrameOp::BufferDependency>& bufferDependencies);

    /// allocate instance of compiled
    virtual Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator) = 0;

    /// build operation
    virtual void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& buffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures
        );

    /// setup synchronization
    void SetupSynchronization(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& buffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures);

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
