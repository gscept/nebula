//------------------------------------------------------------------------------
//  charactercontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "charactercontext.h"
#include "skeletonresource.h"
#include "coreanimation/animationresource.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "coreanimation/animsamplemixinfo.h"
#include "coreanimation/animsamplebuffer.h"
#include "models/nodes/characterskinnode.h"
#include "profiling/profiling.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "coreanimation/animationloader.h"
#include "models/modelloader.h"

using namespace Graphics;
using namespace Resources;
namespace Characters
{

CharacterContext::CharacterContextAllocator CharacterContext::characterContextAllocator;
__ImplementContext(CharacterContext, CharacterContext::characterContextAllocator);

Util::HashTable<Util::StringAtom, CoreAnimation::AnimSampleMask> CharacterContext::masks;
Threading::Event CharacterContext::totalCompletionEvent;
Threading::Interlocked::AtomicCounter CharacterContext::ConstantUpdateCounter = 0;

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
    Copy the joint masks of the model of a character, they're kept with the character so the clips playing with them
    can point to them
*/
static void
FetchJointMasks(const Graphics::GraphicsEntityId id, Util::FixedArray<CoreAnimation::AnimSampleMask>& outMasks)
{
    const Models::ModelId model = Models::ModelContext::GetModel(id);
    if (model == Models::InvalidModelId)
        return;
    const Util::FixedArray<Models::JointMask>& masks = Models::ModelGetJointMasks(model);
    outMasks.Resize(masks.Size());
    for (IndexT i = 0; i < masks.Size(); i++)
    {
        outMasks[i].name = masks[i].name;
        outMasks[i].weights = masks[i].weights;
    }
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

#if WITH_NEBULA_EDITOR
    // a reloaded animation has animations of its own, the characters playing it fetch them again. Its clips may
    // not be the ones played anymore, so the tracks are stopped
    CoreAnimation::AnimationLoader* loader = Resources::ResourceServer::Instance()->GetStreamLoader<CoreAnimation::AnimationLoader>();
    loader->AddReloadListener([](Resources::ResourceId id)
    {
        const Util::Array<Resources::ResourceId>& resources = characterContextAllocator.GetArray<AnimationResource>();
        for (IndexT i = 0; i < resources.Size(); i++)
        {
            if (resources[i].loaderInstanceId != id.loaderInstanceId || resources[i].loaderIndex != id.loaderIndex)
                continue;

            const CoreAnimation::AnimationId animation = CoreAnimation::AnimationResourceGetAnimation(id, characterContextAllocator.Get<AnimationIndex>(i));
            characterContextAllocator.Set<Animation>(i, animation);
            CoreAnimation::AnimSampleBuffer& sampleBuffer = characterContextAllocator.Get<SampleBuffer>(i);
            if (sampleBuffer.IsValid())
                sampleBuffer.Discard();
            sampleBuffer.Setup(animation);

            AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(i);
            for (IndexT track = 0; track < MaxNumTracks; track++)
            {
                tracks.pendingAnimations[track].Clear();
                tracks.playingAnimations[track].clip = -1;
                tracks.fadingAnimations[track].clip = -1;
            }
        }
    });

    // a reloaded model may have other joint masks, the clips playing with one of the model get the reloaded mask of
    // the same name, or none if there's none of the name anymore
    Models::ModelLoader* modelLoader = Resources::ResourceServer::Instance()->GetStreamLoader<Models::ModelLoader>();
    modelLoader->AddReloadListener([](Resources::ResourceId id)
    {
        const Models::ModelId reloaded = id;
        const Util::Array<Graphics::GraphicsEntityId>& entities = characterContextAllocator.GetArray<EntityId>();
        for (IndexT i = 0; i < entities.Size(); i++)
        {
            if (entities[i] == Graphics::InvalidGraphicsEntityId || Models::ModelContext::GetModel(entities[i]) != reloaded)
                continue;

            Util::FixedArray<CoreAnimation::AnimSampleMask>& masks = characterContextAllocator.Get<JointMasks>(i);
            Util::FixedArray<CoreAnimation::AnimSampleMask> newMasks;
            FetchJointMasks(entities[i], newMasks);

            // the masks are found by name before the old ones are replaced, then pointed to once they're in place
            AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(i);
            Util::Array<AnimationRuntime*> runtimes;
            Util::Array<IndexT> newIndices;
            auto remap = [&](AnimationRuntime& runtime)
            {
                if (runtime.mask == nullptr || masks.IsEmpty() || runtime.mask < masks.Begin() || runtime.mask >= masks.End())
                    return;
                IndexT found = InvalidIndex;
                for (IndexT m = 0; m < newMasks.Size() && found == InvalidIndex; m++)
                {
                    if (newMasks[m].name == runtime.mask->name)
                        found = m;
                }
                runtimes.Append(&runtime);
                newIndices.Append(found);
            };
            for (IndexT track = 0; track < MaxNumTracks; track++)
            {
                remap(tracks.playingAnimations[track]);
                remap(tracks.fadingAnimations[track]);
                for (IndexT p = 0; p < tracks.pendingAnimations[track].Size(); p++)
                    remap(tracks.pendingAnimations[track][p]);
            }
            masks = newMasks;
            for (IndexT r = 0; r < runtimes.Size(); r++)
                runtimes[r]->mask = newIndices[r] != InvalidIndex ? &masks[newIndices[r]] : nullptr;
        }
    });
#endif
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
    characterContextAllocator.Set<JointMasks>(cid.id, Util::FixedArray<CoreAnimation::AnimSampleMask>());

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
        if (renderables.nodeTypes[i] == Models::CharacterSkinNodeType)
        {
            // Save offset to the character node in the model
            characterContextAllocator.Set<CharacterSkinNodeIndexOffset>(cid.id, i - nodeRange.begin);
            break;
        }
    }

