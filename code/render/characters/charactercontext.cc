//------------------------------------------------------------------------------
//  charactercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "charactercontext.h"
#include "skeleton.h"
#include "characters/streamskeletonpool.h"
#include "coreanimation/animresource.h"
#include "coreanimation/streamanimationpool.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "coreanimation/animsamplemixinfo.h"
#include "coreanimation/animsamplebuffer.h"
#include "util/round.h"
#include "dynui/im3d/im3dcontext.h"
#include "models/nodes/characternode.h"
#include "profiling/profiling.h"
#include "resources/resourceserver.h"

using namespace Graphics;
using namespace Resources;
namespace Characters
{

CharacterContext::CharacterContextAllocator CharacterContext::characterContextAllocator;
_ImplementContext(CharacterContext, CharacterContext::characterContextAllocator);

Jobs::JobPortId CharacterContext::jobPort;
Jobs::JobSyncId CharacterContext::jobSync;
Threading::SafeQueue<Jobs::JobId> CharacterContext::runningJobs;
Util::HashTable<Util::StringAtom, CoreAnimation::AnimSampleMask> CharacterContext::masks;


//------------------------------------------------------------------------------
/**
*/
CharacterContext::CharacterContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
CharacterContext::~CharacterContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Create()
{
    _CreateContext();

    __bundle.OnBegin = CharacterContext::UpdateAnimations;
    __bundle.OnBeforeFrame = CharacterContext::OnAfterFrame;
    __bundle.StageBits = &CharacterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = CharacterContext::OnRenderDebug;
#endif
    CharacterContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
    CharacterContext::jobPort = Graphics::GraphicsServer::renderSystemsJobPort;

    Jobs::CreateJobSyncInfo sinfo =
    {
        nullptr
    };
    CharacterContext::jobSync = Jobs::CreateJobSync(sinfo);

    _CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& skeleton, const Resources::ResourceName& animation, const Util::StringAtom& tag)
{
    const ContextEntityId cid = GetContextId(id);
    n_assert_fmt(cid != InvalidContextEntityId, "Entity %d is not registered in CharacterContext", id.HashCode());
    characterContextAllocator.Get<Loaded>(cid.id) = NoneLoaded;

    // check to make sure we registered this entity for observation, then get the visibility context
    const ContextEntityId visId = Visibility::ObservableContext::GetContextId(id);
    n_assert_fmt(visId != InvalidContextEntityId, "Entity %d needs to be setup as observerable before character!", id.HashCode());
    characterContextAllocator.Get<VisibilityContextId>(cid.id) = id;

    // get model context
    const ContextEntityId mdlId = Models::ModelContext::GetContextId(id);
    n_assert_fmt(mdlId != InvalidContextEntityId, "Entity %d needs to be setup as a model before character!", id.HashCode());
    characterContextAllocator.Get<ModelContextId>(cid.id) = id;

    characterContextAllocator.Get<SkeletonId>(cid.id) = Resources::CreateResource(skeleton, tag, [cid, id](Resources::ResourceId rid)
        {
            characterContextAllocator.Get<SkeletonId>(cid.id) = rid.As<Characters::SkeletonId>();
            characterContextAllocator.Get<Loaded>(cid.id) |= SkeletonLoaded;

            const Util::FixedArray<CharacterJoint>& joints = Characters::SkeletonGetJoints(rid.As<Characters::SkeletonId>());
            characterContextAllocator.Get<JobJoints>(cid.id).Resize(joints.Size());

            // setup joints, scaled joints and user controlled joints
            characterContextAllocator.Get<JointPalette>(cid.id).Resize(joints.Size());
            characterContextAllocator.Get<JointPaletteScaled>(cid.id).Resize(joints.Size());
            characterContextAllocator.Get<UserControlledJoint>(cid.id).Resize(joints.Size());

            // setup job joints
            IndexT i;
            for (i = 0; i < joints.Size(); i++)
            {
                const CharacterJoint& joint = joints[i];
                SkeletonJobJoint& jobJoint = characterContextAllocator.Get<JobJoints>(cid.id)[i];
                jobJoint.parentJointIndex = joint.parentJointIndex;
            }
        }, nullptr, true);

    characterContextAllocator.Get<AnimationId>(cid.id) = Resources::CreateResource(animation, tag, [cid, id](Resources::ResourceId rid)
        {
            characterContextAllocator.Get<AnimationId>(cid.id) = rid.As<CoreAnimation::AnimResourceId>();
            characterContextAllocator.Get<Loaded>(cid.id) |= AnimationLoaded;

            // setup sample buffer when animation is done loading
            characterContextAllocator.Get<SampleBuffer>(cid.id).Setup(rid.As<CoreAnimation::AnimResourceId>());
        }, nullptr, true);

    // clear playing animation state
    IndexT i;
    for (i = 0; i < MaxNumTracks; i++)
    {
        memset(&characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[i], 0, sizeof(AnimationRuntime));
        characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[i].clip = -1;
    }
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
    const CoreAnimation::AnimClip& clip = CoreAnimation::animPool->GetClip(characterContextAllocator.Get<AnimationId>(cid.id), clipIndex);

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
    runtime.duration = Timing::Tick(clip.GetClipDuration() * loopCount * (1 / timeFactor));

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
const bool
IsExpired(const CharacterContext::AnimationRuntime& runtime, const Timing::Time time)
{
    if (runtime.duration == 0)
        return false;
    else
        return runtime.startTime + runtime.baseTime + runtime.duration <= time;
}

//------------------------------------------------------------------------------
/**
*/
const bool
IsInfinite(const CharacterContext::AnimationRuntime& runtime)
{
    return runtime.duration == 0;
}

//------------------------------------------------------------------------------
/**
*/
Timing::Tick
GetAbsoluteStopTime(const CharacterContext::AnimationRuntime& runtime)
{
    n_assert(!IsInfinite(runtime));
    return (runtime.baseTime + runtime.startTime + runtime.duration) - runtime.fadeOutTime;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::UpdateAnimations(const Graphics::FrameContext& ctx)
{
    N_SCOPE(CharacterBeforeFrame, Character);
    using namespace CoreAnimation;
    const Util::Array<Timing::Time>& times = characterContextAllocator.GetArray<AnimTime>();
    const Util::Array<AnimationTracks>& tracks = characterContextAllocator.GetArray<TrackController>();
    const Util::Array<AnimResourceId>& anims = characterContextAllocator.GetArray<AnimationId>();
    const Util::Array<CoreAnimation::AnimSampleBuffer>& sampleBuffers = characterContextAllocator.GetArray<SampleBuffer>();
    const Util::Array<Util::FixedArray<SkeletonJobJoint>>& jobJoints = characterContextAllocator.GetArray<JobJoints>();
    const Util::Array<Characters::SkeletonId>& skeletons = characterContextAllocator.GetArray<SkeletonId>();
    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPalette>();
    const Util::Array<Util::FixedArray<Math::mat4>>& scaledJointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<Util::FixedArray<Math::mat4>>& userJoints = characterContextAllocator.GetArray<UserControlledJoint>();
    const Util::Array<Graphics::GraphicsEntityId>& models = characterContextAllocator.GetArray<ModelContextId>();

    // update times and animations
    IndexT i;
    for (i = 0; i < times.Size(); i++)
    {
        // update time, get track controller
        Timing::Time& currentTime = times[i];
        currentTime += ctx.frameTime;
        AnimationTracks& trackController = tracks[i];
        const AnimResourceId& anim = anims[i];
        const Util::FixedArray<SkeletonJobJoint>& jobJoint = jobJoints[i];
        const Util::FixedArray<Math::mat4>& bindPose = Characters::SkeletonGetBindPose(skeletons[i]);
        const Util::FixedArray<Math::mat4>& userJoint = userJoints[i];
        const Util::FixedArray<Math::mat4>& jointPalette = jointPalettes[i];
        const Util::FixedArray<Math::mat4>& scaledJointPalette = scaledJointPalettes[i];
        const CoreAnimation::AnimSampleBuffer& sampleBuffer = sampleBuffers[i];
        const Graphics::GraphicsEntityId& model = models[i];

        // loop over all tracks, and update the playing clip on each respective track
        bool firstAnimTrack = true;
        IndexT j;
        for (j = 0; j < MaxNumTracks; j++)
        {
            // get playing animation on this track
            AnimationRuntime& playing = trackController.playingAnimations[j];
            bool startedNew = false;

            // update pending animations
            IndexT k;
            for (k = 0; k < trackController.pendingAnimations[j].Size(); k++)
            {
                AnimationRuntime& pending = trackController.pendingAnimations[j][k];
                if (playing.clip == -1)
                {
                    // no clip is playing, this acts as replace automatically
                    pending.baseTime = ctx.time;
                    startedNew = true;
                    playing = pending;
                }
                else
                {
                    if (pending.enqueueMode == Characters::Replace)
                    {
                        pending.baseTime = GetAbsoluteStopTime(playing);
                        startedNew = true;
                        playing = pending;
                    }
                    else if (pending.enqueueMode == Characters::IgnoreIfSame)
                    {
                        if (pending.clip == playing.clip && !IsExpired(playing, currentTime))
                        {
                            // erase clip, because we are already playing it
                            trackController.pendingAnimations[j].EraseIndex(k);
                            k--;
                            continue;
                        }
                        else
                        {
                            // otherwise, act like replace
                            pending.baseTime = GetAbsoluteStopTime(playing);
                            startedNew = true;
                            playing = pending;
                        }
                    }
                    else if (pending.enqueueMode == Characters::Append && IsInfinite(playing))
                    {
                        
#if NEBULA_DEBUG
                        n_error("CharacterContext: Can't insert anim job '%s' because track is blocked by an infinite clip\n", playing.name.AsCharPtr());
#endif
                    }
                    // if none of the above, we have to wait for our current animation to finish
                }

                // if we started a new clip, erase the current pending, and quit the loop
                if (startedNew)
                {
                    trackController.pendingAnimations[j].EraseIndex(k);
                    break;
                }
            }
            
            // if expired, invalidate job
            if (IsExpired(playing, currentTime))
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
                playing.evalTime = ctx.ticks - (playing.baseTime + playing.startTime);
                playing.prevEvalTime = playing.evalTime;
                playing.sampleTime = playing.startTime + ctx.ticks;
                playing.prevSampleTime = playing.sampleTime;

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

                // prepare both an animation job, and a character skeleton job
                Jobs::JobContext ctx[2];
                Jobs::JobId jobs[2];

                // setup two jobs, the first evaluates the animation from the NAX3 resource, 
                // and the next step integrates it with the skeleton, and resolves the new hierarchy
                // this is done through single-slice jobs, one per character
                {
                    // start setting up job
                    if (firstAnimTrack || playing.blend != 1.0f)
                        jobs[0] = Jobs::CreateJob({ AnimSampleJob });
                    else
                        jobs[0] = Jobs::CreateJob({ AnimSampleJobWithMix });

                    // need to compute the sample weight and pointers to "before" and "after" keys
                    const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, playing.clip);
                    Timing::Tick keyDuration = clip.GetKeyDuration();
                    IndexT keyIndex0 = ClampKeyIndex((playing.sampleTime / keyDuration), clip);
                    IndexT keyIndex1 = ClampKeyIndex(keyIndex0 + 1, clip);
                    Timing::Tick inbetweenTicks = InbetweenTicks(playing.sampleTime, clip);

                    // create scratch memory
                    AnimSampleMixInfo* sampleMixInfo = (AnimSampleMixInfo*)Jobs::JobAllocateScratchMemory(jobs[0], Memory::ScratchHeap, sizeof(CoreAnimation::AnimSampleMixInfo));
                    Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
                    sampleMixInfo->sampleType = SampleType::Linear;
                    sampleMixInfo->sampleWeight = float(inbetweenTicks) / float(keyDuration);
                    sampleMixInfo->velocityScale.set(playing.timeFactor, playing.timeFactor, playing.timeFactor, 0);

                    // get pointers to memory and size
                    SizeT src0Size, src1Size;
                    const Math::vec4 *src0Ptr = nullptr, *src1Ptr = nullptr;
                    AnimComputeSlice(anim, playing.clip, keyIndex0, src0Size, src0Ptr);
                    AnimComputeSlice(anim, playing.clip, keyIndex1, src1Size, src1Ptr);

                    // setup output
                    Math::vec4* outSamplesPtr = sampleBuffer.GetSamplesPointer();
                    SizeT numOutSamples = sampleBuffer.GetNumSamples();
                    SizeT outSamplesByteSize = numOutSamples * sizeof(Math::vec4);
                    uchar* outSampleCounts = sampleBuffer.GetSampleCountsPointer();

                    ctx[0].input.numBuffers = 2;
                    ctx[0].output.numBuffers = 2;
                    ctx[0].uniform.numBuffers = 3;

                    // setup inputs
                    ctx[0].input.data[0] = (void*)src0Ptr;
                    ctx[0].input.dataSize[0] = src0Size;
                    ctx[0].input.sliceSize[0] = src0Size;
                    ctx[0].input.data[1] = (void*)src1Ptr;
                    ctx[0].input.dataSize[1] = src1Size;
                    ctx[0].input.sliceSize[1] = src1Size;

                    // setup outputs
                    ctx[0].output.data[0] = (void*)outSamplesPtr;
                    ctx[0].output.dataSize[0] = outSamplesByteSize;
                    ctx[0].output.sliceSize[0] = outSamplesByteSize;
                    ctx[0].output.data[1] = (void*)outSampleCounts;
                    ctx[0].output.dataSize[1] = Util::Round::RoundUp16(numOutSamples);
                    ctx[0].output.sliceSize[1] = Util::Round::RoundUp16(numOutSamples);

                    // setup uniforms
                    ctx[0].uniform.data[0] = &clip.CurveByIndex(0);
                    ctx[0].uniform.dataSize[0] = clip.GetNumCurves() * sizeof(AnimCurve);
                    ctx[0].uniform.data[1] = sampleMixInfo;
                    ctx[0].uniform.dataSize[1] = sizeof(AnimSampleMixInfo);
                    ctx[0].uniform.data[2] = (const void*)playing.mask; 
                    ctx[0].uniform.dataSize[2] = sizeof(AnimSampleMask*);
                    ctx[0].uniform.scratchSize = 0;
                }

                {
                    // create skeleton eval job
                    jobs[1] = Jobs::CreateJob({ SkeletonEvalJobWithVariation });

                    const SizeT elmSize = sizeof(Math::mat4);
                    const SizeT numElements = jobJoint.Size();
                    SizeT outBufSize = numElements * elmSize;

                    ctx[1].input.numBuffers = 2;
                    ctx[1].uniform.numBuffers = 2;
                    ctx[1].output.numBuffers = 2;
                    ctx[1].uniform.scratchSize = outBufSize;

                    // setup inputs
                    ctx[1].input.data[0] = jobJoint.Begin();
                    ctx[1].input.dataSize[0] = numElements * sizeof(SkeletonJobJoint);
                    ctx[1].input.sliceSize[0] = numElements * sizeof(SkeletonJobJoint);
                    ctx[1].input.data[1] = sampleBuffer.GetSamplesPointer();
                    ctx[1].input.dataSize[1] = sampleBuffer.GetNumSamples() * sizeof(Math::vec4);
                    ctx[1].input.sliceSize[1] = sampleBuffer.GetNumSamples() * sizeof(Math::vec4);

                    // setup outputs
                    ctx[1].output.data[0] = scaledJointPalette.Begin();
                    ctx[1].output.dataSize[0] = outBufSize;
                    ctx[1].output.sliceSize[0] = outBufSize;
                    ctx[1].output.data[1] = jointPalette.Begin();
                    ctx[1].output.dataSize[1] = outBufSize;
                    ctx[1].output.sliceSize[1] = outBufSize;

                    // setup uniforms
                    ctx[1].uniform.data[0] = bindPose.Begin();
                    ctx[1].uniform.dataSize[0] = bindPose.Size() * sizeof(Math::mat4);
                    ctx[1].uniform.data[1] = userJoint.Begin();
                    ctx[1].uniform.dataSize[1] = userJoint.Size() * sizeof(Math::mat4);
                }

                // schedule jobs
                Jobs::JobScheduleSequence({ jobs[0], jobs[1] }, CharacterContext::jobPort, { ctx[0], ctx[1] });

                // add to delete list
                CharacterContext::runningJobs.Enqueue(jobs[0]);
                CharacterContext::runningJobs.Enqueue(jobs[1]);

                // flip the first anim track flag, which will trigger the next job to mix
                firstAnimTrack = false;
            }
        }

        // get all character node instances, so we can set their skeleton
        const Util::Array<Models::ModelNode::Instance*>& nodeInstances = Models::ModelContext::GetModelNodeInstances(model);
        for (j = 0; j < nodeInstances.Size(); j++)
        {
            // if type is character node, set the joint palette pointer to this instance of the characater
            // this bridges the gap between the model node and this character instance
            if (nodeInstances[j]->node->type == Models::NodeType::CharacterNodeType)
            {
                Models::CharacterNode::Instance* cinst = static_cast<Models::CharacterNode::Instance*>(nodeInstances[j]);
                cinst->joints = &jointPalette;
            }
        }
    }

    // put sync object
    Jobs::JobSyncSignal(CharacterContext::jobSync, CharacterContext::jobPort);
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::OnAfterFrame(const Graphics::FrameContext& ctx)
{
    if (CharacterContext::runningJobs.Size() > 0)
    {
        Util::Array<Jobs::JobId> jobs;
        jobs.Reserve(CharacterContext::runningJobs.Size());
        CharacterContext::runningJobs.DequeueAll(jobs);

        // wait for all jobs to finish
        Jobs::JobSyncHostWait(CharacterContext::jobSync);

        // destroy jobs
        IndexT i;
        for (i = 0; i < jobs.Size(); i++)
        {
            Jobs::DestroyJob(jobs[i]);
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

#ifndef PUBLIC_BUILD 
//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::OnRenderDebug(uint32 flags)
{
    // wait for jobs to finish
    Jobs::JobSyncHostWait(CharacterContext::jobSync);

    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<Graphics::GraphicsEntityId>& modelContexts = characterContextAllocator.GetArray<ModelContextId>();
    const Math::mat4 scale = Math::scaling(0.1f, 0.1f, 0.1f);
    IndexT i;
    for (i = 0; i < jointPalettes.Size(); i++)
    {
        const Util::FixedArray<Math::mat4>& jointsPalette = jointPalettes[i];
        const Math::mat4& transform = Models::ModelContext::GetTransform(modelContexts[i]);
        IndexT j;
        for (j = 0; j < jointsPalette.Size(); j++)
        {
            Math::mat4 joint = scale * jointsPalette[j] * transform;
            CoreGraphics::RenderShape shape;
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Sphere, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), joint, Math::vec4(1, 0, 0, 0.5f));
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            //Im3d::Im3dContext::DrawSphere(joint, Math::vec4(1,0,0,0.5f));
        }
    }
}
    
#endif

} // namespace Characters

