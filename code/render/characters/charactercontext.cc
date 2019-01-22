//------------------------------------------------------------------------------
//  charactercontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "charactercontext.h"
#include "skeleton.h"
#include "characters/streamskeletonpool.h"
#include "coreanimation/animresource.h"
#include "coreanimation/streamanimationpool.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "coreanimation/animsamplemixinfo.h"
#include "coreanimation/animsamplebuffer.h"
#include "util/round.h"

using namespace Graphics;
using namespace Resources;
namespace Characters
{

_ImplementContext(CharacterContext);
CharacterContext::CharacterContextAllocator CharacterContext::characterContextAllocator;

Jobs::JobPortId CharacterContext::jobPort;
Threading::SafeQueue<Jobs::JobId> CharacterContext::runningJobs;
Util::HashTable<Util::StringAtom, CoreAnimation::AnimSampleMask> CharacterContext::masks;

extern void AnimSampleJob(const Jobs::JobFuncContext& ctx);
extern void AnimSampleWithMixJob(const Jobs::JobFuncContext& ctx);
//------------------------------------------------------------------------------
/**
*/
CharacterContext::CharacterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
CharacterContext::~CharacterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Create()
{
	__bundle.OnBeforeFrame = CharacterContext::OnBeforeFrame;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = nullptr;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &CharacterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = CharacterContext::OnRenderDebug;
#endif
	CharacterContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle);

	Jobs::CreateJobPortInfo info =
	{
		"CharacterJobPort",
		2,
		System::Cpu::Core1 | System::Cpu::Core2 | System::Cpu::Core3 | System::Cpu::Core4,
		UINT_MAX
	};
	CharacterContext::jobPort = Jobs::CreateJobPort(info);