    characterContextAllocator.Get<Skeleton>(cid.id) = Characters::InvalidSkeletonId;
    Resources::CreateResource(skeletonResource, tag, [cid, skeletonIndex](Resources::ResourceId rid)
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

    }, nullptr, { .immediate = true });

    characterContextAllocator.Get<Animation>(cid.id) = CoreAnimation::InvalidAnimationId;
    characterContextAllocator.Set<AnimationResource>(cid.id, Resources::InvalidResourceId);
    Resources::CreateResource(animationResource, tag, [cid, animationIndex, supportBlending](Resources::ResourceId rid)
        {
            auto animation = CoreAnimation::AnimationResourceGetAnimation(rid, animationIndex);
            characterContextAllocator.Set<Animation>(cid.id, animation);
            characterContextAllocator.Set<AnimationResource>(cid.id, rid);
            characterContextAllocator.Set<AnimationIndex>(cid.id, animationIndex);
            characterContextAllocator.Get<Loaded>(cid.id) |= AnimationLoaded;

            // setup sample buffer when animation is done loading
            characterContextAllocator.Get<SampleBuffer>(cid.id).Setup(animation);
            characterContextAllocator.Set<SupportMix>(cid.id, supportBlending);
        }, nullptr, { .immediate = true });

    // clear playing animation state
    IndexT i;
    for (i = 0; i < MaxNumTracks; i++)
    {
        memset(&characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[i], 0, sizeof(AnimationRuntime));
        characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[i].clip = -1;
        memset(&characterContextAllocator.Get<TrackController>(cid.id).fadingAnimations[i], 0, sizeof(AnimationRuntime));
        characterContextAllocator.Get<TrackController>(cid.id).fadingAnimations[i].clip = -1;
    }
}

//------------------------------------------------------------------------------
/**
    A clip takes the track over. The clip playing plays on under it while it fades in, a clip appended after the
    playing one fades in for as long as the playing one fades out if that's longer. Without a fade the clip playing
    is dropped, and so is one still fading from before
*/
void
TakeOverTrack(CharacterContext::AnimationRuntime& playing, CharacterContext::AnimationRuntime& fading, CharacterContext::AnimationRuntime& next, const bool appended)
{
    Timing::Tick fade = next.fadeInTime;
    if (appended)
        fade = Math::max(fade, playing.fadeOutTime);
    if (fade <= 0)
    {
        fading.clip = -1;
    }
    else if (playing.clip != -1 && playing.started)
    {
        // a clip that took over without having started yet leaves the one fading under it
        fading = std::move(playing);
        next.fadeInTime = fade;
    }
    playing = std::move(next);
}

