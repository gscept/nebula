//------------------------------------------------------------------------------
//  charactercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "charactercontext.h"
#include "skeletonresource.h"
#include "coreanimation/animationresource.h"
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
Threading::Event CharacterContext::totalCompletionEvent;
Threading::AtomicCounter CharacterContext::ConstantUpdateCounter = 0;

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
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = CharacterContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::Setup(
    const Graphics::GraphicsEntityId id
    , const Resources::ResourceName& skeletonResource
    , const IndexT skeletonIndex
    , const Resources::ResourceName& animationResource
    , const IndexT animationIndex
    , const Util::StringAtom& tag
    , bool supportBlending
)
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
            // Save offset to the character node in the model
            characterContextAllocator.Set<CharacterSkinNodeIndexOffset>(cid.id, i - nodeRange.begin);
            break;
        }
    }

    characterContextAllocator.Get<Skeleton>(cid.id) = Characters::InvalidSkeletonId;
    Resources::CreateResource(skeletonResource, tag, [cid, id, skeletonIndex](Resources::ResourceId rid)
        {
            auto skeleton = SkeletonResourceGetSkeleton(rid, skeletonIndex);
            characterContextAllocator.Set<Skeleton>(cid.id, skeleton);
            characterContextAllocator.Get<Loaded>(cid.id) |= SkeletonLoaded;

            const Util::FixedArray<CharacterJoint>& joints = Characters::SkeletonGetJoints(skeleton);
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

    characterContextAllocator.Get<Animation>(cid.id) = CoreAnimation::InvalidAnimationId;
    Resources::CreateResource(animationResource, tag, [cid, id, animationIndex, supportBlending](Resources::ResourceId rid)
        {
            auto animation = CoreAnimation::AnimationResourceGetAnimation(rid, animationIndex);
            characterContextAllocator.Set<Animation>(cid.id, animation);
            characterContextAllocator.Get<Loaded>(cid.id) |= AnimationLoaded;

            // setup sample buffer when animation is done loading
            characterContextAllocator.Get<SampleBuffer>(cid.id).Setup(animation);
            characterContextAllocator.Set<SupportMix>(cid.id, supportBlending);
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
    return CoreAnimation::AnimGetIndex(characterContextAllocator.Get<Animation>(cid.id), name);
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
    const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(characterContextAllocator.Get<Animation>(cid.id), clipIndex);

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
    runtime.curveSampleIndices.Resize(clip.numCurves);
    runtime.curveSampleIndices.Fill(0, clip.numCurves, 0x0);
    runtime.mask = mask;

    runtime.baseTime = 0;
    runtime.duration = 0;
    runtime.evalTime = runtime.prevEvalTime = 0;
    runtime.sampleTime = runtime.prevSampleTime = 0;
    runtime.paused = false;
    runtime.duration = Timing::Tick(clip.duration * loopCount * (1 / timeFactor));

    // append to pending animations
    if (mode == Replace)
        characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track] = std::move(runtime);
    else if (mode == Append)
        characterContextAllocator.Get<TrackController>(cid.id).pendingAnimations[track].Append(std::move(runtime));
    else if (mode == IgnoreIfSame)
    {
        const AnimationRuntime& current = characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track];

        // if clip index doesn't match, append animation
        if (current.clip != clipIndex)
            characterContextAllocator.Get<TrackController>(cid.id).pendingAnimations[track].Append(std::move(runtime));
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
    const CoreAnimation::AnimationId& animRes = characterContextAllocator.Get<Animation>(cid.id);
    outClips = CoreAnimation::AnimGetClips(animRes);
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

struct CharacterJobContext
{
    const Util::Array<Timing::Time>* times;
    const Util::Array<CharacterContext::AnimationTracks>* tracks;
    const Util::Array<CoreAnimation::AnimationId>* anims;
    const Util::Array<CoreAnimation::AnimSampleBuffer>* sampleBuffers;
    const Util::Array<Util::FixedArray<SkeletonJobJoint>>* jobJoints;
    const Util::Array<SkeletonId>* skeletons;
    const Util::Array<Util::FixedArray<Math::mat4>>* jointPalettes;
    const Util::Array<Util::FixedArray<Math::mat4>>* scaledJointPalettes;
    const Util::Array<Util::FixedArray<Math::mat4>>* userJoints;
    float** tmpSamples;
    uint** tmpSampleIndices;
    Math::mat4** tmpJoints;
    
    const Util::Array<Graphics::GraphicsEntityId>* entities;
    CoreAnimation::AnimSampleMixInfo* animMixInfos;
    float frameTime;
    Timing::Tick time;
    Timing::Tick ticks;
};

//------------------------------------------------------------------------------
/**
*/
void
EvalCharacter(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
{
    N_SCOPE(EvalCharacter, Graphics);
    auto context = static_cast<CharacterJobContext*>(ctx);
    using namespace CoreAnimation;

    for (IndexT i = 0; i < groupSize; i++)
    {
        IndexT index = invocationOffset + i;
        if (index >= totalJobs)
            return;

        // update time, get track controller
        Timing::Time& currentTime = context->times->Get(index);
        currentTime += context->frameTime;
        CharacterContext::AnimationTracks& trackController = context->tracks->Get(index);
        const AnimationId anim = context->anims->Get(index);
        if (anim == InvalidAnimationId)
            continue;
        const Util::FixedArray<SkeletonJobJoint>& jobJoint = context->jobJoints->Get(index);
        const SkeletonId skeleton = context->skeletons->Get(index);
        if (skeleton == InvalidSkeletonId)
            continue;
        const Util::FixedArray<Math::mat4>& bindPose = Characters::SkeletonGetBindPose(skeleton);
        const Util::FixedArray<Math::mat4>& userJoint = context->userJoints->Get(index);
        const Util::FixedArray<Math::mat4>& jointPalette = context->jointPalettes->Get(index);
        const Util::FixedArray<Math::mat4>& scaledJointPalette = context->scaledJointPalettes->Get(index);
        const Util::FixedArray<Math::vec4>& idleSamples = Characters::SkeletonGetIdleSamples(skeleton);
        const CoreAnimation::AnimSampleBuffer& sampleBuffer = context->sampleBuffers->Get(index);
        const Graphics::GraphicsEntityId& model = context->entities->Get(index);
        Math::mat4* tmpMatrices = context->tmpJoints[index];
        float* tmpSamples = context->tmpSamples[index];
        uint* tmpSampleIndices = context->tmpSampleIndices[index];
        auto sampleMixInfo = context->animMixInfos + index;
        bool runSkeletonThisFrame = false;

        // loop over all tracks, and update the playing clip on each respective track
        bool firstAnimTrack = true;
        IndexT j;
        for (j = 0; j < CharacterContext::MaxNumTracks; j++)
        {
            // get playing animation on this track
            CharacterContext::AnimationRuntime& playing = trackController.playingAnimations[j];
            bool startedNew = false;

            // update pending animations
            IndexT k;
            for (k = 0; k < trackController.pendingAnimations[j].Size(); k++)
            {
                CharacterContext::AnimationRuntime& pending = trackController.pendingAnimations[j][k];
                if (playing.clip == -1)
                {
                    // no clip is playing, this acts as replace automatically
                    pending.baseTime = context->time;
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
                playing.evalTime = context->ticks - (playing.baseTime + playing.startTime);
                playing.prevEvalTime = playing.evalTime;
                playing.sampleTime = playing.startTime + context->ticks;
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

                // Need to compute the sample weight and pointers to "before" and "after" keys
                const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, playing.clip);
                const Ptr<AnimKeyBuffer>& buffer = AnimGetBuffer(anim);

                // Create scratch memory
                Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
                sampleMixInfo->sampleType = SampleType::Linear;
                sampleMixInfo->velocityScale.set(playing.timeFactor, playing.timeFactor, playing.timeFactor, 0);

                // Get pointers to memory and size
                const float* srcPtr = buffer->GetKeyBufferPointer();
                const AnimKeyBuffer::Interval* srcTimePtr = buffer->GetIntervalBufferPointer();

                const Util::FixedArray<AnimCurve>& curves = CoreAnimation::AnimGetCurves(anim);
                Timing::Tick evalTime = playing.sampleTime % clip.duration;

                if (firstAnimTrack
                    || playing.blend != 1.0f)
                {
                    if (sampleMixInfo->sampleType == SampleType::Step)
                        AnimSampleStep(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, playing.curveSampleIndices.Begin(), sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
                    else
                        AnimSampleLinear(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, playing.curveSampleIndices.Begin(), sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
                }
                else // Playing with mix
                {
                    uchar tmpSampleCounts = 0;
                    if (sampleMixInfo->sampleType == SampleType::Step)
                        AnimSampleStep(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, tmpSampleIndices, tmpSamples, &tmpSampleCounts);
                    else
                        AnimSampleLinear(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, tmpSampleIndices, tmpSamples, &tmpSampleCounts);

                    AnimMix(clip, curves.Size(), playing.mask, sampleMixInfo->mixWeight, sampleBuffer.GetSamplesPointer(), tmpSamples, sampleBuffer.GetSampleCountsPointer(), &tmpSampleCounts, sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
                }

                // Run skeleton as soon as we have a single anim job queued
                runSkeletonThisFrame = true;

                // flip the first anim track flag, which will trigger the next job to mix
                firstAnimTrack = false;
            }

        }

        // If we have no animations, just skip the character skeleton update
        if (!runSkeletonThisFrame)
            continue;

        // Evaluate skeleton
        const Math::mat4* invPoseMatrixBase = bindPose.Begin();
        const Math::mat4* mixPoseMatrixBase = userJoint.Begin();
        Math::mat4* unscaledMatrixBase = tmpMatrices;

        Math::vec3 translate(0.0f, 0.0f, 0.0f);
        Math::vec3 scale(1.0f, 1.0f, 1.0f);
        Math::quat rotate;

        // load pointers from context
        // NOTE: the samplesBase pointer may be NULL if no valid animation
        // data exists, in this case the skeleton should simply be set
        // to its jesus pose
        const SkeletonJobJoint* compsBase = jobJoint.Begin();
        n_assert(0 != compsBase);
        const float* samplesBase = sampleBuffer.GetSamplesPointer();
        const float* samplesPtr = samplesBase;

        const uchar* sampleCountBase = sampleBuffer.GetSampleCountsPointer();
        const uchar* sampleCountPtr = sampleCountBase;

        Math::mat4* scaledMatrixBase = scaledJointPalette.Begin();
        Math::mat4* skinMatrixBase = jointPalette.Begin();

        // input samples may optionally include velocity samples which we need to skip...
        uint sampleWidth = (sampleBuffer.GetNumSamples() / jointPalette.Size());

        // compute number of joints
        int jointIndex;
        for (jointIndex = 0; jointIndex < jointPalette.Size(); jointIndex++)
        {
            static const size_t TRANSLATION_OFFSET = 0;
            static const size_t ROTATION_OFFSET = 3;
            static const size_t SCALE_OFFSET = 7;

            translate.load(samplesPtr + TRANSLATION_OFFSET);
            rotate.load(samplesPtr + ROTATION_OFFSET);
            scale.load(samplesPtr + SCALE_OFFSET);

            sampleCountPtr += 3;
            samplesPtr += sampleWidth;

            const SkeletonJobJoint& comps = compsBase[jointIndex];

            Math::mat4& unscaledMatrix = unscaledMatrixBase[jointIndex];
            Math::mat4& scaledMatrix = scaledMatrixBase[jointIndex];

            // update unscaled matrix
            // animation rotation
            scaledMatrix = Math::affine(scale, rotate, translate);
            unscaledMatrix = Math::affine(Math::vec3(1), rotate, translate);
            if (InvalidIndex != comps.parentJointIndex)
            {
                const Math::mat4& parentUnscaledMatrix = unscaledMatrixBase[comps.parentJointIndex];
                scaledMatrix = parentUnscaledMatrix * scaledMatrix;
                unscaledMatrix = parentUnscaledMatrix * unscaledMatrix;
            }

            skinMatrixBase[jointIndex] = scaledMatrix * invPoseMatrixBase[jointIndex];
        }
    }
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
    const Util::Array<AnimationId>& anims = characterContextAllocator.GetArray<Animation>();
    const Util::Array<CoreAnimation::AnimSampleBuffer>& sampleBuffers = characterContextAllocator.GetArray<SampleBuffer>();
    const Util::Array<Util::FixedArray<SkeletonJobJoint>>& jobJoints = characterContextAllocator.GetArray<JobJoints>();
    const Util::Array<SkeletonId>& skeletons = characterContextAllocator.GetArray<Skeleton>();
    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPalette>();
    const Util::Array<Util::FixedArray<Math::mat4>>& scaledJointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<Util::FixedArray<Math::mat4>>& userJoints = characterContextAllocator.GetArray<UserControlledJoint>();
    const Util::Array<Graphics::GraphicsEntityId>& models = characterContextAllocator.GetArray<EntityId>();
    const Util::Array<bool>& supportsBlending = characterContextAllocator.GetArray<SupportMix>();
    const Util::Array<IndexT>& characterSkinNodeIndices = characterContextAllocator.GetArray<CharacterSkinNodeIndexOffset>();

    if (!models.IsEmpty())
    {
        static Threading::AtomicCounter animationCounter = 0;

        // Set total counter
        n_assert(animationCounter == 0);
        animationCounter = 1;

        CharacterJobContext charCtx;
        charCtx.times = &times;
        charCtx.tracks = &tracks;
        charCtx.anims = &anims;
        charCtx.sampleBuffers = &sampleBuffers;
        charCtx.jobJoints = &jobJoints;
        charCtx.skeletons = &skeletons;
        charCtx.jointPalettes = &jointPalettes;
        charCtx.scaledJointPalettes = &scaledJointPalettes;
        charCtx.userJoints = &userJoints;
        charCtx.entities = &models;
        charCtx.frameTime = ctx.frameTime;
        charCtx.ticks = ctx.ticks;
        charCtx.time = ctx.time;
        charCtx.animMixInfos = Jobs2::JobAlloc<AnimSampleMixInfo>(models.Size());

        charCtx.tmpJoints = Jobs2::JobAlloc<Math::mat4*>(models.Size());
        charCtx.tmpSampleIndices = Jobs2::JobAlloc<uint*>(models.Size());
        charCtx.tmpSamples = Jobs2::JobAlloc<float*>(models.Size());

        IndexT i;
        for (i = 0; i < models.Size(); i++)
        {
            if (models[i] == Graphics::InvalidGraphicsEntityId)
                continue;

            // Allocate scratch memory for character transforms
            const Util::FixedArray<Math::mat4>& jointPalette = jointPalettes[i];
            charCtx.tmpJoints[i] = Jobs2::JobAlloc<Math::mat4>(jointPalette.Size());

            // Allocate scratch memory for animation mixing
            const CoreAnimation::AnimSampleBuffer& sampleBuffer = sampleBuffers[i];
            if (supportsBlending[i])
            {
                charCtx.tmpSampleIndices[i] = Jobs2::JobAlloc<uint>(sampleBuffer.GetNumSamples());
                charCtx.tmpSamples[i] = Jobs2::JobAlloc<float>(sampleBuffer.GetNumSamples());
            }
        }

        // Run job
        Jobs2::JobDispatch(EvalCharacter, models.Size(), 64, charCtx, nullptr, &animationCounter, nullptr);

        n_assert(ConstantUpdateCounter == 0);
        ConstantUpdateCounter = 1;

        struct UpdateJointsContext
        {
            const Util::Array<IndexT>* characterNodeIndices;
            const Util::Array<Util::FixedArray<Math::mat4>>* jointPalettes;
            const Util::Array<Graphics::GraphicsEntityId>* entities;
        } jobCtx;
        jobCtx.characterNodeIndices = &characterSkinNodeIndices;
        jobCtx.entities = &models;
        jobCtx.jointPalettes = &jointPalettes;

        // Run job to update constants
        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(JointConstantsUpdate, Graphics);
            auto jobCtx = static_cast<UpdateJointsContext*>(ctx);
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = invocationOffset + i;
                if (index >= totalJobs)
                    return;

                const Graphics::GraphicsEntityId entity = jobCtx->entities->Get(index);
                if (entity == Graphics::InvalidGraphicsEntityId)
                    continue;

                const Models::NodeInstanceRange& range = Models::ModelContext::GetModelRenderableRange(entity);
                const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();

                const Util::FixedArray<Math::mat4>& jointPalette = jobCtx->jointPalettes->Get(index);
                IndexT node = range.begin + jobCtx->characterNodeIndices->Get(index);
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
                        const IndexT lookup = usedIndices[j];
                        usedMatrices[j] = jointPalette[lookup];
                    }
                }

                // Update skinning palette
                uint offset = CoreGraphics::SetConstants(usedMatrices.Begin(), usedMatrices.Size());
                renderables.nodeStates[node].resourceTableOffsets[renderables.nodeStates[node].skinningConstantsIndex] = offset;
            }

        }, characterSkinNodeIndices.Size(), 64, jobCtx, { &animationCounter }, &ConstantUpdateCounter, &CharacterContext::totalCompletionEvent);
    }
    else // If we have no jobs, just signal completion event
        CharacterContext::totalCompletionEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterContext::WaitForCharacterJobs(const Graphics::FrameContext& ctx)
{
    N_MARKER_BEGIN(WaitForCharacter, Graphics);
    CharacterContext::totalCompletionEvent.Wait();
    N_MARKER_END();
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
            Math::mat4 joint = transform * jointsPalette[j];
            Math::mat4 mat;
            mat.row0 = normalize3(joint.row0);
            mat.row1 = normalize3(joint.row1);
            mat.row2 = normalize3(joint.row2);
            mat.row3 = normalize3(joint.row3);
            mat.row3.w = 1.0f;
            mat.position = joint.position;
            mat = mat * scale;
            CoreGraphics::RenderShape shape;
            shape.SetupSimpleShape(CoreGraphics::RenderShape::Sphere, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::AlwaysOnTop), Math::vec4(1, 0, 0, 0.5f), mat);
            CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            //Im3d::Im3dContext::DrawSphere(joint, Math::vec4(1,0,0,0.5f));
        }
    }
}
    
#endif

} // namespace Characters

