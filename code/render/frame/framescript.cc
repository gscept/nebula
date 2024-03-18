//------------------------------------------------------------------------------
// framescript.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framescript.h"
#include "frameserver.h"
#include "coregraphics/displaydevice.h"

namespace Frame
{

__ImplementClass(Frame::FrameScript, 'FRSC', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
FrameScript::FrameScript() :
    window(Ids::InvalidId32),
    frameOpCounter(0),
    subScript(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameScript::~FrameScript()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId tex)
{
    IndexT i = this->texturesByName.FindIndex(name);
    n_assert(i == InvalidIndex);
    this->texturesByName.Add(name, tex);
    this->textures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddBuffer(const Util::StringAtom& name, const CoreGraphics::BufferId buf)
{
    n_assert(!this->buffersByName.Contains(name));
    this->buffersByName.Add(name, buf);
    this->buffers.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddOp(Frame::FrameOp* op)
{
    op->index = this->frameOpCounter;
    this->frameOpCounter++;
    this->ops.Append(op);
    this->opsByName.Add(op->name, op);
}

//------------------------------------------------------------------------------
/**
*/
Frame::FrameOp*
FrameScript::GetOp(const Util::String& search)
{
    Util::Array<Util::String> paths = search.Tokenize("/");
    Util::Dictionary<Util::StringAtom, FrameOp*>* list = &this->opsByName;
    Frame::FrameOp* op = nullptr;
    while (!paths.IsEmpty())
    {
        IndexT i = list->FindIndex(paths.Front());
        if (i == InvalidIndex)
        {
            n_warning("Could not find operation using path '%s'", search.AsCharPtr());
            return nullptr;
        }

        // get operation for next iteration of loop
        op = list->ValueAtIndex(i);
        list = &op->childrenByName;

        // remove element we just used
        paths.EraseFront();
    }
    return op;
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameScript::Setup()
{
#if NEBULA_ENABLE_MT_DRAW
    this->drawThread = CoreGraphics::CreateDrawThread();
    Util::String scriptName = this->resId.Value();
    scriptName.StripFileExtension();
    Util::String threadName = Util::String::Sprintf("FrameScript %s Draw Thread", scriptName.AsCharPtr());
    this->drawThread->SetName(threadName);
    this->drawThread->SetThreadAffinity(System::Cpu::Core5);
    this->drawThread->Start();
    CoreGraphics::CmdBufferPoolCreateInfo poolInfo =
    {
        CoreGraphics::GraphicsQueueType,
        true,
        true
    };
    this->drawThreadCommandPool = CoreGraphics::CreateCmdBufferPool(poolInfo);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Discard()
{
    // unload ourselves, this is only for convenience
    FrameServer::Instance()->UnloadFrameScript(this->resId);

#if NEBULA_ENABLE_MT_DRAW
    this->drawThread->Stop();
    CoreGraphics::DestroyCmdBufferPool(this->drawThreadCommandPool);
#endif

    this->buildAllocator.Release();
    this->allocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
#if NEBULA_ENABLE_MT_DRAW
    N_MARKER_BEGIN(WaitForRecord, Graphics);

    // wait for draw thread to finish before executing buffers
    this->drawThreadEvent.Wait();

    N_MARKER_END();
#endif

    IndexT i;
    for (i = 0; i < this->compiled.Size(); i++)
    {
        // Top level calls won't have a command buffer, that's handled by the submissions
        this->compiled[i]->Run(CoreGraphics::InvalidCmdBufferId, frameIndex, bufferIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Build()
{
    IndexT i;
    
    // go through and discard all compiled (dunno if needed)
    if (!this->compiled.IsEmpty())
    {
        for (i = 0; i < this->compiled.Size(); i++)
        {
            this->compiled[i]->Discard();
        }
        this->compiled.Clear();
    }

    for (i = 0; i < this->resourceResetBarriers.Size(); i++)
        DestroyBarrier(this->resourceResetBarriers[i]);
    this->resourceResetBarriers.Clear();

    for (i = 0; i < this->events.Size(); i++)
        DestroyEvent(this->events[i]);
    this->events.Clear();

    for (i = 0; i < this->barriers.Size(); i++)
        DestroyBarrier(this->barriers[i]);
    this->barriers.Clear();

    // clear old compiled result
    this->buildAllocator.Release();
     
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<FrameOp::BufferDependency>> buffers;
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<FrameOp::TextureDependency>> textures;

    // get window texture
    CoreGraphics::TextureId window = FrameServer::Instance()->GetWindowTexture();

    // give every resource an initial dependency
    for (i = 0; i < this->textures.Size(); i++)
    {
        CoreGraphics::TextureId tex = this->textures[i];
        bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));
        CoreGraphics::TextureUsage usage = CoreGraphics::TextureGetUsage(tex);
        auto& arr = textures.Emplace(tex);

        uint layers = CoreGraphics::TextureGetNumLayers(tex);
        uint mips = CoreGraphics::TextureGetNumMips(tex);

        CoreGraphics::TextureSubresourceInfo subres;
        subres.bits = isDepth ? (CoreGraphics::ImageBits::DepthBits | CoreGraphics::ImageBits::StencilBits) : CoreGraphics::ImageBits::ColorBits;
        subres.layer = 0;
        subres.layerCount = layers;
        subres.mip = 0;
        subres.mipCount = mips;
        if (tex == window)
            arr.Append(FrameOp::TextureDependency{ CoreGraphics::PipelineStage::Present, DependencyIntent::Read, subres });
        else
        {
            if (AllBits(usage, CoreGraphics::RenderTexture))
            {
                if (isDepth)
                    arr.Append(FrameOp::TextureDependency{ CoreGraphics::PipelineStage::DepthStencilRead, DependencyIntent::Read, subres });
                else
                    arr.Append(FrameOp::TextureDependency{ CoreGraphics::PipelineStage::ColorRead, DependencyIntent::Read, subres });
            }
            else
            {
                arr.Append(FrameOp::TextureDependency{ CoreGraphics::PipelineStage::AllShadersRead, DependencyIntent::Read, subres });
            }
        }
    }

    FrameOp::BuildContext buildContext { CoreGraphics::InvalidPassId, 0xFFFFFFFF, this->buildAllocator, this->compiled, this->events, this->barriers, buffers, textures };

    // build ops
    for (i = 0; i < this->ops.Size(); i++)
    {
        this->ops[i]->Build(buildContext);
    }

    for (i = 0; i < textures.Size(); i++)
    {
        const CoreGraphics::TextureId& tex = textures.KeyAtIndex(i);
        const Util::Array<FrameOp::TextureDependency>& deps = textures.ValueAtIndex(i);
        bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));
        CoreGraphics::TextureUsage usage = CoreGraphics::TextureGetUsage(tex);

        const FrameOp::TextureDependency& dep = deps.Back();
        CoreGraphics::TextureSubresourceInfo info = dep.subres;

        // The last thing we do with present is to transition to present
        CoreGraphics::PipelineStage fromStage, toStage;
        fromStage = dep.stage;
        if (tex == window)
            toStage = CoreGraphics::PipelineStage::Present;
        else
        {
            if (AllBits(usage, CoreGraphics::RenderTexture))
            {
                if (isDepth)
                    toStage = CoreGraphics::PipelineStage::DepthStencilRead;
                else
                    toStage = CoreGraphics::PipelineStage::ColorRead;
            }
            else
            {
                toStage = CoreGraphics::PipelineStage::AllShadersRead;
            }
        }

        // render textures are created as shader read
        if (fromStage != toStage)
        {
            CoreGraphics::BarrierCreateInfo inf =
            {
                Util::String::Sprintf("End of Frame Texture Reset Transition %d", tex.id),
                CoreGraphics::BarrierDomain::Global,
                fromStage,
                toStage,
                CoreGraphics::QueueType::InvalidQueueType,
                CoreGraphics::QueueType::InvalidQueueType,
                {
                    CoreGraphics::TextureBarrierInfo{ tex, info }
                },
                nullptr
            };
            this->resourceResetBarriers.Append(CoreGraphics::CreateBarrier(inf));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Cleanup()
{
    IndexT i;
    for (i = 0; i < this->textures.Size(); i++) DestroyTexture(this->textures[i]);
    this->textures.Clear();
    this->texturesByName.Clear();

    for (i = 0; i < this->buffers.Size(); i++) DestroyBuffer(this->buffers[i]);
    this->buffers.Clear();
    this->buffersByName.Clear();

    for (i = 0; i < this->events.Size(); i++) DestroyEvent(this->events[i]);
    this->events.Clear();

    for (i = 0; i < this->ops.Size(); i++) this->ops[i]->Discard();
    this->ops.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::OnWindowResized()
{
    // only do this if we actually use the window
    if (this->window != Ids::InvalidId32)
    {
        CoreGraphics::WindowId prev = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();

        // make this window current
        WindowMakeCurrent(this->window);

        IndexT i;
        for (i = 0; i < this->textures.Size(); i++)
            TextureWindowResized(this->textures[i]);
        for (i = 0; i < this->ops.Size(); i++)
            this->ops[i]->OnWindowResized();

        Build();

        // reset old window
        WindowMakeCurrent(prev);
    }

}

} // namespace Frame2