//------------------------------------------------------------------------------
/**
    Move a clip on to the time of the update. It starts on the first update it's in, a paused clip holds its time
*/
void
AdvanceRuntime(CharacterContext::AnimationRuntime& runtime, const Timing::Tick ticks)
{
    if (!runtime.started)
    {
        // 0 unless the clip was moved to a time before it started
        runtime.baseTime = ticks - runtime.startTime - runtime.prevEvalTime;
        runtime.started = true;
    }

    // the time since the clip started
    runtime.evalTime = ticks - (runtime.baseTime + runtime.startTime);
    if (runtime.paused)
    {
        // hold the pose, the clip starts later by the time it's paused, which delays its expiry as well
        runtime.baseTime += runtime.evalTime - runtime.prevEvalTime;
        runtime.evalTime = runtime.prevEvalTime;
    }
    runtime.prevEvalTime = runtime.evalTime;
    runtime.prevSampleTime = runtime.sampleTime;
    runtime.sampleTime = Timing::Tick(Math::max(runtime.evalTime, 0) * runtime.timeFactor) + runtime.timeOffset;
}

//------------------------------------------------------------------------------
/**
    How far a clip is faded in, and out as it ends if it isn't infinite, from 0 to 1. The fades go by the time since
    the clip started, not its own time, so they take as long whatever the clip's speed
*/
float
FadeWeight(const CharacterContext::AnimationRuntime& runtime)
{
    float weight = 1.0f;
    if (runtime.fadeInTime > 0 && runtime.evalTime < runtime.fadeInTime)
        weight *= Math::max(runtime.evalTime, 0) / (float)runtime.fadeInTime;
    if (runtime.fadeOutTime > 0 && runtime.duration > 0)
    {
        const Timing::Tick remaining = runtime.duration - runtime.evalTime;
        if (remaining < runtime.fadeOutTime)
            weight *= Math::max(remaining, 0) / (float)runtime.fadeOutTime;
    }
    return weight;
}

//------------------------------------------------------------------------------
/**
    The packager writes a model's skeleton and animation under the model's name, so a skinned model that's
    placed without naming them (like a model asset dropped in the editor) still gets skinned. Without a character
    the skinned shaders read a joint palette that's never uploaded
*/
bool
CharacterContext::SetupFromModel(const Graphics::GraphicsEntityId id, const IO::Path& model, const Util::StringAtom& tag)
{
    const Models::NodeInstanceRange& nodeRange = Models::ModelContext::GetModelRenderableRange(id);
    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
    bool skinned = false;
    for (SizeT i = nodeRange.begin; i < nodeRange.end && !skinned; i++)
        skinned = renderables.nodeTypes[i] == Models::CharacterSkinNodeType;
    if (!skinned)
        return false;

    const IO::URI skeleton = IO::Path::File(model.GetFolder(), model.GetFile(), "ske").ExportURI();
    const IO::URI animation = IO::Path::File(model.GetFolder(), model.GetFile(), "ani").ExportURI();
    if (!IO::FileExists(skeleton) || !IO::FileExists(animation))
    {
        n_warning("Skinned model '%s' has no skeleton or animation, it isn't animated\n", model.AsString().AsCharPtr());
        return false;
    }

    CharacterContext::RegisterEntity(id);
    CharacterContext::Setup(id, skeleton.AsString(), 0, animation.AsString(), 0, tag);
    CharacterContext::PlayClip(id, nullptr, 0, 0, EnqueueMode::Replace);
    return true;
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
    runtime.fadeInTime = Timing::SecondsToTicks(Math::max(fadeIn, 0.0f));
    runtime.fadeOutTime = Timing::SecondsToTicks(Math::max(fadeOut, 0.0f));
    runtime.timeFactor = timeFactor;
    runtime.timeOffset = timeOffset;
    runtime.startTime = startTime;
    runtime.curveSampleIndices.Resize(clip.numCurves);
    runtime.curveSampleIndices.Fill(0, clip.numCurves, 0x0);
    runtime.mask = mask;

    runtime.baseTime = 0;
    runtime.duration = 0;
    runtime.evalTime = runtime.prevEvalTime = 0;
    // before the start, so the events at the start of the clip fire on its first update
    runtime.sampleTime = runtime.prevSampleTime = -1;
    runtime.paused = false;
    runtime.started = false;
    runtime.duration = Timing::Tick(clip.duration * loopCount * (1 / timeFactor));

    // a replacing clip takes the track over right away, the others are queued
    if (mode == Replace)
    {
        AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);
        TakeOverTrack(tracks.playingAnimations[track], tracks.fadingAnimations[track], runtime, false);
    }
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
CharacterContext::StopTrack(const Graphics::GraphicsEntityId id, const IndexT track, const float fadeOut)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);

    tracks.pendingAnimations[track].Clear();
    tracks.fadingAnimations[track].clip = -1;
    AnimationRuntime& playing = tracks.playingAnimations[track];
    const Timing::Tick fadeTime = Timing::SecondsToTicks(Math::max(fadeOut, 0.0f));
    if (fadeTime > 0 && playing.clip != -1 && playing.started)
    {
        // the clip ends once it's faded out from where it is, unless it ends before
        const Timing::Tick end = playing.prevEvalTime + fadeTime;
        if (playing.duration <= 0 || end < playing.duration)
        {
            playing.duration = end;
            playing.fadeOutTime = fadeTime;
        }
    }
    else
    {
        playing.clip = -1;
    }
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
        tracks.fadingAnimations[i].clip = -1;
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
    tracks.fadingAnimations[track].paused = tracks.playingAnimations[track].paused;
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

    // the playing clip goes on from the time it's at, the time since it started and its duration are scaled to the new rate
    AnimationRuntime& playing = tracks.playingAnimations[track];
    if (playing.clip != -1 && playing.timeFactor > 0.0f && factor > 0.0f && playing.timeFactor != factor)
    {
        const float scale = playing.timeFactor / factor;
        const Timing::Tick evalTime = Timing::Tick(playing.prevEvalTime * scale);
        if (playing.started)
            playing.baseTime += playing.prevEvalTime - evalTime;
        playing.prevEvalTime = evalTime;
        playing.evalTime = evalTime;
        playing.duration = Timing::Tick(playing.duration * scale);
    }
    playing.timeFactor = factor;
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
const SkeletonId
CharacterContext::GetSkeleton(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<Skeleton>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreAnimation::AnimationId
CharacterContext::GetAnimation(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<Animation>(cid.id);
}

//------------------------------------------------------------------------------
/**
    The scaled palette holds the joints in model space, the skinning palette has the inverse bind pose applied
*/
const Util::FixedArray<Math::mat4>&
CharacterContext::GetJointTransforms(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<JointPaletteScaled>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
CharacterContext::GetPlayingClip(const Graphics::GraphicsEntityId id, const IndexT track)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track].clip;
}

