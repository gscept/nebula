#pragma once
//------------------------------------------------------------------------------
/**
	A frame op is a base class for frame operations, use as base class for runnable
	sequences within a frame script.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
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
	typedef uint ExecutionMask;

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

		virtual void Run(const IndexT frameIndex) = 0;
		virtual void Discard();

		virtual void QueuePreSync();
		virtual void QueuePostSync();

		SizeT numWaitEvents;
		struct
		{
			CoreGraphics::EventId event;
			CoreGraphicsQueueType queue;
		} *waitEvents;

		SizeT numSignalEvents;
		struct
		{
			CoreGraphics::EventId event;
			CoreGraphicsQueueType queue;
		} *signalEvents;

		SizeT numBarriers;
		struct
		{
			CoreGraphics::BarrierId barrier;
			CoreGraphicsQueueType queue;
		} *barriers;

		CoreGraphicsQueueType queue;
	};

	struct TextureDependency
	{
		FrameOp::Compiled* op;
		CoreGraphicsQueueType queue;
		CoreGraphicsImageLayout layout;
		CoreGraphics::BarrierStage stage;
		CoreGraphics::BarrierAccess access;
		DependencyIntent intent;
		IndexT index;
		CoreGraphics::ImageSubresourceInfo subres;
	};

	struct BufferDependency
	{
		FrameOp::Compiled* op;
		CoreGraphicsQueueType queue;
		CoreGraphics::BarrierStage stage;
		CoreGraphics::BarrierAccess access;
		DependencyIntent intent;
		IndexT index;

		CoreGraphics::BufferSubresourceInfo subres;
	};

	template <typename RESOURCE_TYPE>
	static void AnalyzeAndSetupTextureBarriers(
		struct FrameOp::Compiled* op,
		RESOURCE_TYPE tex,
		const Util::StringAtom& textureName,
		DependencyIntent readOrWrite,
		CoreGraphics::BarrierAccess access,
		CoreGraphics::BarrierStage stage,
		CoreGraphicsImageLayout layout,
		CoreGraphics::BarrierDomain domain,
		const CoreGraphics::ImageSubresourceInfo& subres,
		IndexT fromIndex,
		CoreGraphicsQueueType fromQueue,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::BarrierCreateInfo>& barriers,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::EventCreateInfo>& waitEvents,
		Util::Dictionary<std::tuple<IndexT, IndexT>, struct FrameOp::Compiled*>& signalEvents,
		Util::Array<FrameOp::TextureDependency>& renderTextureDependencies);

	template <typename RESOURCE_TYPE, typename BARRIER_TYPE>
	static void AnalyzeAndSetupTextureBarriers(
		struct FrameOp::Compiled* op,
		RESOURCE_TYPE tex,
		std::function<void(CoreGraphics::EventCreateInfo& info, const BARRIER_TYPE & barrier)> eventAddFunc,
		std::function<void(CoreGraphics::BarrierCreateInfo& info, const BARRIER_TYPE & barrier)> barrierAddFunc,
		const Util::StringAtom& textureName,
		DependencyIntent readOrWrite,
		CoreGraphics::BarrierAccess access,
		CoreGraphics::BarrierStage stage,
		CoreGraphicsImageLayout layout,
		CoreGraphics::BarrierDomain domain,
		const CoreGraphics::ImageSubresourceInfo& subres,
		IndexT fromIndex,
		CoreGraphicsQueueType fromQueue,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::BarrierCreateInfo>& barriers,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::EventCreateInfo>& waitEvents,
		Util::Dictionary<std::tuple<IndexT, IndexT>, struct FrameOp::Compiled*>& signalEvents,
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
		CoreGraphicsQueueType fromQueue,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::BarrierCreateInfo>& barriers,
		Util::Dictionary<std::tuple<IndexT, IndexT>, CoreGraphics::EventCreateInfo>& waitEvents,
		Util::Dictionary<std::tuple<IndexT, IndexT>, struct FrameOp::Compiled*>& signalEvents,
		Util::Array<FrameOp::BufferDependency>& bufferDependencies);

	/// allocate instance of compiled
	virtual Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator) = 0;

	/// build operation
	virtual void Build(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<FrameOp::Compiled*>& compiledOps,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<TextureDependency>>& rwTextures,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<TextureDependency>>& renderTextures);

	/// setup synchronization
	void SetupSynchronization(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<TextureDependency>>& rwTextures,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<TextureDependency>>& renderTextures);

	CoreGraphics::BarrierDomain domain;
	CoreGraphicsQueueType queue;
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, std::tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::ImageSubresourceInfo, CoreGraphicsImageLayout>> rwTextureDeps;
	Util::Dictionary<CoreGraphics::RenderTextureId, std::tuple<Util::StringAtom, CoreGraphics::BarrierAccess, CoreGraphics::BarrierStage, CoreGraphics::ImageSubresourceInfo, CoreGraphicsImageLayout>> renderTextureDeps;
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