#pragma once
//------------------------------------------------------------------------------
/**
	A frame op is a base class for frame operations, use as base class for runnable
	sequences within a frame script.
	
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
	Read,	// reading means we must wait if we are writing
	Write	// writing always means we must wait for previous writes and reads to finish
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

	/// handle display resizing
	virtual void OnWindowResized();

protected:
	friend class FrameScriptLoader;
	friend class FrameScript;
	friend class FramePass;
	friend class FrameSubpass;

	// inherit this class to implement the compiled runtime for the frame operation
	struct Compiled
	{
		Compiled() :
			numWaitEvents(0),
			numSignalEvents(0),
			numBarriers(0)
		{
		}

		virtual void UpdateResources(const IndexT frameIndex);
		virtual void RunJobs(const IndexT frameIndex);
		virtual void Run(const IndexT frameIndex) = 0;
		virtual void Discard();

		virtual void QueuePreSync();
		virtual void QueuePostSync();

		SizeT numWaitEvents;
		struct
		{
			CoreGraphics::EventId event;
			CoreGraphics::QueueType queue;
		} *waitEvents;

		SizeT numSignalEvents;
		struct
		{
			CoreGraphics::EventId event;
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
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
		Util::Array<FrameOp::TextureDependency>& renderTextureDependencies);

	static void	AnalyzeAndSetupBufferBarriers(
		struct FrameOp::Compiled* op,
		CoreGraphics::ShaderRWBufferId buf,
		const Util::StringAtom& bufferName,
		DependencyIntent readOrWrite,
		CoreGraphics::BarrierAccess access,
		CoreGraphics::BarrierStage stage,
		CoreGraphics::BarrierDomain domain,
		const CoreGraphics::BufferSubresourceInfo& subres,
		IndexT fromIndex,
		CoreGraphics::QueueType fromQueue,
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::BarrierCreateInfo>& barriers,
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, CoreGraphics::EventCreateInfo>& waitEvents,
		Util::Dictionary<std::tuple<IndexT, IndexT, CoreGraphics::BarrierStage>, struct FrameOp::Compiled*>& signalEvents,
		Util::Array<FrameOp::BufferDependency>& bufferDependencies);

	/// allocate instance of compiled
	virtual Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator) = 0;

	/// build operation
	virtual void Build(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<FrameOp::Compiled*>& compiledOps,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures);

	/// setup synchronization
	void SetupSynchronization(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures);

	CoreGraphics::BarrierDomain domain;
	CoreGraphics::QueueType queue;
	Util::Dictionary<CoreGraphics::TextureId, std::tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::ImageSubresourceInfo, CoreGraphics::ImageLayout>> textureDeps;
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, std::tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::BufferSubresourceInfo>> rwBufferDeps;

	Compiled* compiled;
	Util::StringAtom name;
	IndexT index;
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

} // namespace Frame2