//------------------------------------------------------------------------------
/**
*/
bool
CharacterContext::IsTrackPaused(const Graphics::GraphicsEntityId id, const IndexT track)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track].paused;
}

//------------------------------------------------------------------------------
/**
    The time is taken from the time since the clip started as of the last update, or the last seek, so a seek shows
    right away
*/
Timing::Time
CharacterContext::GetTrackTime(const Graphics::GraphicsEntityId id, const IndexT track)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    const AnimationRuntime& playing = characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track];
    const CoreAnimation::AnimationId anim = characterContextAllocator.Get<Animation>(cid.id);
    if (playing.clip == -1 || anim == CoreAnimation::InvalidAnimationId)
        return 0.0;

    const Timing::Tick duration = CoreAnimation::AnimGetClip(anim, playing.clip).duration;
    if (duration <= 0)
        return 0.0;
    Timing::Tick time = Timing::Tick(Math::max(playing.prevEvalTime, 0) * playing.timeFactor) + playing.timeOffset;
    time %= duration;
    if (time < 0)
        time += duration;
    return Timing::TicksToSeconds(time);
}

//------------------------------------------------------------------------------
/**
    The weight the clip was mixed with in the last update, without the track's weight. A clip that hasn't started
    yet is at the start of its fade in
*/
float
CharacterContext::GetTrackFadeWeight(const Graphics::GraphicsEntityId id, const IndexT track)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    const AnimationRuntime& playing = characterContextAllocator.Get<TrackController>(cid.id).playingAnimations[track];
    if (playing.clip == -1)
    {
        return 0.0f;
    }
    return FadeWeight(playing);
}

//------------------------------------------------------------------------------
/**
    The clip's start is moved, the next update gets the time since the clip started from it. That keeps the time a
    clip that isn't looped stops at in step with the time it's moved to
*/
void
CharacterContext::SeekTrack(const Graphics::GraphicsEntityId id, const IndexT track, const Timing::Time time)
{
    n_assert(track < MaxNumTracks);
    const ContextEntityId cid = GetContextId(id);
    AnimationTracks& tracks = characterContextAllocator.Get<TrackController>(cid.id);
    AnimationRuntime& playing = tracks.playingAnimations[track];
    if (playing.clip == -1)
        return;

    // moving the clip ends a fade from the clip before it
    tracks.fadingAnimations[track].clip = -1;

    const Timing::Tick clipTime = Timing::SecondsToTicks(time) - playing.timeOffset;
    const Timing::Tick evalTime = playing.timeFactor > 0.0f ? Timing::Tick(clipTime / playing.timeFactor) : clipTime;

    // a clip that hasn't started yet gets its start from this when it does
    if (playing.started)
        playing.baseTime += playing.prevEvalTime - evalTime;
    playing.prevEvalTime = evalTime;
    playing.evalTime = evalTime;

    // the next update fires the events from here on
    playing.sampleTime = Timing::Tick(Math::max(evalTime, 0) * playing.timeFactor) + playing.timeOffset;
}

