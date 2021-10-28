//------------------------------------------------------------------------------
//  particlecontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particlecontext.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "models/nodes/particlesystemnode.h"
#include "particles/emitterattrs.h"
#include "particles/emittermesh.h"
#include "particles/envelopesamplebuffer.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

#include <particle.h>

using namespace Graphics;
using namespace Models;
namespace Particles
{

ParticleContext::ParticleContextAllocator ParticleContext::particleContextAllocator;
__ImplementContext(ParticleContext, ParticleContext::particleContextAllocator);

extern void ParticleStepJob(const Jobs::JobFuncContext& ctx);

CoreGraphics::MeshId ParticleContext::DefaultEmitterMesh;
const Timing::Time DefaultStepTime = 1.0f / 60.0f;
Timing::Time StepTime = 1.0f / 60.0f;

const SizeT ParticleContextNumEnvelopeSamples = 192;

struct
{
    CoreGraphics::BufferId geometryVbo;
    CoreGraphics::BufferId geometryIbo;
    Util::FixedArray<CoreGraphics::BufferId> vbos;
    Util::FixedArray<SizeT> vboSizes;
    Util::FixedArray<byte*> mappedVertices;
    byte* vertexPtr;

    SizeT vertexSize;

    Util::Array<CoreGraphics::VertexComponent> particleComponents;
    CoreGraphics::VertexLayoutId layout;
    CoreGraphics::PrimitiveGroup primGroup;
} state;

Jobs::JobSyncId ParticleContext::jobSync;
Jobs::JobSyncId ParticleContext::particleSync;
Util::Queue<Jobs::JobId> ParticleContext::runningJobs;
//------------------------------------------------------------------------------
/**
*/
ParticleContext::ParticleContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ParticleContext::~ParticleContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Create()
{
    __bundle.OnBegin = ParticleContext::UpdateParticles;
    __bundle.OnPrepareView = ParticleContext::OnPrepareView;
    __bundle.OnBeforeFrame = ParticleContext::WaitForParticleUpdates; // wait for jobs before we issue visibility
    __bundle.StageBits = &ParticleContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ParticleContext::OnRenderDebug;
#endif

    ParticleContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    Jobs::CreateJobSyncInfo sinfo =
    {
        nullptr
    };
    ParticleContext::jobSync = Jobs::CreateJobSync(sinfo);
    ParticleContext::particleSync = Jobs::CreateJobSync(sinfo);

    struct VectorB4N
    {
        char x : 8;
        char y : 8;
        char z : 8;
        char w : 8;
    };

    VectorB4N normal;
    normal.x = 0;
    normal.y = 127;
    normal.z = 0;
    normal.w = 0;

    VectorB4N tangent;
    tangent.x = 0;
    tangent.y = 0;
    tangent.z = 127;
    tangent.w = 0;

    float vertex[] = { 0, 0, 0, 0, 0 };
    memcpy(&vertex[3], &normal, 4);
    memcpy(&vertex[4], &tangent, 4);

    Util::Array<CoreGraphics::VertexComponent> emitterComponents;
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Position, 0, CoreGraphics::VertexComponent::Float3, 0));
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Normal, 0, CoreGraphics::VertexComponent::Byte4N, 0));
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Tangent, 0, CoreGraphics::VertexComponent::Byte4N, 0));
    CoreGraphics::VertexLayoutId emitterLayout = CoreGraphics::CreateVertexLayout({ emitterComponents });

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "Single Point Particle Emitter VBO";
    vboInfo.size = 1;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(emitterLayout);
    vboInfo.mode = CoreGraphics::DeviceToHost;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = vertex;
    vboInfo.dataSize = sizeof(vertex);
    CoreGraphics::BufferId vbo = CoreGraphics::CreateBuffer(vboInfo);

    uint indices[] = { 0 };
    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.name = "Single Point Particle Emitter IBO";
    iboInfo.size = 1;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
    iboInfo.mode = CoreGraphics::DeviceToHost;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = indices;
    iboInfo.dataSize = sizeof(indices);
    CoreGraphics::BufferId ibo = CoreGraphics::CreateBuffer(iboInfo);

    CoreGraphics::PrimitiveGroup group;
    group.SetBaseIndex(0);
    group.SetBaseVertex(0);
    group.SetNumIndices(1);
    group.SetNumVertices(1);
    group.SetVertexLayout(emitterLayout);

    // setup single point emitter mesh
    CoreGraphics::MeshCreateInfo meshInfo;
    CoreGraphics::MeshCreateInfo::Stream stream;
    stream.vertexBuffer = vbo;
    stream.index = 0;
    meshInfo.streams.Append(stream);
    meshInfo.indexBuffer = ibo;
    meshInfo.name = "Single Point Particle Emitter Mesh";
    meshInfo.primitiveGroups.Append(group);
    meshInfo.topology = CoreGraphics::PrimitiveTopology::PointList;
    meshInfo.tag = "system";
    meshInfo.vertexLayout = emitterLayout;
    ParticleContext::DefaultEmitterMesh = CoreGraphics::CreateMesh(meshInfo);

    // setup particle geometry buffer
    Util::Array<CoreGraphics::VertexComponent> cornerComponents;
    cornerComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)0, 0, CoreGraphics::VertexComponent::Float2, 0));
    float cornerVertexData[] = { 0, 0,  1, 0,  1, 1,  0, 1 };
    CoreGraphics::VertexLayoutId cornerLayout = CoreGraphics::CreateVertexLayout({ cornerComponents });

    vboInfo.name = "Particle Geometry Vertex Buffer";
    vboInfo.size = 4;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(cornerLayout);
    vboInfo.mode = CoreGraphics::DeviceLocal;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.dataSize = sizeof(cornerVertexData);
    vboInfo.data = cornerVertexData;
    state.geometryVbo = CoreGraphics::CreateBuffer(vboInfo);

    // setup the corner index buffer
    ushort cornerIndexData[] = { 0, 1, 2, 2, 3, 0 };
    iboInfo.name = "Particle Geometry Index Buffer";
    iboInfo.size = 6;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index16);
    iboInfo.mode = CoreGraphics::DeviceLocal;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = cornerIndexData;
    iboInfo.dataSize = sizeof(cornerIndexData);
    state.geometryIbo = CoreGraphics::CreateBuffer(iboInfo);

    // save vertex components so we can allocate a buffer later
    state.particleComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)1, 0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1));   // Particle::position
    state.particleComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)2, 0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1));   // Particle::stretchPosition
    state.particleComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)3, 0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1));   // Particle::color
    state.particleComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)4, 0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1));   // Particle::uvMinMax
    state.particleComponents.Append(CoreGraphics::VertexComponent((CoreGraphics::VertexComponent::SemanticName)5, 0, CoreGraphics::VertexComponent::Float4, 1, CoreGraphics::VertexComponent::PerInstance, 1));   // x: Particle::rotation, y: Particle::size

    SizeT numFrames = CoreGraphics::GetNumBufferedFrames();
    state.vbos.Resize(numFrames);
    state.vboSizes.Resize(numFrames);
    state.mappedVertices.Resize(numFrames);
    for (IndexT i = 0; i < numFrames; i++)
    {
        state.vbos[i] = CoreGraphics::InvalidBufferId;
        state.vboSizes[i] = 0;
        state.mappedVertices[i] = nullptr;
    }

    Util::Array<CoreGraphics::VertexComponent> layoutComponents;
    layoutComponents.AppendArray(cornerComponents);
    layoutComponents.AppendArray(state.particleComponents);
    CoreGraphics::VertexLayoutCreateInfo vloInfo{ layoutComponents };
    state.layout = CoreGraphics::CreateVertexLayout(vloInfo);
    state.vertexSize = sizeof(Math::vec4) * 5; // 5 vertex attributes using vec4

    state.primGroup.SetBaseIndex(0);
    state.primGroup.SetNumIndices(6);
    state.primGroup.SetBaseVertex(0);
    state.primGroup.SetNumVertices(4);

    __CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Setup(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    n_assert_fmt(cid != InvalidContextEntityId, "Entity %d is not registered in ParticleContext", id.HashCode());

    // get model context
    const ContextEntityId mdlId = Models::ModelContext::GetContextId(id);
    n_assert_fmt(mdlId != InvalidContextEntityId, "Entity %d needs to be setup as a model before particle!", id.HashCode());
    particleContextAllocator.Get<ModelId>(cid.id) = mdlId;
    particleContextAllocator.Set<ModelContextId>(cid.id, id);

    // get node map
    Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);

    // get ranges
    const Models::NodeInstanceRange& range = ModelContext::GetModelRenderableRange(id);
    const Models::ModelContext::ModelInstance::Renderable& renderables = ModelContext::GetModelRenderables();
    
    // setup nodes
    IndexT i;
    for (i = range.begin; i < range.end; i++)
    {
        Models::ModelNode* node = renderables.nodes[i];
        if (node->type == ParticleSystemNodeType)
        {
            Models::ParticleSystemNode* pNode = reinterpret_cast<Models::ParticleSystemNode*>(node);
            const Particles::EmitterAttrs& attrs = pNode->GetEmitterAttrs();
            float maxFreq = attrs.GetEnvelope(EmitterAttrs::EmissionFrequency).GetMaxValue();
            float maxLifeTime = attrs.GetEnvelope(EmitterAttrs::LifeTime).GetMaxValue();

            ParticleSystemRuntime& system = systems.Emplace();
            system.renderableIndex = i - range.begin;
            system.emissionCounter = 0;
            system.particles.SetCapacity(1 + SizeT(maxFreq * maxLifeTime));
            system.outputCapacity = 0;
            system.outputData = nullptr;
            system.uniformData.sampleBuffer = pNode->GetSampleBuffer().GetSampleBuffer();
            system.uniformData.gravity = Math::vector(0.0f, attrs.GetFloat(EmitterAttrs::Gravity), 0.0f);
            system.uniformData.stretchToStart = attrs.GetBool(EmitterAttrs::StretchToStart);
            system.uniformData.stretchTime = attrs.GetBool(EmitterAttrs::StretchToStart);
            system.uniformData.windVector = xyz(attrs.GetVec4(EmitterAttrs::WindDirection));

            // Setup model callback (actually identical for ALL particles...)
            renderables.nodeModelCallbacks[i] = [&system]()
            {
                CoreGraphics::SetVertexLayout(ParticleContext::GetParticleVertexLayout());
                CoreGraphics::SetIndexBuffer(ParticleContext::GetParticleIndexBuffer(), 0);
                CoreGraphics::SetPrimitiveGroup(ParticleContext::ParticleContext::GetParticlePrimitiveGroup());
                CoreGraphics::SetStreamVertexBuffer(0, ParticleContext::GetParticleVertexBuffer(), 0);
                CoreGraphics::SetStreamVertexBuffer(1, state.vbos[CoreGraphics::GetBufferedFrameIndex()], 0);
            };
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::ShowParticle(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    const Models::ModelContext::ModelInstance::Renderable& renderables = ModelContext::GetModelRenderables();
    const Models::NodeInstanceRange& stateRange = ModelContext::GetModelRenderableRange(id);

    // use node map to set active flag
    Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
    for (IndexT i = 0; i < systems.Size(); i++)
    {
        SetBits(renderables.nodeFlags[stateRange.begin + systems[i].renderableIndex], Models::NodeInstance_Active);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::HideParticle(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);
    const Models::ModelContext::ModelInstance::Renderable& renderables = ModelContext::GetModelRenderables();
    const Models::NodeInstanceRange& stateRange = ModelContext::GetModelRenderableRange(id);

    // use node map to set active flag
    Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
    for (IndexT i = 0; i < systems.Size(); i++)
    {
        UnsetBits(renderables.nodeFlags[stateRange.begin + systems[i].renderableIndex], Models::NodeInstance_Active);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Play(const Graphics::GraphicsEntityId id, const PlayMode mode)
{
    const ContextEntityId cid = GetContextId(id);

    // use node map to set active flag
    Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
    ParticleRuntime& runtime = particleContextAllocator.Get<Runtime>(cid.id);

    if ((runtime.playing && mode == RestartIfPlaying) || !runtime.playing)
    {
        runtime.stepTime = 0;
        runtime.prevEmissionTime = 0.0f;
        runtime.emissionStartTimeOffset = 0.0f;
        runtime.firstFrame = true;
        runtime.playing = true;
    }
    
    for (IndexT i = 0; i < systems.Size(); i++)
    {
        if ((runtime.playing && mode == RestartIfPlaying) || !runtime.playing)
        {
            systems[i].particles.Reset();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Stop(const Graphics::GraphicsEntityId id)
{
    const ContextEntityId cid = GetContextId(id);

    ParticleRuntime& runtime = particleContextAllocator.Get<Runtime>(cid.id);
    runtime.stopping = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::UpdateParticles(const Graphics::FrameContext& ctx)
{
    N_SCOPE(UpdateParticles, Particles);

    // Reset job sync
    Jobs::JobSyncHostReset(ParticleContext::particleSync);

    const Util::Array<ParticleRuntime>& runtimes = particleContextAllocator.GetArray<Runtime>();
    const Util::Array<Util::Array<ParticleSystemRuntime>>& allSystems = particleContextAllocator.GetArray<ParticleSystems>();
    const Models::ModelContext::ModelInstance::Renderable& renderables = ModelContext::GetModelRenderables();
    const Models::ModelContext::ModelInstance::Transformable& transformables = ModelContext::GetModelTransformables();
    const Util::Array<Graphics::GraphicsEntityId>& graphicsEntities = particleContextAllocator.GetArray<ModelContextId>();

    IndexT i;
    for (i = 0; i < runtimes.Size(); i++)
    {
        ParticleRuntime& runtime = runtimes[i];
        const Util::Array<ParticleSystemRuntime>& systems = allSystems[i];

        Timing::Time timeDiff = ctx.time - runtime.stepTime;
        n_assert(timeDiff >= 0.0f);
        if (timeDiff > 0.5f)
        {
            runtime.stepTime = ctx.time - StepTime;
            timeDiff = StepTime;
        }

        const Models::NodeInstanceRange& stateRange = Models::ModelContext::GetModelRenderableRange(graphicsEntities[i]);
        const Models::NodeInstanceRange& transformRange = Models::ModelContext::GetModelTransformableRange(graphicsEntities[i]);

        if (runtime.playing)
        {
            IndexT j;
            for (j = 0; j < systems.Size(); j++)
            {
                ParticleSystemRuntime& system = systems[j];
                system.transform = transformables.nodeTransforms[transformRange.begin + renderables.nodeTransformIndex[stateRange.begin + system.renderableIndex]];

                auto node = reinterpret_cast<Models::ParticleSystemNode*>(renderables.nodes[stateRange.begin + system.renderableIndex]);

#if NEBULA_USED_FIXED_PARTICLE_UPDATE_TIME
                IndexT curStep = 0;
                while (runtime.stepTime <= ctx.time)
                {
                    ParticleContext::EmitParticles(runtime, system, node, ParticleContext::StepTime);
                    bool isLastJob = (runtime.stepTime + ParticleContext::StepTime) > ctx.time;
                    ParticleContext::RunParticleStep(runtime, system, ParticleContext::StepTime, isLastJob);
                    runtime.stepTime += ParticleContext::StepTime;
                    curStep++;
                }
#else
                if (!runtime.stopping)
                {
                    ParticleContext::EmitParticles(runtime, system, node, float(timeDiff));
                }
                ParticleContext::RunParticleStep(runtime, system, float(timeDiff), true);
                runtime.stepTime += timeDiff;
#endif
            }
        }


    }

    // issue sync
    if (runtimes.Size() > 0)
        Jobs::JobSyncThreadSignal(jobSync, Graphics::GraphicsServer::renderSystemsJobPort);

}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(PrepareView, Particles);
    const Util::Array<Util::Array<ParticleSystemRuntime>>& allSystems = particleContextAllocator.GetArray<ParticleSystems>();
    const Models::ModelContext::ModelInstance::Renderable& renderables = ModelContext::GetModelRenderables();
    const Util::Array<Graphics::GraphicsEntityId>& graphicsEntities = particleContextAllocator.GetArray<ModelContextId>();

    Math::mat4 invViewMatrix = inverse(Graphics::CameraContext::GetTransform(view->GetCamera()));
    IndexT i, j;

    // update billboard particles
    for (i = 0; i < allSystems.Size(); i++)
    {

        Util::Array<ParticleSystemRuntime>& systems = allSystems[i];
        const Models::NodeInstanceRange& stateRange = Models::ModelContext::GetModelRenderableRange(graphicsEntities[i]);
        for (j = 0; j < systems.Size(); j++)
        {
            ParticleSystemRuntime& system = systems[j];
            ParticleSystemNode* pnode = reinterpret_cast<ParticleSystemNode*>(renderables.nodes[stateRange.begin + system.renderableIndex]);
            Math::mat4 particleTransform = system.transform;
            if (pnode->GetEmitterAttrs().GetBool(Particles::EmitterAttrs::Billboard))
                system.transform = invViewMatrix * system.transform;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::WaitForParticleUpdates(const Graphics::FrameContext& ctx)
{
    N_SCOPE(WaitForParticleJobs, Particles);
    if (ParticleContext::runningJobs.Size() > 0)
    {
        // wait for all jobs to finish
        Jobs::JobSyncHostWait(ParticleContext::jobSync);

        // destroy jobs
        while (!ParticleContext::runningJobs.IsEmpty())
            Jobs::DestroyJob(ParticleContext::runningJobs.Dequeue());
    }

    // get node map
    Util::Array<Util::Array<ParticleSystemRuntime>>& allSystems = particleContextAllocator.GetArray<ParticleSystems>();
    const Util::Array<Graphics::GraphicsEntityId>& models = particleContextAllocator.GetArray<ModelContextId>();
    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();

    SizeT numParticlesThisFrame = 0;

    // get frame to modify
    IndexT frame = CoreGraphics::GetBufferedFrameIndex();

    // walk through all particles and update their bounding boxes 
    IndexT i;
    for (i = 0; i < allSystems.Size(); i++)
    {
        const Util::Array<ParticleSystemRuntime>& systems = allSystems[i];
        const NodeInstanceRange& stateRange = Models::ModelContext::GetModelRenderableRange(models[i]);

        // TODO: Can't we make this a part of the particle chain system job chain? Run one job per particle system, and also update constants and whatnot there?
        IndexT j;
        for (j = 0; j < systems.Size(); j++)
        {
            ParticleSystemRuntime& system = systems[j];
            system.boundingBox = system.outputData->bbox;
            n_assert(system.outputData != nullptr);
            if (system.outputData->numLivingParticles > 0)
            {
                renderables.nodeBoundingBoxes[stateRange.begin + system.renderableIndex] = system.outputData->bbox;
                SetBits(renderables.nodeFlags[stateRange.begin + system.renderableIndex], NodeInstance_Active);
                numParticlesThisFrame += system.outputData->numLivingParticles;

                ParticleSystemNode* pnode = reinterpret_cast<ParticleSystemNode*>(renderables.nodes[stateRange.begin + system.renderableIndex]);

                ::Particle::ParticleObjectBlock block;

                // update system transform
                system.transform.store(block.EmitterTransform);

                // update parameters
                block.Billboard = pnode->emitterAttrs.GetBool(EmitterAttrs::Billboard);
                block.NumAnimPhases = pnode->emitterAttrs.GetInt(EmitterAttrs::AnimPhases);
                block.AnimFramesPerSecond = pnode->emitterAttrs.GetFloat(EmitterAttrs::PhasesPerSecond);

                // allocate block
                uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, block);
                renderables.nodeStates[stateRange.begin + system.renderableIndex].resourceTableOffsets[renderables.nodeStates[stateRange.begin + system.renderableIndex].particleConstantsIndex] = offset;
            }
            else
                UnsetBits(renderables.nodeFlags[stateRange.begin + system.renderableIndex], NodeInstance_Active);
        }
    }

    // check if we need to realloc buffers
    if (numParticlesThisFrame * state.vertexSize > state.vboSizes[frame])
    {
        CoreGraphics::BufferCreateInfo vboInfo;
        vboInfo.name = "Particle Vertex Buffer";
        vboInfo.size = numParticlesThisFrame;
        vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(state.layout);
        vboInfo.mode = CoreGraphics::HostToDevice;
        vboInfo.usageFlags = CoreGraphics::VertexBuffer;
        vboInfo.data = nullptr;
        vboInfo.dataSize = 0;

        // delete old if needed
        if (state.vbos[frame] != CoreGraphics::InvalidBufferId)
        {
            CoreGraphics::BufferUnmap(state.vbos[frame]);
            CoreGraphics::DestroyBuffer(state.vbos[frame]);
        }
        state.vbos[frame] = CoreGraphics::CreateBuffer(vboInfo);
        state.mappedVertices[frame] = (byte*)CoreGraphics::BufferMap(state.vbos[frame]);
        state.vboSizes[frame] = numParticlesThisFrame * state.vertexSize;
    }

    IndexT baseVertex = 0;

    // walk through systems again and update index and vertex buffers
    float* buf = (float*)state.mappedVertices[frame];
    Math::vec4 tmp;
    bool flushBuffer = false;
    for (i = 0; i < allSystems.Size(); i++)
    {
        const Util::Array<ParticleSystemRuntime>& systems = allSystems[i];
        const NodeInstanceRange& stateRange = Models::ModelContext::GetModelRenderableRange(models[i]);

        IndexT j;
        for (j = 0; j < systems.Size(); j++)
        {
            ParticleSystemRuntime& system = systems[j];
            SizeT numParticles = 0;

            // stream update vertex buffer region
            IndexT k;
            for (k = 0; k < system.particles.Size(); k++)
            {
                const Particle& particle = system.particles[k];
                if (particle.relAge < 1.0f && particle.color.w > 0.001f)
                {
                    particle.position.stream(buf); buf += 4;
                    particle.stretchPosition.stream(buf); buf += 4;
                    particle.color.stream(buf); buf += 4;
                    particle.uvMinMax.stream(buf); buf += 4;
                    float sinRot = Math::sin(particle.rotation);
                    float cosRot = Math::cos(particle.rotation);
                    tmp.set(sinRot, cosRot, particle.size, particle.particleId);
                    tmp.stream(buf); buf += 4;
                    numParticles++;
                }
            }

            // Setup draw modifiers
            renderables.nodeDrawModifiers[stateRange.begin + system.renderableIndex] = Util::MakeTuple(numParticles, baseVertex);
            system.baseVertex = baseVertex;

            // Bump base vertex with number of particles from this system
            baseVertex += numParticles;

            if (numParticles > 0)
                flushBuffer = true;
        }
    }

    // Signal particle done
    Jobs::JobSyncHostSignal(ParticleContext::particleSync, false);

    // flush changes
    if (flushBuffer)
        CoreGraphics::BufferFlush(state.vbos[frame]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId 
ParticleContext::GetParticleIndexBuffer()
{
    return state.geometryIbo;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId 
ParticleContext::GetParticleVertexBuffer()
{
    return state.geometryVbo;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::VertexLayoutId 
ParticleContext::GetParticleVertexLayout()
{
    return state.layout;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PrimitiveGroup
ParticleContext::GetParticlePrimitiveGroup()
{
    return state.primGroup;
}

#ifndef PUBLIC_DEBUG    
//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnRenderDebug(uint32_t flags)
{
    CoreGraphics::ShapeRenderer* shapeRenderer = CoreGraphics::ShapeRenderer::Instance();
    Util::Array<Util::Array<ParticleSystemRuntime>>& allSystems = particleContextAllocator.GetArray<ParticleSystems>();
    for (IndexT i = 0; i < allSystems.Size(); i++)
    {
        Util::Array<ParticleSystemRuntime> runtimes = allSystems[i];
        for (IndexT j = 0; j < runtimes.Size(); j++)
        {
            // for each system, make a white box
            shapeRenderer->AddWireFrameBox(runtimes[j].boundingBox, Math::vec4(1));
        }
    }
}
#endif

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::EmitParticles(ParticleRuntime& rt, ParticleSystemRuntime& srt, Models::ParticleSystemNode* node, float stepTime)
{
    N_SCOPE(EmitParticles, Particles);

    // get the (wrapped around if looping) time since emission has started
    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
    const Particles::EmitterAttrs& attrs = node->GetEmitterAttrs();
    const Particles::EmitterMesh& mesh = node->GetEmitterMesh();
    const Particles::EnvelopeSampleBuffer& buffer = node->GetSampleBuffer();
    rt.emissionStartTimeOffset += stepTime;
    Timing::Time emDuration = attrs.GetFloat(EmitterAttrs::EmissionDuration);
    Timing::Time startDelay = attrs.GetFloat(EmitterAttrs::StartDelay);
    Timing::Time loopTime = emDuration + startDelay;
    bool looping = attrs.GetBool(EmitterAttrs::Looping);
    if (looping && (rt.emissionStartTimeOffset > loopTime))
    {
        // a wrap-around
        rt.emissionStartTimeOffset = Math::fmod((float)rt.emissionStartTimeOffset, (float)loopTime);
    }

    // if we are before the start delay, we definitely don't need to emit anything
    if (rt.emissionStartTimeOffset < startDelay)
    {
        // we're before the start delay
        return;
    }
    else if ((rt.emissionStartTimeOffset > loopTime) && !looping)
    {
        // we're past the emission time
        if (0 == (rt.stopping | rt.stopping))
        {
            rt.stopped = 1;
        }
        return;
    }

    // compute the relative emission time (0.0 .. 1.0)
    Timing::Time relEmissionTime = (rt.emissionStartTimeOffset - startDelay) / emDuration;
    IndexT emSampleIndex = IndexT(relEmissionTime * (Particles::ParticleContextNumEnvelopeSamples - 1));

    // lookup current emission frequency
    float emFrequency = buffer.LookupSamples(emSampleIndex)[EmitterAttrs::EmissionFrequency];
    if (emFrequency > N_TINY)
    {
        Timing::Time emTimeStep = 1.0 / emFrequency;

        // if we haven't emitted particles in this run yet, we need to compute
        // the precalc-time and initialize the lastEmissionTime
        if (rt.firstFrame)
        {
            rt.firstFrame = false;

            // handle pre-calc if necessary, this will instantly produce
            // a number of particles to let the particle system
            // appear as if has been emitting for quite a while already
            float preCalcTime = attrs.GetFloat(EmitterAttrs::PrecalcTime);
            if (preCalcTime > N_TINY)
            {
                // during pre-calculation we need to update the particle system,
                // but we do this with a lower step-rate for better performance
                // (but less precision)
                rt.prevEmissionTime = 0.0f;
                Timing::Time updateTime = 0.0f;
                Timing::Time updateStep = 0.05f;
                if (updateStep < emTimeStep)
                {
                    updateStep = emTimeStep;
                }
                while (rt.prevEmissionTime < preCalcTime)
                {
                    ParticleContext::EmitParticle(rt, srt, attrs, mesh, buffer, emSampleIndex, 0.0f);
                    rt.prevEmissionTime += emTimeStep;
                    updateTime += emTimeStep;
                    if (updateTime >= updateStep)
                    {
                        ParticleContext::RunParticleStep(rt, srt, (float)updateStep, false);
                        updateTime = 0.0f;
                    }
                }
            }

            // setup the lastEmissionTime for particle emission so that at least
            // one frame's worth of particles is emitted, this is necessary
            // for very short-emitting particles
            rt.prevEmissionTime = rt.stepTime;
            if (stepTime > emTimeStep)
            {
                rt.prevEmissionTime -= (stepTime + N_TINY);
            }
            else
            {
                rt.prevEmissionTime -= (emTimeStep + N_TINY);
            }
        }

        // handle the "normal" particle emission while the particle system is emitting
        while ((rt.prevEmissionTime + emTimeStep) <= rt.stepTime)
        {
            rt.prevEmissionTime += emTimeStep;
            ParticleContext::EmitParticle(rt, srt, attrs, mesh, buffer, emSampleIndex, (float)(rt.stepTime - rt.prevEmissionTime));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::EmitParticle(ParticleRuntime& rt, ParticleSystemRuntime& srt, const Particles::EmitterAttrs& attrs, const Particles::EmitterMesh& mesh, const Particles::EnvelopeSampleBuffer& buffer, IndexT sampleIndex, float initialAge)
{
    N_SCOPE(EmitParticle, Particles);
    n_assert(initialAge >= 0.0f);

    using namespace Math;

    Particle particle;
    float* particleEnvSamples = buffer.LookupSamples(0);
    float* emissionEnvSamples = buffer.LookupSamples(sampleIndex);

    // lookup pseudo-random emitter vertex from the emitter mesh
    const EmitterMesh::EmitterPoint& emPoint = mesh.GetEmitterPoint(srt.emissionCounter++);

    // setup particle position and start position
    particle.position = srt.transform * emPoint.position;
    particle.startPosition = particle.position;
    particle.stretchPosition = particle.position;

    // compute emission direction
    float minSpread = emissionEnvSamples[EmitterAttrs::SpreadMin];
    float maxSpread = emissionEnvSamples[EmitterAttrs::SpreadMax];
    float theta = Math::deg2rad(Math::lerp(minSpread, maxSpread, Math::rand()));
    float rho = N_PI_DOUBLE * Math::rand();
    mat4 rot = rotationaxis(xyz(emPoint.tangent), theta) * rotationaxis(xyz(emPoint.normal), rho);
    vec4 emNormal = rot * emPoint.normal;

    vec3 dummy, dummy2;
    quat qrot;
    decompose(srt.transform, dummy, qrot, dummy2);
    emNormal = rotationquat(qrot) * emNormal;
    // compute start velocity
    float velocityVariation = 1.0f - (Math::rand() * attrs.GetFloat(EmitterAttrs::VelocityRandomize));
    float startVelocity = emissionEnvSamples[EmitterAttrs::StartVelocity] * velocityVariation;

    // setup particle velocity vector
    particle.velocity = emNormal * startVelocity;

    // setup uvMinMax to a random texture tile
    // FIXME: what's up with the horizontal flip?
    float texTile = Math::clamp(attrs.GetFloat(EmitterAttrs::TextureTile), 1.0f, 16.0f);
    float step = 1.0f / texTile;
    float tileIndex = floorf(Math::rand() * texTile);
    float vMin = step * tileIndex;
    float vMax = vMin + step;
    particle.uvMinMax.set(1.0f, 1.0f - vMin, 0.0f, 1.0f - vMax);

    // setup initial particle color and clamp to 0,0,0
    particle.color.load(&(particleEnvSamples[EmitterAttrs::Red]));

    // setup rotation and rotationVariation
    float startRotMin = attrs.GetFloat(EmitterAttrs::StartRotationMin);
    float startRotMax = attrs.GetFloat(EmitterAttrs::StartRotationMax);
    particle.rotation = Math::clamp(Math::rand(), startRotMin, startRotMax);
    float rotVar = 1.0f - (Math::rand() * attrs.GetFloat(EmitterAttrs::RotationRandomize));
    if (attrs.GetBool(EmitterAttrs::RandomizeRotation) && (Math::rand() < 0.5f))
    {
        rotVar = -rotVar;
    }
    particle.rotationVariation = rotVar;

    // setup particle size and size variation
    particle.size = particleEnvSamples[EmitterAttrs::Size];
    particle.sizeVariation = 1.0f - (Math::rand() * attrs.GetFloat(EmitterAttrs::SizeRandomize));

    // setup particle age and oneDivLifetime, clamp lifetime to 1.0f if there's a risk we will divide by 0
    float lifeTime = emissionEnvSamples[EmitterAttrs::LifeTime];
    if (lifeTime >= 1.0f)
    {
        particle.oneDivLifeTime = 1.0f / lifeTime;
    }
    else
    {
        particle.oneDivLifeTime = 0.0f;
    }
    particle.relAge = initialAge * particle.oneDivLifeTime;
    particle.age = initialAge;

    // group particles dependent on its lifetime
    if (particle.relAge < 0.25f)
    {
        particle.particleId = 4;
    }
    else if (particle.relAge < 0.5f)
    {
        particle.particleId = 3;
    }
    else if (particle.relAge < 0.75f)
    {
        particle.particleId = 2;
    }
    else
    {
        particle.particleId = 1;
    }
    //this->particleId = !this->particleId;
    // set particle id, just switch between 0 and 1 
    //particle.particleId = (float)this->particleId;    
    //if (++this->particleId > 3) this->particleId = 0;         

    // add the new particle to the particle ring buffer
    srt.particles.Add(particle);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::RunParticleStep(ParticleRuntime& rt, ParticleSystemRuntime& srt, float stepTime, bool generateVtxList)
{
    N_SCOPE(RunParticleStep, Particles);
    // if no particles, no need to run the step update
    if (srt.particles.Size() == 0)
        return;

    Jobs::JobContext ctx;

    n_assert(srt.particles.GetBuffer());
    const SizeT inputBufferSize = srt.particles.Size() * ParticleJobInputElementSize;
    const SizeT inputSliceSize = inputBufferSize;

    ctx.input.data[0] = srt.particles.GetBuffer();
    ctx.input.dataSize[0] = inputBufferSize;
    ctx.input.sliceSize[0] = inputSliceSize;
    ctx.input.numBuffers = 1;

    const SizeT outputSliceCount = (inputBufferSize + inputSliceSize - 1) / inputSliceSize;
    n_assert(outputSliceCount > 0);
    if (outputSliceCount > srt.outputCapacity)
    {
        if (srt.outputData)
            n_delete_array(srt.outputData);

        srt.outputData = n_new_array(ParticleJobOutput, outputSliceCount);
        srt.outputCapacity = outputSliceCount;
    }

    ctx.output.data[0] = srt.particles.GetBuffer();
    ctx.output.dataSize[0] = inputBufferSize;
    ctx.output.sliceSize[0] = inputSliceSize;

    ctx.output.data[1] = srt.outputData;
    ctx.output.dataSize[1] = outputSliceCount * sizeof(ParticleJobOutput);
    ctx.output.sliceSize[1] = sizeof(ParticleJobOutput);
    ctx.output.numBuffers = 2;

    // issue job
    Jobs::JobId job = Jobs::CreateJob({ Particles::ParticleStepJob });

    // pass in uniforms from system which is not step-dependent
    ctx.uniform.data[0] = &srt.uniformData;
    ctx.uniform.dataSize[0] = sizeof(srt.uniformData);

    // create a copy of the uniforms, because the step size may change between every step,
    // which causes bugs if we do precalculation while we are changing the value
    srt.perJobUniformData.stepTime = stepTime;
    void* uniformCopy = Jobs::JobAllocateScratchMemory(job, Memory::HeapType::ScratchHeap, sizeof(ParticleJobUniformPerJobData));
    memcpy(uniformCopy, &srt.perJobUniformData, sizeof(ParticleJobUniformPerJobData));
    ctx.uniform.data[1] = uniformCopy;
    ctx.uniform.dataSize[1] = sizeof(ParticleJobUniformPerJobData);
    ctx.uniform.numBuffers = 2;

    // schedule job
    Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, ctx);

    // add to list of running jobs
    ParticleContext::runningJobs.Enqueue(job);
}

} // namespace Particles