	_CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& skeleton, const Resources::ResourceName& animation, const Util::StringAtom& tag)
{
	const ContextEntityId cid = GetContextId(id);
	characterContextAllocator.Get<Loaded>(cid.id) = NoneLoaded;

	// create skeleton
	ResourceCreateInfo info;
	info.resource = skeleton;
	info.tag = tag;
	info.async = false;
	info.failCallback = nullptr;
	info.successCallback = [cid, id](Resources::ResourceId id)
	{
		characterContextAllocator.Get<SkeletonId>(cid.id) = id.As<Characters::SkeletonId>();
		characterContextAllocator.Get<Loaded>(cid.id) |= SkeletonLoaded;
	};
	characterContextAllocator.Get<SkeletonId>(cid.id) = Characters::CreateSkeleton(info);

	// create animation
	info.resource = animation;
	info.successCallback = [cid, id](Resources::ResourceId id)
	{
		characterContextAllocator.Get<AnimationId>(cid.id) = id.As<CoreAnimation::AnimResourceId>();
		characterContextAllocator.Get<Loaded>(cid.id) |= AnimationLoaded;

		// setup sample buffer when animation is done loading
		characterContextAllocator.Get<SampleBuffer>(cid.id).Setup(id.As<CoreAnimation::AnimResourceId>());
	};
	characterContextAllocator.Get<AnimationId>(cid.id) = CoreAnimation::CreateAnimation(info);

	// check to make sure we registered this entity for observation, then get the 
	const ContextEntityId visId = Visibility::ObservableContext::GetContextId(id);
	n_assert(visId != ContextEntityId::Invalid());
	characterContextAllocator.Get<VisibilityContextId>(cid.id) = visId;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
CharacterContext::GetClipIndex(const Graphics::GraphicsEntityId id, const Util::StringAtom& name)
{
	const ContextEntityId cid = GetContextId(id);
	return CoreAnimation::animPool->GetClipIndex(characterContextAllocator.Get<AnimationId>(cid.id), name);
}

//------------------------------------------------------------------------------
/**
*/
bool 
CharacterContext::PlayClip(
	const Graphics::GraphicsEntityId id, 
	const CoreAnimation::AnimSampleMask* mask,
	const IndexT clipIndex, 
	const IndexT track, 
	const EnqueueMode mode, 
	const float weight, 
	const SizeT loopCount, 
	const float startTime, 
	const float fadeIn, 
	const float fadeOut, 
	const float timeOffset, 
	const float timeFactor)
{
	n_assert(track < MaxNumTracks);
	const ContextEntityId cid = GetContextId(id);

	// the animation runtime is the context used to keep track of running animations
	AnimationRuntime runtime;
	runtime.clip = clipIndex;
	runtime.enqueueMode = mode;
	runtime.blend = weight;
	runtime.fadeInTime = fadeIn;
	runtime.fadeOutTime = fadeOut;
	runtime.timeFactor = timeFactor;
	runtime.timeOffset = timeOffset;
	runtime.startTime = startTime;
	runtime.mask = mask;

	runtime.baseTime = 0;
	runtime.duration = 0;
	runtime.evalTime = runtime.prevEvalTime = 0;
	runtime.sampleTime = runtime.prevSampleTime = 0;
	runtime.paused = false;

	// append to pending animations
	if (mode == Replace)
		characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track] = runtime;
	else if (mode == Append)
		characterContextAllocator.Get<TrackController>(cid.id).pendingAnimations[track].Append(runtime);
	else if (mode == IgnoreIfSame)
	{
		const AnimationRuntime& current = characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track];

		// if clip index doesn't match, append animation
		if (current.clip != clipIndex)
			characterContextAllocator.Get<TrackController>(cid.id).pendingAnimations[track].Append(runtime);
	}

	return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::StopTrack(const Graphics::GraphicsEntityId id, const IndexT track)
{
	n_assert(track < MaxNumTracks);
	const ContextEntityId cid = GetContextId(id);
	AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

	tracks.pendingAnimations[track].Clear();
	tracks.playingAnimations[track].clip = -1;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::StopAllTracks(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

	// just clear all processing animations
	for (IndexT i = 0; i < MaxNumTracks; i++)
	{
		tracks.pendingAnimations[i].Clear();
		tracks.playingAnimations[i].clip = -1;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::PauseTrack(const Graphics::GraphicsEntityId id, const IndexT track)
{
	n_assert(track < MaxNumTracks);
	const ContextEntityId cid = GetContextId(id);
	AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

	// set for all pending tracks, and the current track
	IndexT i;
	for (i = 0; i < tracks.pendingAnimations[track].Size(); i++)
		tracks.pendingAnimations[track][i].paused = !tracks.pendingAnimations[track][i].paused;

	tracks.playingAnimations[track].paused = !tracks.playingAnimations[track].paused;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Seek(const Graphics::GraphicsEntityId id, const float time)
{
	const ContextEntityId cid = GetContextId(id);
	characterContextAllocator.Get<AnimTime>(cid.id) = time;
}

//------------------------------------------------------------------------------
/**
*/
const float 
CharacterContext::GetTime(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return characterContextAllocator.Get<AnimTime>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
bool 
CharacterContext::IsPlaying(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);

	// if any track is playing, this entity is playing
	bool ret = false;
	for (IndexT i = 0; i < MaxNumTracks; i++)
		ret |= characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[i].clip != -1;

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::SetTrackWeight(const Graphics::GraphicsEntityId id, const IndexT track, const float weight)
{
	const ContextEntityId cid = GetContextId(id);
	AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

	// set for all pending tracks, and the current track
	IndexT i;
	for (i = 0; i < tracks.pendingAnimations[track].Size(); i++)
		tracks.pendingAnimations[track][i].blend = weight;

	tracks.playingAnimations[track].blend = weight;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::SetTrackTimeFactor(const Graphics::GraphicsEntityId id, const IndexT track, const float factor)
{
	const ContextEntityId cid = GetContextId(id);
	AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

	// set for all pending tracks, and the current track
	IndexT i;
	for (i = 0; i < tracks.pendingAnimations[track].Size(); i++)
		tracks.pendingAnimations[track][i].timeFactor = factor;

	tracks.playingAnimations[track].timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::QueryClips(const Graphics::GraphicsEntityId id, Util::FixedArray<CoreAnimation::AnimClip>& outClips)
{
	const ContextEntityId cid = GetContextId(id);
	const CoreAnimation::AnimResourceId& animRes = characterContextAllocator.Get<AnimationId>(cid.id);
	outClips = CoreAnimation::AnimGetClips(animRes);
}

//------------------------------------------------------------------------------
/**
	Clamp key indices into the valid range, take pre-infinity and
	post-infinity type into account.
*/
IndexT
ClampKeyIndex(IndexT keyIndex, const CoreAnimation::AnimClip& clip)
{
	SizeT clipNumKeys = clip.GetNumKeys();
	if (keyIndex < 0)
	{
		if (clip.GetPreInfinityType() == CoreAnimation::InfinityType::Cycle)
		{
			keyIndex %= clipNumKeys;
			if (keyIndex < 0)
			{
				keyIndex += clipNumKeys;
			}
		}
		else
		{
			keyIndex = 0;
		}
	}
	else if (keyIndex >= clipNumKeys)
	{
		if (clip.GetPostInfinityType() == CoreAnimation::InfinityType::Cycle)
		{
			keyIndex %= clipNumKeys;
		}
		else
		{
			keyIndex = clipNumKeys - 1;
		}
	}
	return keyIndex;
}

//------------------------------------------------------------------------------
/**
	Compute the inbetween-ticks between two frames for a given sample time.
*/
Timing::Tick
InbetweenTicks(Timing::Tick sampleTime, const CoreAnimation::AnimClip& clip)
{
	// normalize sample time into valid time range
	Timing::Tick clipDuration = clip.GetClipDuration();
	if (sampleTime < 0)
	{
		if (clip.GetPreInfinityType() == CoreAnimation::InfinityType::Cycle)
		{
			sampleTime %= clipDuration;
			if (sampleTime < 0)
			{
				sampleTime += clipDuration;
			}
		}
		else
		{
			sampleTime = 0;
		}
	}
	else if (sampleTime >= clipDuration)
	{
		if (clip.GetPostInfinityType() == CoreAnimation::InfinityType::Cycle)
		{
			sampleTime %= clipDuration;
		}
		else
		{
			sampleTime = clipDuration;
		}
	}

	Timing::Tick inbetweenTicks = sampleTime % clip.GetKeyDuration();
	return inbetweenTicks;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	using namespace CoreAnimation;
	const Util::Array<Timing::Time>& times = characterContextAllocator.GetArray<AnimTime>();
	const Util::Array<AnimationTracks>& tracks = characterContextAllocator.GetArray<TrackController>();
	const Util::Array<AnimResourceId>& anims = characterContextAllocator.GetArray<AnimationId>();
	const Util::Array<CoreAnimation::AnimSampleBuffer>& sampleBuffers = characterContextAllocator.GetArray<SampleBuffer>();

	// update times and animations
	IndexT i;
	for (i = 0; i < times.Size(); i++)
	{
		// update time, get track controller
		Timing::Time& currentTime = times[i];
		currentTime += frameTime;
		AnimationTracks& trackController = tracks[i];
		const AnimResourceId& anim = anims[i];

		// loop over all tracks, and update the playing clip on each respective track
		bool firstAnimTrack = true;
		IndexT j;
		for (j = 0; j < MaxNumTracks; j++)
		{
			AnimationRuntime& playing = trackController.playingAnimations[j];

			// if expired, invalidate job
			if (playing.startTime + playing.baseTime + trackController.playingAnimations[j].duration > currentTime)
				trackController.playingAnimations[j].clip = -1;

			// if current playing job on this track is invalid, copy a pending job over
			if (playing.clip == -1)
			{ 
				if (trackController.pendingAnimations[j].Size() > 0)
				{
					playing = trackController.pendingAnimations[j].Back();
					trackController.pendingAnimations[j].EraseBack();
				}
			}

			// this might change from the previous condition, so test it again
			if (playing.clip != -1)
			{
				// update times
				playing.evalTime = ticks - (playing.baseTime + playing.startTime);
				playing.prevEvalTime = playing.evalTime;

				// if not paused, update sample time
				if (!playing.paused)
				{
					int timeDivider = Math::n_frnd(1 / playing.timeFactor);
					Timing::Tick frameTicks = playing.evalTime - playing.prevEvalTime;
					Timing::Tick timeDiff = frameTicks / timeDivider;
					playing.sampleTime = playing.prevSampleTime + timeDiff;
					playing.prevSampleTime = playing.sampleTime;
					playing.sampleTime += playing.timeOffset;
				}

				// start setting up job
				Jobs::JobContext ctx;
				Jobs::JobId job;
				
				if (firstAnimTrack && playing.blend != 1.0f)
					job = Jobs::CreateJob({ AnimSampleJob });
				else
					job = Jobs::CreateJob({ AnimSampleWithMixJob });

				// need to compute the sample weight and pointers to "before" and "after" keys
				const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, playing.clip);
				Timing::Tick keyDuration = clip.GetKeyDuration();
				IndexT keyIndex0 = ClampKeyIndex((time / keyDuration), clip);
				IndexT keyIndex1 = ClampKeyIndex(keyIndex0 + 1, clip);
				Timing::Tick inbetweenTicks = InbetweenTicks(time, clip);

				// create scratch memory
				AnimSampleMixInfo* sampleMixInfo = (AnimSampleMixInfo*)Jobs::JobAllocateScratchMemory(job, Memory::ScratchHeap, sizeof(CoreAnimation::AnimSampleMixInfo));
				Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
				sampleMixInfo->sampleType = SampleType::Linear;
				sampleMixInfo->sampleWeight = float(inbetweenTicks) / float(keyDuration);
				sampleMixInfo->velocityScale.set(playing.timeFactor, playing.timeFactor, playing.timeFactor, 0);

				// get pointers to memory and size
				SizeT src0Size, src1Size;
				const Math::float4 *src0Ptr = nullptr, *src1Ptr = nullptr;
				AnimComputeSlice(anim, playing.clip, keyIndex0, src0Size, src0Ptr);
				AnimComputeSlice(anim, playing.clip, keyIndex1, src1Size, src1Ptr);

				// setup output
				Math::float4* outSamplesPtr = sampleBuffers[i].GetSamplesPointer();
				SizeT numOutSamples = sampleBuffers[i].GetNumSamples();
				SizeT outSamplesByteSize = numOutSamples * sizeof(Math::float4);
				uchar* outSampleCounts = sampleBuffers[i].GetSampleCountsPointer();

				// setup inputs
				ctx.input.data[0] = (void*)src0Ptr;
				ctx.input.dataSize[0] = src0Size;
				ctx.input.sliceSize[0] = src0Size;
				ctx.input.data[1] = (void*)src1Ptr;
				ctx.input.dataSize[1] = src1Size;
				ctx.input.sliceSize[1] = src1Size;
				ctx.input.numBuffers = 2;

				// setup outputs
				ctx.output.data[0] = (void*)outSamplesPtr;
				ctx.output.dataSize[0] = outSamplesByteSize;
				ctx.output.sliceSize[0] = outSamplesByteSize;
				ctx.output.data[1] = (void*)outSampleCounts;
				ctx.output.dataSize[1] = Util::Round::RoundUp16(numOutSamples);
				ctx.output.sliceSize[1] = Util::Round::RoundUp16(numOutSamples);
				ctx.output.numBuffers = 2;

				// setup uniforms
				ctx.uniform.data[0] = &clip.CurveByIndex(0);
				ctx.uniform.dataSize[0] = clip.GetNumCurves() * sizeof(AnimCurve);
				ctx.uniform.data[1] = sampleMixInfo;
				ctx.uniform.dataSize[1] = sizeof(AnimSampleMixInfo);
				ctx.uniform.data[2] = (const void*)playing.mask; 
				ctx.uniform.dataSize[2] = sizeof(AnimSampleMask*);

				// schedule job
				Jobs::JobSchedule(job, CharacterContext::jobPort, ctx);

				// add to delete list
				CharacterContext::runningJobs.Enqueue(job);

				// flip the first anim track flag, which will trigger the next job to mix
				firstAnimTrack = false;
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
CoreAnimation::AnimSampleMask* 
CharacterContext::CreateAnimSampleMask(const Util::StringAtom& name, const Util::FixedArray<Math::scalar>& weights)
{
	IndexT index = CharacterContext::masks.Add(name, CoreAnimation::AnimSampleMask{ name, weights });
	return &CharacterContext::masks.ValueAtIndex(name, index);
}

//------------------------------------------------------------------------------
/**
*/
CoreAnimation::AnimSampleMask*
CharacterContext::GetAnimSampleMask(const Util::StringAtom& name)
{
	IndexT index = CharacterContext::masks.FindIndex(name);
	n_assert(index != InvalidIndex);
	return &CharacterContext::masks.ValueAtIndex(name, index);
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::OnRenderDebug(uint32 flags)
{
}
	
} // namespace Characters

