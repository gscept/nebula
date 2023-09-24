//------------------------------------------------------------------------------
// framescriptloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framescriptloader.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "framepass.h"
#include "frameblit.h"
#include "framecopy.h"
#include "framemipmap.h"
#include "framecompute.h"
#include "frameplugin.h"
#include "framesubpass.h"
#include "framesubgraph.h"
#include "frameswap.h"
#include "framesubpassplugin.h"
#include "framesubpassbatch.h"
#include "framesubpassorderedbatch.h"
#include "framesubpassfullscreeneffect.h"
#include "frameresolve.h"
#include "frameserver.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/config.h"
#include "framesubpassplugin.h"
#include "framebarrier.h"
#include "coregraphics/barrier.h"
#include "coregraphics/displaydevice.h"
#include "memory/arenaallocator.h"
#include <mutex>

using namespace CoreGraphics;
using namespace IO;
namespace Frame
{

Frame::FrameSubmission* FrameScriptLoader::LastSubmission[2] = {nullptr, nullptr};
//------------------------------------------------------------------------------
/**
*/
Ptr<Frame::FrameScript>
FrameScriptLoader::LoadFrameScript(const IO::URI& path)
{
    Ptr<FrameScript> script = FrameScript::Create();
    Ptr<IO::Stream> stream = IoServer::Instance()->CreateStream(path);
    if (stream->Open())
    {
        void* data = stream->Map();
        SizeT size = stream->GetSize();

        // create copy for jzon
        char* jzon_buf = new char[size+1];
        memcpy(jzon_buf, data, size);
        jzon_buf[size] = '\0';

        stream->Unmap();
        stream->Close();

        // make sure last byte is 0, since jzon doesn't care about input size
        JzonParseResult result = jzon_parse(jzon_buf);
        JzonValue* json = result.output;
        if (!result.success)
        {
            n_error("jzon parse error around: '%.100s'\n", result.error);
        }

        // assert version is compatible
        JzonValue* main = jzon_get(json, "framescript");
        n_assert(main != nullptr);

        JzonValue* node = jzon_get(main, "version");
        n_assert(node->int_value >= 3);
        node = jzon_get(main, "engine");
        n_assert(Util::String(node->string_value) == "Nebula");

        // run parser entry point
        ParseFrameScript(script, main);

        // clear jzon
        delete[] jzon_buf;
        jzon_free(json);

    }
    else
    {
        n_error("Failed to load frame script '%s'\n", path.LocalPath().AsCharPtr());
    }
    return script;
}


//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseFrameScript(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        Util::String name(cur->key);
        if (name == "textures")                         ParseTextureList(script, cur);
        else if (name == "read_write_buffers")          ParseReadWriteBufferList(script, cur);
        else if (name == "submissions")					ParseSubmissionList(script, cur);
        else if (name == "comment" || name == "_comment" || name == "version" || name == "engine") continue; // Skip these
        else
        {
            n_error("Frame script operation '%s' is unrecognized.\n", name.AsCharPtr());
        }
    }

    // set end of frame barrier if not a sub-script
    if (!script->subScript)
    {
        n_assert_fmt(FrameScriptLoader::LastSubmission[GraphicsQueueType] != nullptr, "Frame script '%s' must contain an end submission, otherwise the script is invalid!", script->GetResourceName().Value());
        FrameScriptLoader::LastSubmission[GraphicsQueueType]->resourceResetBarriers = &script->resourceResetBarriers;
        FrameScriptLoader::LastSubmission[GraphicsQueueType] = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameScriptLoader::ParseTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        JzonValue* name = jzon_get(cur, "name");
        n_assert(name != nullptr);
        if (Util::String(name->string_value) == "__WINDOW__")
        {
            // code to fetch window render texture goes here
            CoreGraphics::TextureId tex = FrameServer::Instance()->GetWindowTexture();
            script->AddTexture("__WINDOW__", tex);

            // save the window used for the relative dimensions used for this framescript
            script->window = DisplayDevice::Instance()->GetCurrentWindow();
        }
        else
        {
            JzonValue* format = jzon_get(cur, "format");
            n_assert(format != nullptr);
            JzonValue* width = jzon_get(cur, "width");
            n_assert(width != nullptr);
            JzonValue* height = jzon_get(cur, "height");
            n_assert(height != nullptr);

            JzonValue* usage = jzon_get(cur, "usage");
            n_assert(usage != nullptr);
            JzonValue* type = jzon_get(cur, "type");
            n_assert(type != nullptr);

            // get format
            CoreGraphics::PixelFormat::Code fmt = CoreGraphics::PixelFormat::FromString(format->string_value);

            // create texture
            TextureCreateInfo info;
            info.name = name->string_value;
            info.usage = TextureUsageFromString(usage->string_value) | DeviceExclusive;
            info.tag = "frame_script"_atm;
            info.type = TextureTypeFromString(type->string_value);
            info.format = fmt;

            if (JzonValue* alias = jzon_get(cur, "alias"))
                info.alias = script->GetTexture(alias->string_value);

            if (JzonValue* layers = jzon_get(cur, "layers"))
            {
                n_assert2(info.type >= Texture1DArray, "Texture format must be array type if the layers value is set");
                info.layers = layers->int_value;
            }

            if (JzonValue* mips = jzon_get(cur, "mips"))
                if (Util::String(mips->string_value) == "auto")
                    info.mips = TextureAutoMips;
                else
                    info.mips = mips->int_value;

            if (JzonValue* depth = jzon_get(cur, "depth"))
                info.depth = (float)depth->float_value;

            // set relative, dynamic or msaa if defined
            if (jzon_get(cur, "relative"))  info.windowRelative = jzon_get(cur, "relative")->bool_value;
            if (jzon_get(cur, "samples"))   info.samples = jzon_get(cur, "samples")->int_value;
            if (jzon_get(cur, "sparse"))    info.sparse = jzon_get(cur, "sparse")->int_value;

            // set dimension after figuring out if the texture is a cube
            info.width = (float)width->float_value;
            info.height = (float)height->float_value;

            // add to script
            TextureId tex = CreateTexture(info);
            script->AddTexture(name->string_value, tex);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        JzonValue* name = jzon_get(cur, "name");
        n_assert(name != nullptr);
        JzonValue* size = jzon_get(cur, "size");
        n_assert(size != nullptr);

        // create shader buffer 
        BufferCreateInfo info;
        info.name = name->string_value;
        info.size = 1;
        info.elementSize = size->int_value;
        info.mode = CoreGraphics::DeviceLocal;
        info.usageFlags = CoreGraphics::ReadWriteBuffer;
        
        // add to script
        BufferId buf = CreateBuffer(info);
        script->AddBuffer(name->string_value, buf);
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameBlit* op = script->GetAllocator().Alloc<FrameBlit>();

    // set name of op
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* queue = jzon_get(node, "queue");
    if (queue == nullptr)
        op->queue = CoreGraphics::QueueType::GraphicsQueueType;
    else
        op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

    JzonValue* from = jzon_get(node, "from");
    n_assert(from != nullptr);

    JzonValue* to = jzon_get(node, "to");
    n_assert(to != nullptr);

    CoreGraphics::TextureId fromTex, toTex;
    CoreGraphics::ImageBits fromBits = CoreGraphics::ImageBits::Auto, toBits = CoreGraphics::ImageBits::Auto;

    JzonValue* from_tex = jzon_get(from, "tex");
    n_assert(from_tex != nullptr);
    fromTex = script->GetTexture(from_tex->string_value);

    JzonValue* from_bits = jzon_get(from, "bits");
    if (from_bits != nullptr)
        fromBits = ImageBitsFromString(from_bits->string_value);

    JzonValue* to_tex = jzon_get(to, "tex");
    n_assert(to_tex != nullptr);
    toTex = script->GetTexture(to_tex->string_value);

    JzonValue* to_bits = jzon_get(to, "bits");
    if (to_bits != nullptr)
        toBits = ImageBitsFromString(to_bits->string_value);

    // If bits are auto, resolve them using the format
    if (fromBits == CoreGraphics::ImageBits::Auto)
        fromBits = CoreGraphics::PixelFormat::ToImageBits(CoreGraphics::TextureGetPixelFormat(fromTex));
    if (toBits == CoreGraphics::ImageBits::Auto)
        toBits = CoreGraphics::PixelFormat::ToImageBits(CoreGraphics::TextureGetPixelFormat(toTex));

    // add implicit barriers
    TextureSubresourceInfo subresFrom = TextureSubresourceInfo::Texture(fromBits, fromTex);
    TextureSubresourceInfo subresTo = TextureSubresourceInfo::Texture(toBits, toTex);

    op->textureDeps.Add(fromTex, Util::MakeTuple(from->string_value, CoreGraphics::PipelineStage::TransferRead, subresFrom));
    op->textureDeps.Add(toTex, Util::MakeTuple(to->string_value, CoreGraphics::PipelineStage::TransferWrite, subresTo));

    // setup blit operation
    op->fromBits = fromBits;
    op->toBits = toBits;
    op->from = fromTex;
    op->to = toTex;
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameCopy* op = script->GetAllocator().Alloc<FrameCopy>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* queue = jzon_get(node, "queue");
    if (queue == nullptr)
        op->queue = CoreGraphics::QueueType::GraphicsQueueType;
    else
        op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

    JzonValue* from = jzon_get(node, "from");
    n_assert(from != nullptr);

    JzonValue* to = jzon_get(node, "to");
    n_assert(to != nullptr);

    CoreGraphics::TextureId fromTex, toTex;
    CoreGraphics::ImageBits fromBits = CoreGraphics::ImageBits::Auto, toBits = CoreGraphics::ImageBits::Auto;

    JzonValue* from_tex = jzon_get(from, "tex");
    n_assert(from_tex != nullptr);
    fromTex = script->GetTexture(from_tex->string_value);

    JzonValue* from_bits = jzon_get(from, "bits");
    if (from_bits != nullptr)
        fromBits = ImageBitsFromString(from_bits->string_value);

    JzonValue* to_tex = jzon_get(to, "tex");
    n_assert(to_tex != nullptr);
    toTex = script->GetTexture(to_tex->string_value);

    JzonValue* to_bits = jzon_get(to, "bits");
    if (to_bits != nullptr)
        toBits = ImageBitsFromString(to_bits->string_value);

    // If bits are auto, resolve them using the format
    if (fromBits == CoreGraphics::ImageBits::Auto)
        fromBits = CoreGraphics::PixelFormat::ToImageBits(CoreGraphics::TextureGetPixelFormat(fromTex));
    if (toBits == CoreGraphics::ImageBits::Auto)
        toBits = CoreGraphics::PixelFormat::ToImageBits(CoreGraphics::TextureGetPixelFormat(toTex));

    // add implicit barriers
    TextureSubresourceInfo subresFrom = TextureSubresourceInfo::Texture(fromBits, fromTex);
    TextureSubresourceInfo subresTo = TextureSubresourceInfo::Texture(toBits, toTex);

    op->textureDeps.Add(fromTex, Util::MakeTuple(from->string_value, CoreGraphics::PipelineStage::TransferRead, subresFrom));
    op->textureDeps.Add(toTex, Util::MakeTuple(to->string_value, CoreGraphics::PipelineStage::TransferWrite, subresTo));

    // setup copy operation
    op->fromBits = fromBits;
    op->toBits = toBits;
    op->from = fromTex;
    op->to = toTex;
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseResolve(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameResolve* op = script->GetAllocator().Alloc<FrameResolve>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* from = jzon_get(node, "from");
    n_assert(from != nullptr);

    JzonValue* to = jzon_get(node, "to");
    n_assert(to != nullptr);

    CoreGraphics::TextureId fromTex, toTex;
    CoreGraphics::ImageBits fromBits = CoreGraphics::ImageBits::Auto, toBits = CoreGraphics::ImageBits::Auto;

    JzonValue* from_tex = jzon_get(from, "tex");
    n_assert(from_tex != nullptr);
    fromTex = script->GetTexture(from_tex->string_value);

    JzonValue* from_bits = jzon_get(from, "bits");
    if (from_bits != nullptr)
        fromBits = ImageBitsFromString(from_bits->string_value);

    JzonValue* to_tex = jzon_get(to, "tex");
    n_assert(to_tex != nullptr);
    toTex = script->GetTexture(to_tex->string_value);

    JzonValue* to_bits = jzon_get(to, "bits");
    if (to_bits != nullptr)
        toBits = ImageBitsFromString(to_bits->string_value);

    // add implicit barriers
    TextureSubresourceInfo subresFrom = TextureSubresourceInfo::Texture(fromBits, fromTex);
    TextureSubresourceInfo subresTo = TextureSubresourceInfo::Texture(toBits, toTex);

    op->textureDeps.Add(fromTex, Util::MakeTuple(from->string_value, CoreGraphics::PipelineStage::TransferRead, subresFrom));
    op->textureDeps.Add(toTex, Util::MakeTuple(to->string_value, CoreGraphics::PipelineStage::TransferWrite, subresTo));

    // setup copy operation
    op->fromBits = fromBits;
    op->toBits = toBits;
    op->from = fromTex;
    op->to = toTex;
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseMipmap(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameMipmap* op = script->GetAllocator().Alloc<FrameMipmap>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* queue = jzon_get(node, "queue");
    if (queue == nullptr)
        op->queue = CoreGraphics::QueueType::GraphicsQueueType;
    else
        op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

    JzonValue* tex = jzon_get(node, "texture");
    n_assert(tex != nullptr);
    const CoreGraphics::TextureId& ttex = script->GetTexture(tex->string_value);
    bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(ttex));

    // add implicit barriers
    CoreGraphics::ImageBits bits = isDepth ? (CoreGraphics::ImageBits::DepthBits | CoreGraphics::ImageBits::StencilBits) : CoreGraphics::ImageBits::ColorBits;
    TextureSubresourceInfo subres = TextureSubresourceInfo::Texture(bits, ttex);
    op->textureDeps.Add(ttex, Util::MakeTuple(tex->string_value, CoreGraphics::PipelineStage::TransferRead, subres));

    // setup copy operation
    op->tex = ttex;
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{   
    FrameCompute* op = script->GetAllocator().Alloc<FrameCompute>();

    // get name of compute sequence
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* queue = jzon_get(node, "queue");
    if (queue == nullptr)
        op->queue = CoreGraphics::QueueType::GraphicsQueueType;
    else
        op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // create shader state
    JzonValue* shader = jzon_get(node, "shader_state");
    n_assert(shader != nullptr);
    ParseShaderState(script, shader, op->shader, op->resourceTable, op->constantBuffers, op->textures);

    JzonValue* variation = jzon_get(node, "variation");
    n_assert(variation != nullptr);
    op->program = ShaderGetProgram(op->shader, ShaderServer::Instance()->FeatureStringToMask(variation->string_value));

    // dimensions, must be 3
    if (JzonValue* dims = jzon_get(node, "dimensions"))
    {
        n_assert(dims->size == 3);
        op->x = dims->array_values[0]->int_value;
        op->y = dims->array_values[1]->int_value;
        op->z = dims->array_values[2]->int_value;
    }
    else if (JzonValue* dims = jzon_get(node, "texture_dimensions"))
    {
        CoreGraphics::TextureId tex = script->GetTexture(dims->string_value);
        CoreGraphics::TextureDimensions tDims = CoreGraphics::TextureGetDimensions(tex);
        op->x = tDims.width;
        op->y = tDims.height;
        op->z = tDims.depth;
    }

    // add op to script
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParsePlugin(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    FramePlugin* op = script->GetAllocator().Alloc<FramePlugin>();
    op->SetName(name->string_value);

    JzonValue* queue = jzon_get(node, "queue");
    if (queue == nullptr)
        op->queue = CoreGraphics::QueueType::GraphicsQueueType;
    else
        op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // add to script
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseSubgraph(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    FrameSubgraph* op = script->GetAllocator().Alloc<FrameSubgraph>();
    op->SetName(name->string_value);
    op->domain = CoreGraphics::BarrierDomain::Global;

    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameBarrier* op = script->GetAllocator().Alloc<FrameBarrier>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // add operation to script
    return op;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParseSwap(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    FrameSwap* op = script->GetAllocator().Alloc<FrameSwap>();

    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    return op;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubmissionList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        FrameSubmission* submission = script->GetAllocator().Alloc<FrameSubmission>();

        JzonValue* sub = node->array_values[i];
        JzonValue* name = jzon_get(sub, "name");
        n_assert(name != nullptr);
        submission->SetName(name->string_value);

        JzonValue* queue = jzon_get(sub, "queue");
        submission->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

        JzonValue* waitSubmissions = jzon_get(sub, "wait_for_submissions");
        if (waitSubmissions != nullptr)
        {
            uint j;
            for (j = 0; j < waitSubmissions->size; j++)
            {
                Frame::FrameOp* dependency = script->GetOp(waitSubmissions->array_values[j]->string_value);
                n_assert(dependency != nullptr);
                submission->waitSubmissions.Append(static_cast<Frame::FrameSubmission*>(dependency));
            }
        }

        JzonValue* waitQueue = jzon_get(sub, "wait_for_queue");
        if (waitQueue != nullptr)
        {
            CoreGraphics::QueueType queue;
            Util::String q(waitQueue->string_value);
            if (q == "Graphics") queue = CoreGraphics::QueueType::GraphicsQueueType;
            else if (q == "Compute") queue = CoreGraphics::QueueType::ComputeQueueType;
            else if (q == "Transfer") queue = CoreGraphics::QueueType::TransferQueueType;
            else if (q == "Sparse") queue = CoreGraphics::QueueType::SparseQueueType;
            else
            {
                n_error("Unknown queue type '%s'\n", q.AsCharPtr());
            }
            submission->waitQueues.Append(queue);
        }

        JzonValue* ops = jzon_get(sub, "ops");
        if (ops != nullptr)
        {
            uint j = 0;
            for (j = 0; j < ops->size; j++)
            {
                JzonValue* op = ops->array_values[j]->array_values[0];
                Util::String type(op->key);
                if (type == "blit")								submission->AddChild(ParseBlit(script, op));
                else if (type == "copy")                        submission->AddChild(ParseCopy(script, op));
                else if (type == "resolve")                     submission->AddChild(ParseResolve(script, op));
                else if (type == "mipmap")                      submission->AddChild(ParseMipmap(script, op));
                else if (type == "compute")                     submission->AddChild(ParseCompute(script, op));
                else if (type == "pass")                        submission->AddChild(ParsePass(script, op));
                else if (type == "barrier")                     submission->AddChild(ParseBarrier(script, op));
                else if (type == "swap")                        submission->AddChild(ParseSwap(script, op));
                else if (type == "plugin" || type == "call")    submission->AddChild(ParsePlugin(script, op));
                else if (type == "subgraph")                    submission->AddChild(ParseSubgraph(script, op));
                else if (type == "comment" || type == "_comment") continue; // just skip comments
                else
                {
                    n_error("Frame script operation '%s' is unrecognized.\n", type.AsCharPtr());
                }
            }
        }

        // remember the begin
        FrameScriptLoader::LastSubmission[submission->queue] = submission;

        // Create command buffer pool for submission
        CoreGraphics::CmdBufferPoolCreateInfo cmdPoolInfo;
        cmdPoolInfo.queue = submission->queue;
        cmdPoolInfo.resetable = false;
        cmdPoolInfo.shortlived = true;
        submission->commandBufferPool = CoreGraphics::CreateCmdBufferPool(cmdPoolInfo);

        script->AddOp(submission);
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameOp*
FrameScriptLoader::ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
    // create pass
    FramePass* op = script->GetAllocator().Alloc<FramePass>();

    PassCreateInfo info;

    // get name of pass
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    info.name = name->string_value;

    Util::Array<Resources::ResourceName> attachmentNames;

    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        Util::String name(cur->key);
        if (name == "name")                 op->SetName(cur->string_value);
        else if (name == "attachments")     ParseAttachmentList(script, info, attachmentNames, cur);
        else if (name == "subpasses")       ParseSubpassList(script, info, op, attachmentNames, cur);
        else
        {
            n_error("Passes don't support operations, and '%s' is no exception.\n", name.AsCharPtr());
        }
    }

    TextureSubresourceInfo subres;
    subres.layer = 0;
    subres.layerCount = 1;
    subres.mip = 0;
    subres.mipCount = 1;
    for (SizeT i = 0; i < info.attachments.Size(); i++)
    {
        const CoreGraphics::TextureId tex = TextureViewGetTexture(info.attachments[i]);
        subres.layerCount = TextureGetNumLayers(tex);
        subres.mipCount = TextureGetNumMips(tex);
        if (info.attachmentDepthStencil[i])
        {
            subres.bits = CoreGraphics::ImageBits::StencilBits | CoreGraphics::ImageBits::DepthBits;
            op->textureDeps.Add(tex, Util::MakeTuple(attachmentNames[i], CoreGraphics::PipelineStage::DepthStencilWrite, subres));
        }
        else
        {
            subres.bits = CoreGraphics::ImageBits::ColorBits;
            op->textureDeps.Add(tex, Util::MakeTuple(attachmentNames[i], CoreGraphics::PipelineStage::ColorWrite, subres));
        }
    }

    // setup framebuffer and bind to pass
    op->pass = CreatePass(info);
    return op;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseAttachmentList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        JzonValue* name = jzon_get(cur, "name");
        n_assert(name != nullptr);

        TextureId tex = script->GetTexture(name->string_value);
        TextureViewCreateInfo viewCreate =
        {
            Util::String::Sprintf("%s - View Attachment in %s", name->string_value, pass.name.Value())
            , tex
            , 0, 1, 0, TextureGetNumLayers(tex)
            , TextureGetPixelFormat(tex)
        };

        bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));

        if (isDepth)
        {
            viewCreate.bits = CoreGraphics::ImageBits::DepthBits;
        }

        pass.attachments.Append(CreateTextureView(viewCreate));
        attachmentNames.Append(name->string_value);

        // set clear flag if present
        JzonValue* clear = jzon_get(cur, "clear");
        AttachmentFlagBits flags = AttachmentFlagBits::NoFlags;

        JzonValue* flagBits = jzon_get(cur, "flags");
        if (flagBits != nullptr)
        {
            flags = AttachmentFlagsFromString(flagBits->string_value);
            if (AnyBits(flags, AttachmentFlagBits::ClearStencil | AttachmentFlagBits::LoadStencil | AttachmentFlagBits::DiscardStencil))
            {
                n_assert_msg(isDepth, "Format is not depth-stencil");
            }
        }

        if (clear != nullptr)
        {
            n_assert_msg(!isDepth, "Format is depth-stencil, use clear_depth and/or clear_stencil");
            Math::vec4 clearValue;
            n_assert(clear->size <= 4);
            uint j;
            for (j = 0; j < clear->size; j++)
            {
                clearValue[j] = clear->array_values[j]->float_value;
            }
            pass.attachmentClears.Append(clearValue);
            flags |= AttachmentFlagBits::Clear;
        }
        else
            pass.attachmentClears.Append(Math::vec4(1)); // we set the clear to 1, but the flag is not to clear...

        JzonValue* clearDepth = jzon_get(cur, "clear_depth");
        if (clearDepth != nullptr)
        {
            n_assert_msg(isDepth, "Format is not depth-stencil");
            n_assert_msg(clear == nullptr, "Attachments with 'clear_depth' must not also have 'clear'\n");
            pass.attachmentClears.Back().x = clearDepth->float_value;
            flags |= AttachmentFlagBits::Clear;
        }

        JzonValue* clearStencil = jzon_get(cur, "clear_stencil");
        if (clearStencil != nullptr)
        {
            n_assert_msg(isDepth, "Format is not depth-stencil");
            n_assert_msg(clear == nullptr, "Attachments with 'clear_stencil' must not also have 'clear'\n");
            pass.attachmentClears.Back().y = clearStencil->int_value;
            flags |= AttachmentFlagBits::ClearStencil;
        }

        n_assert_msg(!AllBits(flags, AttachmentFlagBits::Clear | AttachmentFlagBits::Load), "Can't both clear and load");
        n_assert_msg(!AllBits(flags, AttachmentFlagBits::ClearStencil | AttachmentFlagBits::LoadStencil), "Can't both clear and load stencil");
        n_assert_msg(!AllBits(flags, AttachmentFlagBits::Store | AttachmentFlagBits::Discard), "Can't both discard and store");
        n_assert_msg(!AllBits(flags, AttachmentFlagBits::StoreStencil | AttachmentFlagBits::DiscardStencil), "Can't both discard and store stencil");

        pass.attachmentFlags.Append((AttachmentFlagBits)flags);
        pass.attachmentDepthStencil.Append(isDepth);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Frame::FramePass* framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        FrameSubpass* frameSubpass = script->GetAllocator().Alloc<FrameSubpass>();

        frameSubpass->domain = BarrierDomain::Pass;
        Subpass subpass;
        subpass.depth = InvalidIndex;

        JzonValue* cur = node->array_values[i];
        uint j;
        for (j = 0; j < cur->size; j++)
        {
            JzonValue* field = cur->array_values[j];
            Util::String name(field->key);
            if (name == "name")                             frameSubpass->SetName(field->string_value);
            else if (name == "subpass_dependencies")        ParseSubpassDependencyList(framePass, subpass, field);
            else if (name == "attachments")                 ParseSubpassAttachmentList(framePass, subpass, attachmentNames, field);
            else if (name == "inputs")                      ParseSubpassInputList(framePass, subpass, attachmentNames, field);
            else if (name == "depth")                       ParseSubpassDepthAttachment(framePass, subpass, attachmentNames, field);
            else if (name == "resolves")                    ParseSubpassResolves(framePass, subpass, attachmentNames, field);
            else if (name == "resource_dependencies")       ParseResourceDependencies(script, framePass, field);
            else if (name == "comment" || name == "_comment") continue; // just skip comments
            else if (name == "ops")
            {
                uint k;
                for (k = 0; k < field->size; k++)
                {
                    JzonValue* op = field->array_values[k]->array_values[0];
                    Util::String type(op->key);
                    if (type == "plugin" || type == "call")         ParseSubpassPlugin(script, frameSubpass, op);
                    else if (type == "subgraph")                    ParseSubpassSubgraph(script, frameSubpass, op);
                    else if (type == "batch")                       ParseSubpassBatch(script, frameSubpass, op);
                    else if (type == "sorted_batch")                ParseSubpassSortedBatch(script, frameSubpass, op);
                    else if (type == "fullscreen_effect")           ParseSubpassFullscreenEffect(script, frameSubpass, op);
                    else if (type == "comment" || type == "_comment") continue; // just skip comments
                    else
                    {
                        n_error("Subpass operation '%s' is invalid.\n", type.AsCharPtr());
                    }
                }
            }
            else
            {
                n_error("Subpass field '%s' is invalid.\n", name.AsCharPtr());
            }
        }

        // make sure the subpass has any attacments
        n_assert2(subpass.attachments.Size() > 0 || subpass.depth != InvalidIndex, "Subpass must at least either bind color attachments or a depth-stencil attachment");

        // correct amount of viewports and scissors if not set explicitly (both values default to 0)
        if (subpass.numViewports == 0)
            subpass.numViewports = subpass.attachments.IsEmpty() ? (subpass.depth == InvalidIndex ? 0 : 1) : subpass.attachments.Size();
        if (subpass.numScissors == 0)
            subpass.numScissors = subpass.attachments.IsEmpty() ? (subpass.depth == InvalidIndex ? 0 : 1) : subpass.attachments.Size();

        // link together frame operations
        pass.subpasses.Append(subpass);
        framePass->AddChild(frameSubpass);
    }
}

//------------------------------------------------------------------------------
/**
    Extend to use subpass names
*/
void
FrameScriptLoader::ParseSubpassDependencyList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        if (cur->is_int)
            subpass.dependencies.Append(cur->int_value);
        else if (cur->is_string)
        {
            Util::String id(cur->string_value);
            const Util::Array<FrameOp*>& subpasses = pass->GetChildren();

            // setup subpass self-dependency
            if (id == "this")
            {
                subpass.dependencies.Append(subpasses.Size() - 1);
            }
            else
            {
                IndexT j;
                for (j = 0; j < subpasses.Size(); j++)
                {
                    if (subpasses[j]->GetName() == id)
                    {
                        subpass.dependencies.Append(j);
                        break;
                    }
                }
                if (j == subpasses.Size())
                {
                    n_error("Could not find previous subpass '%s'", id.AsCharPtr());
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Extend to use attachment names
*/
void
FrameScriptLoader::ParseSubpassAttachmentList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        if (cur->is_int)
            subpass.attachments.Append(cur->int_value);
        else if (cur->is_string)
        {
            Util::String id(cur->string_value);
            IndexT j;
            for (j = 0; j < attachmentNames.Size(); j++)
            {
                if (attachmentNames[j] == id)
                {
                    subpass.attachments.Append(j);
                    break;
                }
            }
            if (j == attachmentNames.Size())
            {
                n_error("Could not find attachment '%s'", id.AsCharPtr());
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassDepthAttachment(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    if (node->is_int)
        subpass.depth = node->int_value;
    else if (node->is_string)
    {
        Util::String id(node->string_value);
        IndexT j;
        for (j = 0; j < attachmentNames.Size(); j++)
        {
            if (attachmentNames[j] == id)
            {
                subpass.depth = j;
                break;
            }
        }
        if (j == attachmentNames.Size())
        {
            n_error("Could not find attachment '%s'", id.AsCharPtr());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassResolves(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        if (cur->is_int)
            subpass.resolves.Append(cur->int_value);
        else if (cur->is_string)
        {
            Util::String id(cur->string_value);
            IndexT j;
            for (j = 0; j < attachmentNames.Size(); j++)
            {
                if (attachmentNames[j] == id)
                {
                    subpass.resolves.Append(j);
                    break;
                }
            }
            if (j == attachmentNames.Size())
            {
                n_error("Could not find attachment '%s'", id.AsCharPtr());
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassInputList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* cur = node->array_values[i];
        if (cur->is_int)
            subpass.inputs.Append(cur->int_value);
        else if (cur->is_string)
        {
            Util::String id(cur->string_value);
            IndexT j;
            for (j = 0; j < attachmentNames.Size(); j++)
            {
                if (attachmentNames[j] == id)
                {
                    subpass.inputs.Append(j);
                    break;
                }
            }
            if (j == attachmentNames.Size())
            {
                n_error("Could not find previous attachment '%s'", id.AsCharPtr());
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassPlugin(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
    FrameSubpassPlugin* op = script->GetAllocator().Alloc<FrameSubpassPlugin>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    op->SetName(name->string_value);
    op->domain = BarrierDomain::Pass;
    op->queue = CoreGraphics::QueueType::GraphicsQueueType;

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // add to script
    subpass->AddChild(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSubgraph(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
    FrameSubgraph* op = script->GetAllocator().Alloc<FrameSubgraph>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    op->SetName(name->string_value);
    op->domain = CoreGraphics::BarrierDomain::Pass;
    op->queue = CoreGraphics::QueueType::GraphicsQueueType;

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // add to script
    subpass->AddChild(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
    FrameSubpassBatch* op = script->GetAllocator().Alloc<FrameSubpassBatch>();
    op->domain = BarrierDomain::Pass;
    op->queue = CoreGraphics::QueueType::GraphicsQueueType;

    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    op->batch = CoreGraphics::BatchGroup::FromName(name->string_value);
    subpass->AddChild(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
    FrameSubpassOrderedBatch* op = script->GetAllocator().Alloc<FrameSubpassOrderedBatch>();
    op->domain = BarrierDomain::Pass;
    op->queue = CoreGraphics::QueueType::GraphicsQueueType;

    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    op->batch = CoreGraphics::BatchGroup::FromName(name->string_value);
    subpass->AddChild(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
    FrameSubpassFullscreenEffect* op = script->GetAllocator().Alloc<FrameSubpassFullscreenEffect>();

    // get function and name
    JzonValue* name = jzon_get(node, "name");
    n_assert(name != nullptr);
    op->SetName(name->string_value);

    op->domain = BarrierDomain::Pass;
    op->queue = CoreGraphics::QueueType::GraphicsQueueType;

    JzonValue* inputs = jzon_get(node, "resource_dependencies");
    if (inputs != nullptr)
    {
        ParseResourceDependencies(script, op, inputs);
    }

    // create shader state
    JzonValue* shaderState = jzon_get(node, "shader_state");
    n_assert(shaderState != nullptr);
    ParseShaderState(script, shaderState, op->shader, op->resourceTable, op->constantBuffers, op->textures);

    // get texture
    JzonValue* texture = jzon_get(node, "size_from_texture");
    n_assert(texture != nullptr);
    op->tex = script->GetTexture(texture->string_value);
    
    // add op to subpass
    op->Setup();
    subpass->AddChild(op);
}


//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderState(
    const Ptr<Frame::FrameScript>& script, 
    JzonValue* node, 
    CoreGraphics::ShaderId& shd, 
    CoreGraphics::ResourceTableId& table, 
    Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& constantBuffers,
    Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>>& textures
)
{
    JzonValue* shader = jzon_get(node, "shader");
    n_assert(shader != nullptr);
    Util::String shaderRes = "shd:" + Util::String(shader->string_value) + ".fxb";
    shd = ShaderGet(shaderRes);
    table = ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP, 1);

    JzonValue* vars = jzon_get(node, "variables");
    if (vars != nullptr)
        ParseShaderVariables(script, shd, table, constantBuffers, textures, vars);
    CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderVariables(
    const Ptr<Frame::FrameScript>& script,
    const CoreGraphics::ShaderId& shd, 
    CoreGraphics::ResourceTableId& table, 
    Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& constantBuffers, 
    Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>>& textures,
    JzonValue* node)
{
    uint i;
    for (i = 0; i < node->size; i++)
    {
        JzonValue* var = node->array_values[i];

        // variables need to define both semantic and value
        JzonValue* sem = jzon_get(var, "semantic");
        n_assert(sem != nullptr);
        JzonValue* val = jzon_get(var, "value");
        n_assert(val != nullptr);
        Util::String valStr(val->string_value);

        // get variable
        ShaderConstantType type = ShaderGetConstantType(shd, sem->string_value);
        BufferId cbo = InvalidBufferId;
        IndexT bufferOffset = InvalidIndex;
        if (type != SamplerVariableType && type != TextureVariableType && type != ImageReadWriteVariableType && type != BufferReadWriteVariableType)
        {
            Util::StringAtom block = ShaderGetConstantBlockName(shd, sem->string_value);
            IndexT bufferIndex = constantBuffers.FindIndex(block);
            IndexT slot = ShaderGetResourceSlot(shd, block);
            if (bufferIndex == InvalidIndex)
            {
                cbo = ShaderCreateConstantBuffer(shd, block);
                ResourceTableSetConstantBuffer(table, { cbo, slot, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
                constantBuffers.Add(block, cbo);
            }
            else
            {
                cbo = constantBuffers.ValueAtIndex(bufferIndex);
            }
            bufferOffset = ShaderGetConstantBinding(shd, sem->string_value);
        }
        switch (type)
        {
        case IntVariableType:
            BufferUpdate(cbo, valStr.AsInt(), bufferOffset);
            break;
        case FloatVariableType:
            BufferUpdate(cbo, valStr.AsFloat(), bufferOffset);
            break;
        case VectorVariableType:
            BufferUpdate(cbo, valStr.AsVec4(), bufferOffset);
            break;
        case Vector2VariableType:
            BufferUpdate(cbo, valStr.AsVec2(), bufferOffset);
            break;
        case MatrixVariableType:
            BufferUpdate(cbo, valStr.AsMat4(), bufferOffset);
            break;
        case BoolVariableType:
            BufferUpdate(cbo, valStr.AsBool(), bufferOffset);
            break;
        case SamplerHandleType:
        case ImageHandleType:
        case TextureHandleType:
        {
            CoreGraphics::TextureId rtid = script->GetTexture(valStr);
            textures.Append(Util::MakeTuple(bufferOffset, cbo, rtid));
            if (rtid != CoreGraphics::InvalidTextureId)
                BufferUpdate(cbo, CoreGraphics::TextureGetBindlessHandle(rtid), bufferOffset);
            else
                n_error("Unknown resource %s!", valStr.AsCharPtr());
            break;
        }
        case SamplerVariableType:
        case TextureVariableType:
        {
            IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
            CoreGraphics::TextureId rtid = script->GetTexture(valStr);

            textures.Append(Util::MakeTuple(slot, InvalidBufferId, rtid));
            if (rtid != CoreGraphics::InvalidTextureId)
                ResourceTableSetTexture(table, { rtid, slot, 0, CoreGraphics::InvalidSamplerId, false });
            else
                n_error("Unknown resource %s!", valStr.AsCharPtr());
            break;
        }
        case ImageReadWriteVariableType:
        {
            IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
            CoreGraphics::TextureId rtid = script->GetTexture(valStr);

            textures.Append(Util::MakeTuple(slot, InvalidBufferId, rtid));
            if (rtid != CoreGraphics::InvalidTextureId)
                ResourceTableSetRWTexture(table, { rtid, slot, 0, CoreGraphics::InvalidSamplerId });
            else
                n_error("Unknown resource %s!", valStr.AsCharPtr());
            break;
        }
        case BufferReadWriteVariableType:
        {
            IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
            CoreGraphics::BufferId rtid = script->GetBuffer(valStr);
            if (rtid != CoreGraphics::InvalidBufferId)
                ResourceTableSetRWBuffer(table, { rtid, slot, 0 });
            else
                n_error("Unknown resource %s!", valStr.AsCharPtr());
            break;
        }
        }
    }

}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node)
{
    n_assert(node->is_array);
    for (uint i = 0; i < node->size; i++)
    {
        JzonValue* dep = node->array_values[i];
        const Util::String valstr = jzon_get(dep, "name")->string_value;
        CoreGraphics::PipelineStage stage = PipelineStageFromString(jzon_get(dep, "stage")->string_value);

        if (script->texturesByName.Contains(valstr))
        {
            CoreGraphics::TextureSubresourceInfo subres;
            JzonValue* nd = nullptr;

            TextureId tex = script->texturesByName[valstr];
            if ((nd = jzon_get(dep, "bits")) != nullptr) subres.bits = ImageBitsFromString(nd->string_value);
            else
            {
                bool isDepth = PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));
                subres.bits = isDepth ? (ImageBits::DepthBits | ImageBits::StencilBits) : ImageBits::ColorBits;
            }
            if ((nd = jzon_get(dep, "mip")) != nullptr) subres.mip = nd->int_value;
            else                                        subres.mip = 0;
            if ((nd = jzon_get(dep, "mip_count")) != nullptr) subres.mipCount = nd->int_value;
            else                                              subres.mipCount = CoreGraphics::TextureGetNumMips(tex) - subres.mip;

            if ((nd = jzon_get(dep, "layer")) != nullptr) subres.layer = nd->int_value;
            else                                          subres.layer = 0;
            if ((nd = jzon_get(dep, "layer_count")) != nullptr) subres.layerCount = nd->int_value;
            else                                                subres.layerCount = CoreGraphics::TextureGetNumLayers(tex) - subres.layer;

            op->textureDeps.Add(tex, Util::MakeTuple(valstr, stage, subres));
        }
        else if (script->buffersByName.Contains(valstr))
        {
            BufferId buf = script->buffersByName[valstr];
            CoreGraphics::BufferSubresourceInfo subres;
            JzonValue* nd = nullptr;
            if ((nd = jzon_get(dep, "offset")) != nullptr) subres.offset = nd->int_value;
            if ((nd = jzon_get(dep, "size")) != nullptr) subres.size = nd->int_value;
            op->bufferDeps.Add(buf, Util::MakeTuple(valstr, stage, subres));
        }
        else
        {
            n_error("No resource named %s declared\n", valstr.AsCharPtr());
        }
    }
}

} // namespace Frame2