//------------------------------------------------------------------------------
/**
    The masks are copied from the model the first time they're asked for once it's loaded
*/
const Util::FixedArray<CoreAnimation::AnimSampleMask>&
CharacterContext::GetJointMasks(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    Util::FixedArray<CoreAnimation::AnimSampleMask>& masks = characterContextAllocator.Get<JointMasks>(cid.id);
    if (masks.IsEmpty())
        FetchJointMasks(id, masks);
    return masks;
}

//------------------------------------------------------------------------------
/**
*/
const CoreAnimation::AnimSampleMask*
CharacterContext::GetJointMask(const Graphics::GraphicsEntityId id, const Util::StringAtom& name)
{
    const Util::FixedArray<CoreAnimation::AnimSampleMask>& masks = GetJointMasks(id);
    for (IndexT i = 0; i < masks.Size(); i++)
    {
        if (masks[i].name == name)
            return &masks[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CharacterContext::FiredEvent>&
CharacterContext::GetFiredEvents(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    return characterContextAllocator.Get<FiredEvents>(cid.id);
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
    Add the events of a clip that are passed going from one sample time to a later one, the start isn't included.
    The times go on past the clip's duration when it loops, an event is added once however often it's passed
*/
static void
CollectFiredEvents(const CoreAnimation::AnimationId anim, const IndexT clipIndex, const IndexT track, const Timing::Tick from, const Timing::Tick to, Util::Array<CharacterContext::FiredEvent>& outEvents)
{
    const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, clipIndex);
    if (clip.numEvents == 0 || clip.duration <= 0 || to <= from)
        return;
    const Util::FixedArray<CoreAnimation::AnimEvent>& events = CoreAnimation::AnimGetEvents(anim);
    for (IndexT i = 0; i < clip.numEvents; i++)
    {
        const CoreAnimation::AnimEvent& event = events[clip.firstEvent + i];

        // the first time the event is at after the start
        const Timing::Tick loop = (Timing::Tick)Math::floor(double(from - event.time) / double(clip.duration)) + 1;
        const Timing::Tick time = loop * clip.duration + event.time;
        if (time <= to)
            outEvents.Append({ event, clipIndex, track });
    }
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
    uchar** tmpSampleCounts;
    Math::mat4** tmpJoints;
    
    const Util::Array<Graphics::GraphicsEntityId>* entities;
    Util::Array<Util::Array<CharacterContext::FiredEvent>>* firedEvents;
    CoreAnimation::AnimSampleMixInfo* animMixInfos;
    float frameTime;
    Timing::Tick time;
    Timing::Tick ticks;
};

//------------------------------------------------------------------------------
/**
    Set the samples to the idle pose, a translation, rotation and scale per joint, for clips to be mixed over
*/
static void
IdleSamples(const Util::FixedArray<Math::vec4>& idleSamples, const CoreAnimation::AnimSampleBuffer& sampleBuffer)
{
    float* samples = sampleBuffer.GetSamplesPointer();
    uchar* counts = sampleBuffer.GetSampleCountsPointer();
    const SizeT numJoints = Math::min(idleSamples.Size() / 3, sampleBuffer.GetNumSamples() / 10);
    for (IndexT joint = 0; joint < numJoints; joint++)
    {
        const Math::vec4& translation = idleSamples[joint * 3];
        const Math::vec4& rotation = idleSamples[joint * 3 + 1];
        const Math::vec4& scale = idleSamples[joint * 3 + 2];
        samples[0] = translation.x; samples[1] = translation.y; samples[2] = translation.z;
        samples[3] = rotation.x; samples[4] = rotation.y; samples[5] = rotation.z; samples[6] = rotation.w;
        samples[7] = scale.x; samples[8] = scale.y; samples[9] = scale.z;
        samples += 10;
        counts[0] = counts[1] = counts[2] = 1;
        counts += 3;
    }
}

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
        Util::Array<CharacterContext::FiredEvent>& firedEvents = (*context->firedEvents)[index];
        const AnimationId anim = context->anims->Get(index);
        if (anim == InvalidAnimationId)
            continue;
        const Util::FixedArray<SkeletonJobJoint>& jobJoint = context->jobJoints->Get(index);
        const SkeletonId skeleton = context->skeletons->Get(index);
        if (skeleton == InvalidSkeletonId)
            continue;
        const Util::FixedArray<Math::mat4>& bindPose = Characters::SkeletonGetBindPose(skeleton);
        const Util::FixedArray<Characters::CharacterJoint>& joints = Characters::SkeletonGetJoints(skeleton);
        const Util::FixedArray<Math::mat4>& jointPalette = context->jointPalettes->Get(index);
        const Util::FixedArray<Math::mat4>& scaledJointPalette = context->scaledJointPalettes->Get(index);
        const Util::FixedArray<Math::vec4>& idleSamples = Characters::SkeletonGetIdleSamples(skeleton);
        const CoreAnimation::AnimSampleBuffer& sampleBuffer = context->sampleBuffers->Get(index);
        Math::mat4* tmpMatrices = context->tmpJoints[index];
        float* tmpSamples = context->tmpSamples[index];
        uchar* tmpSampleCounts = context->tmpSampleCounts[index];
        auto sampleMixInfo = context->animMixInfos + index;
        bool runSkeletonThisFrame = false;

        // sample a clip at a time of its own, the first clip with its full weight and no mask is sampled right into the
        // samples, the others are mixed over the clips sampled before them, or over the idle pose if there are none.
        // Without scratch memory for mixing the clips replace each other
        bool firstAnimTrack = true;
        const Ptr<AnimKeyBuffer>& buffer = AnimGetBuffer(anim);
        const float* srcPtr = buffer->GetKeyBufferPointer();
        const AnimKeyBuffer::Interval* srcTimePtr = buffer->GetIntervalBufferPointer();
        const Util::FixedArray<AnimCurve>& curves = CoreAnimation::AnimGetCurves(anim);
        auto sampleClip = [&](CharacterContext::AnimationRuntime& runtime, const float weight, const Timing::Tick sampleTime)
        {
            // nothing to mix
            if (weight <= 0.0f && !firstAnimTrack)
                return;

            const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(anim, runtime.clip);
            Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
            sampleMixInfo->sampleType = SampleType::Linear;
            sampleMixInfo->velocityScale.set(runtime.timeFactor, runtime.timeFactor, runtime.timeFactor, 0);
            sampleMixInfo->mixWeight = weight;

            Timing::Tick evalTime = clip.duration > 0 ? sampleTime % clip.duration : 0;
            if (evalTime < 0)
                evalTime += clip.duration;

            const bool mix = tmpSamples != nullptr && (!firstAnimTrack || weight != 1.0f || runtime.mask != nullptr);
            if (!mix)
            {
                if (sampleMixInfo->sampleType == SampleType::Step)
                    AnimSampleStep(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, runtime.curveSampleIndices.Begin(), sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
                else
                    AnimSampleLinear(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, runtime.curveSampleIndices.Begin(), sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
            }
            else
            {
                if (firstAnimTrack)
                    IdleSamples(idleSamples, sampleBuffer);

                if (sampleMixInfo->sampleType == SampleType::Step)
                    AnimSampleStep(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, runtime.curveSampleIndices.Begin(), tmpSamples, tmpSampleCounts);
                else
                    AnimSampleLinear(clip, curves, evalTime, sampleMixInfo->velocityScale, idleSamples, srcPtr, srcTimePtr, runtime.curveSampleIndices.Begin(), tmpSamples, tmpSampleCounts);

                AnimMix(clip, curves, runtime.mask, sampleMixInfo->mixWeight, sampleBuffer.GetSamplesPointer(), tmpSamples, sampleBuffer.GetSampleCountsPointer(), tmpSampleCounts, sampleBuffer.GetSamplesPointer(), sampleBuffer.GetSampleCountsPointer());
            }

            // run the skeleton once a clip is sampled, the next ones are mixed over it
            runSkeletonThisFrame = true;
            firstAnimTrack = false;
        };

        // loop over all tracks, and update the playing clip on each respective track
        IndexT j;
        for (j = 0; j < CharacterContext::MaxNumTracks; j++)
        {
            CharacterContext::AnimationRuntime& playing = trackController.playingAnimations[j];
            CharacterContext::AnimationRuntime& fading = trackController.fadingAnimations[j];
            Util::StackArray<CharacterContext::AnimationRuntime, 8>& pending = trackController.pendingAnimations[j];

            // a clip that ended fires the events up to its end, the runtimes keep their times in ticks
            if (playing.clip != -1 && playing.started && IsExpired(playing, context->ticks))
            {
                const Timing::Tick end = Timing::Tick(playing.duration * playing.timeFactor) + playing.timeOffset;
                CollectFiredEvents(anim, playing.clip, j, playing.sampleTime, end, firedEvents);
                playing.clip = -1;
            }
            if (playing.clip == -1)
                fading.clip = -1;

            // the clip queued first takes the track over, right away if none is playing or the playing one is infinite,
            // otherwise once the playing one starts fading out, or ends if it doesn't. Clips already playing are skipped
            while (!pending.IsEmpty())
            {
                CharacterContext::AnimationRuntime& next = pending[0];
                if (playing.clip != -1 && next.enqueueMode == Characters::IgnoreIfSame && next.clip == playing.clip)
                {
                    pending.EraseIndex(0);
                    continue;
                }
                bool appended = false;
                if (playing.clip != -1)
                {
                    if (IsInfinite(playing))
                    {
                        if (next.enqueueMode == Characters::Append)
                        {
#if NEBULA_DEBUG
                            n_error("CharacterContext: Can't insert anim job '%s' because track is blocked by an infinite clip\n", playing.name.AsCharPtr());
#endif
                            break;
                        }
                    }
                    else if (!playing.started || context->ticks < GetAbsoluteStopTime(playing))
                    {
                        break;
                    }
                    else
                    {
                        appended = true;
                    }
                }
                TakeOverTrack(playing, fading, next, appended);
                pending.EraseIndex(0);
                break;
            }

            if (playing.clip == -1)
                continue;

            // the events passed since the last update, none while it's paused
            AdvanceRuntime(playing, context->ticks);
            CollectFiredEvents(anim, playing.clip, j, playing.prevSampleTime, playing.sampleTime, firedEvents);
            const float fadeWeight = FadeWeight(playing);

            // the clip the playing one took over from plays on under it until it's faded in. It doesn't fade out
            // itself, the one over it does that, and it holds its last pose if it ends before
            if (fading.clip != -1 && (playing.fadeInTime <= 0 || playing.evalTime >= playing.fadeInTime))
                fading.clip = -1;
            if (fading.clip != -1)
            {
                AdvanceRuntime(fading, context->ticks);
                Timing::Tick sampleTime = fading.sampleTime;
                if (fading.duration > 0)
                {
                    const Timing::Tick end = Timing::Tick(fading.duration * fading.timeFactor) + fading.timeOffset;
                    fading.sampleTime = Math::min(fading.sampleTime, end);
                    sampleTime = Math::max(Math::min(sampleTime, end - 1), fading.timeOffset);
                }
                CollectFiredEvents(anim, fading.clip, j, fading.prevSampleTime, fading.sampleTime, firedEvents);
                sampleClip(fading, fading.blend, sampleTime);
            }

            sampleClip(playing, playing.blend * fadeWeight, playing.sampleTime);
        }

        // If we have no animations, just skip the character skeleton update
        if (!runSkeletonThisFrame)
            continue;

        // Evaluate skeleton
        const Math::mat4* invPoseMatrixBase = bindPose.Begin();
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

            translate.loadu(samplesPtr + TRANSLATION_OFFSET);
            rotate.loadu(samplesPtr + ROTATION_OFFSET);
            scale.loadu(samplesPtr + SCALE_OFFSET);

            samplesPtr += sampleWidth;

            const SkeletonJobJoint& comps = compsBase[jointIndex];

            Math::mat4& unscaledMatrix = unscaledMatrixBase[jointIndex];
            Math::mat4& scaledMatrix = scaledMatrixBase[jointIndex];

            Math::vec3 baseScale = xyz(idleSamples[jointIndex * 3 + 2]);

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
        static Threading::Interlocked::AtomicCounter animationCounter = 0;

        // Set total counter
        n_assert(animationCounter.counter == 0);
        animationCounter = 1;

        // the events the last update's jobs collected are the ones reported until the next update finishes,
        // so they aren't read while the jobs add to them
        Util::Array<Util::Array<FiredEvent>>& firedEvents = characterContextAllocator.GetArray<FiredEvents>();
        Util::Array<Util::Array<FiredEvent>>& pendingFiredEvents = characterContextAllocator.GetArray<PendingFiredEvents>();
        for (IndexT i = 0; i < firedEvents.Size(); i++)
        {
            firedEvents[i] = pendingFiredEvents[i];
            pendingFiredEvents[i].Clear();
        }

        CharacterJobContext charCtx;
        charCtx.firedEvents = &pendingFiredEvents;
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
        charCtx.tmpSampleCounts = Jobs2::JobAlloc<uchar*>(models.Size());
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
            // the samples have a float of padding like the sample buffer's, 3 component samples are stored as 4 floats.
            // There's a count per curve, 3 per 10 samples, padded like the sample buffer's as well
            const CoreAnimation::AnimSampleBuffer& sampleBuffer = sampleBuffers[i];
            charCtx.tmpSamples[i] = nullptr;
            charCtx.tmpSampleCounts[i] = nullptr;
            if (supportsBlending[i] && sampleBuffer.IsValid())
            {
                charCtx.tmpSamples[i] = Jobs2::JobAlloc<float>(sampleBuffer.GetNumSamples() + 1);
                charCtx.tmpSampleCounts[i] = Jobs2::JobAlloc<uchar>(sampleBuffer.GetNumSamples() / 10 * 3 + 16);
            }
        }

        // Run job
        Jobs2::JobDispatch(EvalCharacter, models.Size(), 64, charCtx, nullptr, &animationCounter, nullptr);

        n_assert(ConstantUpdateCounter.counter == 0);
        ConstantUpdateCounter = 1;

        // Run job to update constants
        Jobs2::JobDispatch(
            [
                entities = models.ConstBegin()
                , jointPalettes = jointPalettes.ConstBegin()
            ]
        (SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
        {
            N_SCOPE(JointConstantsUpdate, Graphics);
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = invocationOffset + i;
                if (index >= totalJobs)
                    return;

                const Graphics::GraphicsEntityId entity = entities[index];
                if (entity == Graphics::InvalidGraphicsEntityId)
                    continue;

                const Models::NodeInstanceRange& range = Models::ModelContext::GetModelRenderableRange(entity);
                const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();

                const Util::FixedArray<Math::mat4>& jointPalette = jointPalettes[index];

                // every skin fragment is a node of its own, each gets the joints of its palette packed
                for (IndexT node = range.begin; node < range.end; node++)
                {
                    if (renderables.nodeTypes[node] != Models::NodeType::CharacterSkinNodeType)
                        continue;
                    Models::CharacterSkinNode* sparent = reinterpret_cast<Models::CharacterSkinNode*>(renderables.nodes[node]);
                    if (sparent->skinFragments.IsEmpty())
                        continue;
                    const Util::Array<IndexT>& usedIndices = sparent->skinFragments[0].jointPalette;
                    Util::FixedArray<Math::mat4, true> usedMatrices(usedIndices.Size());

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
                    uint64_t offset = CoreGraphics::SetConstants(usedMatrices.Begin(), usedMatrices.Size(), CoreGraphics::QueueType::GraphicsQueueType);
                    renderables.nodeStates[node].resourceTableOffsets[renderables.nodeStates[node].skinningConstantsIndex] = offset;
                }
            }

        }, characterSkinNodeIndices.Size(), 64, { &animationCounter }, &ConstantUpdateCounter, &CharacterContext::totalCompletionEvent);
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
CharacterContext::OnRenderDebug(uint32_t flags)
{
    const Util::Array<Util::FixedArray<Math::mat4>>& jointPalettes = characterContextAllocator.GetArray<JointPaletteScaled>();
    const Util::Array<Graphics::GraphicsEntityId>& modelContexts = characterContextAllocator.GetArray<EntityId>();
    const Math::mat4 scale = Math::scaling(0.1f, 0.1f, 0.1f);
    IndexT i;
    for (i = 0; i < jointPalettes.Size(); i++)
    {
        if (modelContexts[i] == Graphics::InvalidGraphicsEntityId)
            continue;

        const Util::FixedArray<Math::mat4>& jointsPalette = jointPalettes[i];
        const Math::mat4& transform = Models::ModelContext::GetTransform(modelContexts[i]);
        IndexT j;
        for (j = 0; j < jointsPalette.Size(); j++)
        {
            Math::mat4 joint = transform * jointsPalette[j];
            Math::mat4 mat;
            mat.row0() = normalize3(joint.row0());
            mat.row1() = normalize3(joint.row1());
            mat.row2() = normalize3(joint.row2());
            mat.row3() = normalize3(joint.row3());
            mat.row3().w = 1.0f;
            mat.position() = joint.position();
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

