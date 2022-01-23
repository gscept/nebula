//------------------------------------------------------------------------------
//  charactercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "charactercontext.h"
#include "skeleton.h"
#include "characters/streamskeletoncache.h"
#include "coreanimation/animresource.h"
#include "coreanimation/streamanimationcache.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "coreanimation/animsamplemixinfo.h"
#include "coreanimation/animsamplebuffer.h"
#include "util/round.h"
#include "dynui/im3d/im3dcontext.h"
#include "models/nodes/characternode.h"
#include "models/nodes/characterskinnode.h"
#include "profiling/profiling.h"
#include "resources/resourceserver.h"

using namespace Graphics;
using namespace Resources;
namespace Characters
{

CharacterContext::CharacterContextAllocator CharacterContext::characterContextAllocator;
__ImplementContext(CharacterContext, CharacterContext::characterContextAllocator);

Util::HashTable<Util::StringAtom, CoreAnimation::AnimSampleMask> CharacterContext::masks;
Threading::AtomicCounter CharacterContext::totalCompletionCounter;
Threading::Event CharacterContext::totalCompletionEvent;

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
    __CreateContext();

    __bundle.OnBegin = CharacterContext::UpdateAnimations;
    __bundle.OnBeforeFrame = CharacterContext::WaitForCharacterJobs;
    __bundle.StageBits = &CharacterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = CharacterContext::OnRenderDebug;
#endif
    CharacterContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    __CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& skeleton, const Resources::ResourceName& animation, const Util::StringAtom& tag)
{
    const ContextEntityId cid = GetContextId(id);
    n_assert_fmt(cid != InvalidContextEntityId, "Entity %d is not registered in CharacterContext", id.HashCode());
    characterContextAllocator.Set<Loaded>(cid.id, NoneLoaded);
    characterContextAllocator.Set<EntityId>(cid.id, id);

    // check to make sure we registered this entity for observation, then get the visibility context
    const ContextEntityId visId = Visibility::ObservableContext::GetContextId(id);
    n_assert_fmt(visId != InvalidContextEntityId, "Entity %d needs to be setup as observerable before character!", id.HashCode());

    // get model context
    const ContextEntityId mdlId = Models::ModelContext::GetContextId(id);
    n_assert_fmt(mdlId != InvalidContextEntityId, "Entity %d needs to be setup as a model before character!", id.HashCode());

    const Models::NodeInstanceRange& nodeRange = Models::ModelContext::GetModelRenderableRange(id);
    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();

    for (SizeT i = nodeRange.begin; i < nodeRange.end; i++)
    {
        if (renderables.nodes[i]->GetType() == Models::CharacterSkinNodeType)
        {
            // Save relative id
            characterContextAllocator.Set<CharacterSkinNodeIndex>(cid.id, i - nodeRange.begin);
            break;
        }
    }

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
    const Util::Array<Graphics::GraphicsEntityId>& models = characterContextAllocator.GetArray<EntityId>();

    // Increment total counter
    n_assert(CharacterContext::totalCompletionCounter == 0);
    CharacterContext::totalCompletionCounter = models.Size();

    // update times and animations
    IndexT i;
    for (i = 0; i < models.Size(); i++)
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
        bool runSkeletonThisFrame = false;

        // Start job sequence over all tracks
        Jobs2::JobBeginSequence({});

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
                    int timeDivider = Math::frnd(1 / playing.timeFactor);
                    Timing::Tick frameTicks = playing.evalTime - playing.prevEvalTime;
                    Timing::Tick timeDiff = frameTicks / timeDivider;
                    playing.sampleTime = playing.prevSampleTime + timeDiff;
                    playing.prevSampleTime = playing.sampleTime;
                    playing.sampleTime += playing.timeOffset;
                }

                // Sequence job to sample animation resource
                struct AnimContext
                {
                    const AnimCurve* animCurves;
                    uint numCurves;
                    const AnimSampleMixInfo* mixInfo;
                    const AnimSampleMask* mask;
                    const Math::vec4* src0SamplePtr;
                    const Math::vec4* src1SamplePtr;
                    const Math::vec4* mixSamplePtr;
                    uchar* mixSampleCounts;
                    Math::vec4* tmpSamplePtr;
                    uchar* tmpSampleCounts;
                    Math::vec4* outSamplePtr;
                    uchar* outSampleCounts;
                    bool mix;

                } animCtx;

                // Need to compute the sample weight and pointers to "before" and "after" keys
                const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, playing.clip);
                Timing::Tick keyDuration = clip.GetKeyDuration();
                IndexT keyIndex0 = ClampKeyIndex((playing.sampleTime / keyDuration), clip);
                IndexT keyIndex1 = ClampKeyIndex(keyIndex0 + 1, clip);
                Timing::Tick inbetweenTicks = InbetweenTicks(playing.sampleTime, clip);

                // Create scratch memory
                auto sampleMixInfo = Jobs2::JobAlloc<AnimSampleMixInfo>(1);
                Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
                sampleMixInfo->sampleType = SampleType::Linear;
                sampleMixInfo->sampleWeight = float(inbetweenTicks) / float(keyDuration);
                sampleMixInfo->velocityScale.set(playing.timeFactor, playing.timeFactor, playing.timeFactor, 0);

                // Get pointers to memory and size
                SizeT src0Size, src1Size;
                const Math::vec4* src0Ptr = nullptr, *src1Ptr = nullptr;
                AnimComputeSlice(anim, playing.clip, keyIndex0, src0Size, src0Ptr);
                AnimComputeSlice(anim, playing.clip, keyIndex1, src1Size, src1Ptr);

                animCtx.animCurves = &clip.CurveByIndex(0);
                animCtx.numCurves = clip.GetNumCurves();
                animCtx.mixInfo = sampleMixInfo;
                animCtx.src0SamplePtr = src0Ptr;
                animCtx.src1SamplePtr = src1Ptr;
                animCtx.mask = playing.mask;
                animCtx.outSamplePtr = sampleBuffer.GetSamplesPointer();
                animCtx.outSampleCounts = sampleBuffer.GetSampleCountsPointer();

                if (firstAnimTrack || playing.blend != 1.0f)
                {
                    animCtx.mix = false;
                    animCtx.mixSampleCounts = nullptr;
                    animCtx.mixSamplePtr = nullptr;
                }
                else
                {
                    animCtx.mix = true;
                    animCtx.mixSamplePtr = animCtx.outSamplePtr;
                    animCtx.mixSampleCounts = animCtx.outSampleCounts;
                }

                Jobs2::JobAppendSequence([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
                {
                    N_SCOPE_ACCUM(AnimationSample, Graphics);
                    n_assert(totalJobs == 1);
                    auto context = static_cast<AnimContext*>(ctx);
                        
                    const Math::vec4* src0SamplePtr = context->src0SamplePtr;
                    const Math::vec4* src1SamplePtr = context->src1SamplePtr;
                    Math::vec4* outSamplePtr = context->outSamplePtr;
                    uchar* outSampleCounts = context->outSampleCounts;

                    if (context->mix)
                    {
                        const Math::vec4* mixSamplePtr = context->mixSamplePtr;
                        uchar* mixSampleCounts = context->mixSampleCounts;
                        Math::vec4* tmpSamplePtr = context->tmpSamplePtr;
                        uchar* tmpSampleCounts = context->tmpSampleCounts;

                        if (context->mixInfo->sampleType == SampleType::Step)
                            AnimSampleStep(context->animCurves, context->numCurves, context->mixInfo->velocityScale, src0SamplePtr, tmpSamplePtr, tmpSampleCounts);
                        else
                            AnimSampleLinear(context->animCurves, context->numCurves, context->mixInfo->sampleWeight, context->mixInfo->velocityScale, src0SamplePtr, src1SamplePtr, tmpSamplePtr, tmpSampleCounts);

                        if (context->mix)
                            AnimMix(context->animCurves, context->numCurves, context->mask, context->mixInfo->mixWeight, mixSamplePtr, tmpSamplePtr, mixSampleCounts, tmpSampleCounts, outSamplePtr, context->outSampleCounts);
                    }
                    else
                    {
                        if (context->mixInfo->sampleType == SampleType::Step)
                            AnimSampleStep(context->animCurves, context->numCurves, context->mixInfo->velocityScale, src0SamplePtr, context->outSamplePtr, context->outSampleCounts);
                        else
                            AnimSampleLinear(context->animCurves, context->numCurves, context->mixInfo->sampleWeight, context->mixInfo->velocityScale, src0SamplePtr, src1SamplePtr, context->outSamplePtr, context->outSampleCounts);
                    }
                }, 1, animCtx);

                // Run skeleton as soon as we have a single anim job queued
                runSkeletonThisFrame = true;

                // flip the first anim track flag, which will trigger the next job to mix
                firstAnimTrack = false;
            }
        }

        if (runSkeletonThisFrame)
        {
        // When all animations are sequenced, add a character evaluation job
            struct CharJob
            {
                const SkeletonJobJoint* joints;
                uint numJoints;
                const Math::vec4* samples;
                uint numSamples;
                uchar* samplesCountPointer;
                const Math::mat4* bindPoseJoints;
                const Math::mat4* userJoints;
                Math::mat4* scratch;

                Math::mat4* outScaledJoints;
                Math::mat4* outJoints;

            } charJob;
            charJob.joints = jobJoint.Begin();
            charJob.numJoints = jobJoint.Size();
            charJob.samples = sampleBuffer.GetSamplesPointer();
            charJob.numSamples = sampleBuffer.GetNumSamples();
            charJob.samplesCountPointer = sampleBuffer.GetSampleCountsPointer();
            charJob.bindPoseJoints = bindPose.Begin();
            charJob.userJoints = userJoint.Begin();
            charJob.outScaledJoints = scaledJointPalette.Begin();
            charJob.outJoints = jointPalette.Begin();
            charJob.scratch = Jobs2::JobAlloc<Math::mat4>(jobJoint.Size());

            Jobs2::JobAppendSequence([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
            {
                N_SCOPE_ACCUM(SkeletonEval, Graphics);
                n_assert(totalJobs == 1);
                auto context = static_cast<CharJob*>(ctx);
                const Math::mat4* invPoseMatrixBase = context->bindPoseJoints;
                const Math::mat4* mixPoseMatrixBase = context->userJoints;
                Math::mat4* unscaledMatrixBase = context->scratch;

                Math::vec4 finalScale, parentFinalScale, variationTranslation;
                Math::vec4 translate(0.0f, 0.0f, 0.0f, 1.0f);
                Math::vec4 scale(1.0f, 1.0f, 1.0f, 0.0f);
                Math::vec4 parentScale(1.0f, 1.0f, 1.0f, 0.0f);
                Math::vec4 parentTranslate(0.0f, 0.0f, 0.0f, 1.0f);
                Math::quat rotate;
                Math::vec4 vec1111(1.0f, 1.0f, 1.0f, 1.0f);

                // load pointers from context
                // NOTE: the samplesBase pointer may be NULL if no valid animation
                // data exists, in this case the skeleton should simply be set
                // to its jesus pose
                const SkeletonJobJoint* compsBase = context->joints;
                n_assert(0 != compsBase);
                const Math::vec4* samplesBase = context->samples;
                const Math::vec4* samplesPtr = samplesBase;

                Math::mat4* scaledMatrixBase = context->outScaledJoints;
                Math::mat4* skinMatrixBase = context->outJoints;

                // input samples may optionally include velocity samples which we need to skip...
                uint sampleWidth = (context->numSamples / context->numJoints);

                // compute number of joints
                int jointIndex;
                for (jointIndex = 0; jointIndex < context->numJoints; jointIndex++)
                {
                    // load joint translate/rotate/scale
                    if (context->samplesCountPointer[0] != 0)
                    {
                        translate.load((Math::scalar*)&(samplesPtr[0]));
                        rotate.load((Math::scalar*)&(samplesPtr[1]));
                        scale.load((Math::scalar*)&(samplesPtr[2]));
                        samplesPtr += sampleWidth;
                    }

                    const SkeletonJobJoint& comps = compsBase[jointIndex];

                    // load variation scale
                    finalScale.load(&comps.varScaleX);
                    finalScale = scale * finalScale;
                    finalScale = permute(finalScale, vec1111, 0, 1, 2, 7);

                    Math::mat4& unscaledMatrix = unscaledMatrixBase[jointIndex];
                    Math::mat4& scaledMatrix = scaledMatrixBase[jointIndex];

                    // update unscaled matrix
                    // animation rotation
                    unscaledMatrix = rotationquat(rotate);

                    // load variation translation
                    variationTranslation.load(&comps.varTranslationX);
                    unscaledMatrix.translate(xyz(translate + variationTranslation));

                    // add mix pose if the pointer is set
                    if (mixPoseMatrixBase)
                        unscaledMatrix = unscaledMatrix * mixPoseMatrixBase[jointIndex];

                    // update scaled matrix
                    // scale after rotation
                    scaledMatrix.x_axis = unscaledMatrix.x_axis * splat_x(finalScale);
                    scaledMatrix.y_axis = unscaledMatrix.y_axis * splat_y(finalScale);
                    scaledMatrix.z_axis = unscaledMatrix.z_axis * splat_z(finalScale);

                    if (InvalidIndex == comps.parentJointIndex)
                    {
                        // no parent directly set translation
                        scaledMatrix.position = unscaledMatrix.position;
                    }
                    else
                    {
                        // load parent animation scale
                        parentScale.load((Math::scalar*)(samplesBase + sampleWidth * jointIndex + 2));
                        const SkeletonJobJoint& parentComps = compsBase[comps.parentJointIndex];

                        // load parent variation scale
                        parentFinalScale.load(&parentComps.varScaleX);

                        // combine both scaling types
                        parentFinalScale = parentScale * parentFinalScale;
                        parentFinalScale = permute(parentFinalScale, vec1111, 0, 1, 2, 7);

                        // transform our unscaled position with parent scaling
                        unscaledMatrix.position = unscaledMatrix.position * parentFinalScale;
                        scaledMatrix.position = unscaledMatrix.position;

                        // apply rotation and relative animation translation of parent 
                        const Math::mat4& parentUnscaledMatrix = unscaledMatrixBase[comps.parentJointIndex];
                        unscaledMatrix = unscaledMatrix * parentUnscaledMatrix;
                        scaledMatrix = scaledMatrix * parentUnscaledMatrix;
                    }
                    skinMatrixBase[jointIndex] = invPoseMatrixBase[jointIndex] * scaledMatrix;
                }

                // Signal final event
                if (Threading::Interlocked::Decrement(&CharacterContext::totalCompletionCounter) == 0)
                    CharacterContext::totalCompletionEvent.Signal();

            }, 1, charJob);
        }
        else
        {
            // Since we preemptively set the counter to update all skeletons, we need to remove one counter here
            if (Threading::Interlocked::Decrement(&CharacterContext::totalCompletionCounter) == 0)
                CharacterContext::totalCompletionEvent.Signal();
        }

        // End sequence
        Jobs2::JobEndSequence();
    }

    // In case we have 0 systems but have the context running, just signal the event immediately 
    if (CharacterContext::totalCompletionCounter == 0)
        CharacterContext::totalCompletionEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::WaitForCharacterJobs(const Graphics::FrameContext& ctx)
{
    CharacterContext::totalCompletionEvent.Wait();
    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPalette>();
    const Util::Array<Util::FixedArray<Math::mat4>>& scaledJointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<IndexT>& characterSkinNodeIndices = characterContextAllocator.GetArray<CharacterSkinNodeIndex>();
    const Util::Array<Graphics::GraphicsEntityId> graphicsEntities = characterContextAllocator.GetArray<EntityId>();

    for (IndexT i = 0; i < characterSkinNodeIndices.Size(); i++)
    {
        // Update skeleton constants
        // Move the skin fragment extraction to a job and have that output the list of actually used matrices.
        const Models::NodeInstanceRange& range = Models::ModelContext::GetModelRenderableRange(graphicsEntities[i]);
        const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();

        const Util::FixedArray<Math::mat4>& jointPalette = jointPalettes[i];
        IndexT node = range.begin + characterSkinNodeIndices[i];
        n_assert(renderables.nodes[node]->type == Models::NodeType::CharacterSkinNodeType);
        Models::CharacterSkinNode* sparent = reinterpret_cast<Models::CharacterSkinNode*>(renderables.nodes[node]);
        const Util::Array<IndexT>& usedIndices = sparent->skinFragments[0].jointPalette;
        Util::FixedArray<Math::mat4> usedMatrices(usedIndices.Size());

        // update joints, which is stored in character context
        if (!jointPalette.IsEmpty())
        {
            // copy active matrix palette, or set identity
            IndexT j;
            for (j = 0; j < usedIndices.Size(); j++)
            {
                usedMatrices[j] = jointPalette[usedIndices[j]];
            }
        }
        else
        {
            // copy active matrix palette, or set identity
            IndexT j;
            for (j = 0; j < usedIndices.Size(); j++)
            {
                usedMatrices[j] = Math::mat4();
            }
        }

        // update skinning palette
        uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, usedMatrices.Begin(), usedMatrices.Size());
        renderables.nodeStates[node].resourceTableOffsets[renderables.nodeStates[node].skinningConstantsIndex] = offset;
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
    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<Graphics::GraphicsEntityId>& modelContexts = characterContextAllocator.GetArray<EntityId>();
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

