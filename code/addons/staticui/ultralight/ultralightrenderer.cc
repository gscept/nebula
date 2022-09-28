//------------------------------------------------------------------------------
//  @file ultralightrenderer.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ultralightrenderer.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/barrier.h"
#include "graphics/camerasettings.h"
#include "../staticui/staticui.h"
namespace StaticUI
{

struct
{
    CoreGraphics::ShaderProgramId ultralightShader1;
    CoreGraphics::ShaderProgramId ultralightShader2;
    CoreGraphics::ShaderProgramId ultralightShader3;
    CoreGraphics::ResourceTableId resourceTable;

    CoreGraphics::VertexLayoutId vloSmall, vloBig, vloEmpty;

    Staticui::PerDrawState backbufferDrawState;
    uint backbufferDrawOffset;
} ultralightState;

//------------------------------------------------------------------------------
/**
*/
UltralightRenderer::UltralightRenderer()
    : textureId(0)
    , renderBufferId(0)
    , geometryId(0)
{
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:staticui.fxb");
    ultralightState.ultralightShader1 = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureFromString("Ultralight1"));
    ultralightState.ultralightShader2 = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureFromString("Ultralight2"));
    ultralightState.ultralightShader3 = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureFromString("Ultralight3"));

    ultralightState.resourceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_DYNAMIC_OFFSET_GROUP);

    CoreGraphics::ResourceTableSetConstantBuffer(ultralightState.resourceTable,
        {
            CoreGraphics::GetGraphicsConstantBuffer(),
            Staticui::Table_DynamicOffset::PerDrawState::SLOT,
            0,
            Staticui::Table_DynamicOffset::PerDrawState::SIZE, 0,
            false, true,
        });
    CoreGraphics::ResourceTableCommitChanges(ultralightState.resourceTable);

    CoreGraphics::VertexLayoutCreateInfo vloInfo;
    vloInfo.comps =
    {
        CoreGraphics::VertexComponent{ 0, 0, CoreGraphics::VertexComponent::Format::Float2 },
        CoreGraphics::VertexComponent{ 1, 0, CoreGraphics::VertexComponent::Format::UByte4 },
        CoreGraphics::VertexComponent{ 2, 0, CoreGraphics::VertexComponent::Format::Float2 },
    };
    ultralightState.vloSmall = CoreGraphics::CreateVertexLayout(vloInfo);
    vloInfo.comps =
    {
        CoreGraphics::VertexComponent{ 0, 0, CoreGraphics::VertexComponent::Format::Float2 },
        CoreGraphics::VertexComponent{ 1, 0, CoreGraphics::VertexComponent::Format::UByte4 },
        CoreGraphics::VertexComponent{ 2, 0, CoreGraphics::VertexComponent::Format::Float2 },
        CoreGraphics::VertexComponent{ 3, 0, CoreGraphics::VertexComponent::Format::Float2 },
        CoreGraphics::VertexComponent{ 4, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 5, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 6, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 7, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 8, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 9, 0, CoreGraphics::VertexComponent::Format::Float4 },
        CoreGraphics::VertexComponent{ 10, 0, CoreGraphics::VertexComponent::Format::Float4 },
    };
    ultralightState.vloBig = CoreGraphics::CreateVertexLayout(vloInfo);
    vloInfo.comps = {};
    ultralightState.vloEmpty = CoreGraphics::CreateVertexLayout(vloInfo);
}

//------------------------------------------------------------------------------
/**
*/
UltralightRenderer::~UltralightRenderer()
{
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
UltralightRenderer::NextTextureId()
{
    return this->textureId++;
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::CreateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap)
{
    IndexT idx = this->textureMap.FindIndex(texture_id);
    if (idx != InvalidIndex)
    {
        n_printf("UltralightRenderer Texture ID %d already exists\n", texture_id);
        return;
    }

    if (bitmap->format() != ultralight::kBitmapFormat_BGRA8_UNORM_SRGB && bitmap->format() != ultralight::kBitmapFormat_A8_UNORM)
    {
        n_printf("UltralightRenderer Unsupported texture format\n", texture_id);
        return;
    }

    CoreGraphics::TextureCreateInfo texInfo;
    if (bitmap->IsEmpty())
        texInfo.name = Util::String::Sprintf("Ultralight Render Texture %d", texture_id);
    else
        texInfo.name = Util::String::Sprintf("Ultralight Texture %d", texture_id);
    texInfo.mips = 1;
    texInfo.width = bitmap->width();
    texInfo.height = bitmap->height();
    
    switch (bitmap->format())
    {
        case ultralight::kBitmapFormat_BGRA8_UNORM_SRGB:
            texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
            if (!bitmap->IsEmpty())
                texInfo.swizzle = { CoreGraphics::TextureChannelMapping::Blue, CoreGraphics::TextureChannelMapping::Green, CoreGraphics::TextureChannelMapping::Red, CoreGraphics::TextureChannelMapping::Alpha };
            break;
        case ultralight::kBitmapFormat_A8_UNORM:
            texInfo.format = CoreGraphics::PixelFormat::R8;
            if (!bitmap->IsEmpty())
                texInfo.swizzle = { CoreGraphics::TextureChannelMapping::Red, CoreGraphics::TextureChannelMapping::Red, CoreGraphics::TextureChannelMapping::Red, CoreGraphics::TextureChannelMapping::Red };
            break; 
    }
    texInfo.type = CoreGraphics::Texture2D;
    texInfo.usage = bitmap->IsEmpty() ? CoreGraphics::RenderTexture : CoreGraphics::SampleTexture;

    // Create texture
    CoreGraphics::TextureId tex = CoreGraphics::CreateTexture(texInfo);

    TextureHandle storage;
    storage.tex = tex;
    storage.initial = true;
    this->textureMap.Add(texture_id, storage);

    // If bitmap is not empty, update texture
    if (!bitmap->IsEmpty())
    {
        UpdateTexture(texture_id, bitmap);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::UpdateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap)
{
    IndexT idx = this->textureMap.FindIndex(texture_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Texture ID %d not created\n", texture_id);
        return;
    }
    TextureHandle& storage = this->textureMap.ValueAtIndex(texture_id, idx);

    // Create buffer to upload to texture
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::TransferBufferSource;
    bufInfo.byteSize = bitmap->size();
    bufInfo.mode = CoreGraphics::HostLocal;
    bufInfo.data = bitmap->LockPixels();
    bufInfo.dataSize = bitmap->size();
    CoreGraphics::BufferId tempBuf = CoreGraphics::CreateBuffer(bufInfo);
    bitmap->UnlockPixels();

    // Lock setup command buffer for update
    CoreGraphics::CmdBufferId setupCmd = CoreGraphics::LockGraphicsSetupCommandBuffer();

    CoreGraphics::BufferCopy from;
    from.offset = 0;
    CoreGraphics::TextureCopy to;
    to.mip = 0;
    to.layer = 0;
    to.region.left = 0;
    to.region.top = 0;
    to.region.bottom = bitmap->height();
    to.region.right = bitmap->width();

    CoreGraphics::PipelineStage prevStage = storage.initial ? CoreGraphics::PipelineStage::ImageInitial : CoreGraphics::PipelineStage::AllShadersRead;
    storage.initial = false;

    // Perform copy
    CoreGraphics::CmdBarrier(setupCmd, prevStage, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
                        {
                            CoreGraphics::TextureBarrierInfo
                            {
                                storage.tex,
                                CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                            }
                        });
    CoreGraphics::CmdCopy(setupCmd, tempBuf, { from }, storage.tex, { to });
    CoreGraphics::CmdBarrier(setupCmd, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global,
                        {
                            CoreGraphics::TextureBarrierInfo
                            {
                                storage.tex,
                                CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                            }
                        });

    // Finish command buffer
    CoreGraphics::UnlockGraphicsSetupCommandBuffer();

    // Delete temporary buffer
    CoreGraphics::DestroyBuffer(tempBuf);
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::DestroyTexture(uint32_t texture_id)
{
    IndexT idx = this->textureMap.FindIndex(texture_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Texture ID %d not created\n", texture_id);
        return;
    }
    const TextureHandle& storage = this->textureMap.ValueAtIndex(texture_id, idx);
    CoreGraphics::DestroyTexture(storage.tex);
    this->textureMap.EraseIndex(texture_id, idx);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
UltralightRenderer::NextRenderBufferId()
{
    return this->renderBufferId++;
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::CreateRenderBuffer(uint32_t render_buffer_id, const ultralight::RenderBuffer& buffer)
{
    IndexT idx = this->renderBufferMap.FindIndex(render_buffer_id);
    if (idx != InvalidIndex)
    {
        n_printf("UltralightRenderer RenderBuffer ID %d already created\n", buffer.texture_id);
        return;
    }

    idx = this->textureMap.FindIndex(buffer.texture_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Texture ID %d not created\n", buffer.texture_id);
        return;
    }
    const TextureHandle& tex = this->textureMap.ValueAtIndex(buffer.texture_id, idx);

    CoreGraphics::TextureViewCreateInfo texViewInfo;
    texViewInfo.tex = tex.tex;
    texViewInfo.numMips = 1;
    texViewInfo.numLayers = 1;
    texViewInfo.startMip = 0;
    texViewInfo.startLayer = 0;
    texViewInfo.format = CoreGraphics::TextureGetPixelFormat(tex.tex);
    CoreGraphics::TextureViewId texView = CoreGraphics::CreateTextureView(texViewInfo);

    CoreGraphics::PassCreateInfo passInfo;
    passInfo.name = Util::String::Sprintf("Ultralight Pass %d", render_buffer_id);
    passInfo.attachments.Append(texView);
    passInfo.attachmentClears.Append(Math::vec4(0));
    passInfo.attachmentFlags.Append(CoreGraphics::AttachmentFlagBits::Clear | CoreGraphics::AttachmentFlagBits::Store);
    passInfo.attachmentDepthStencil.Append(false);
    CoreGraphics::Subpass subpass;
    subpass.attachments.Append(0);
    subpass.numScissors = 1;
    subpass.numViewports = 1;
    passInfo.subpasses.Append(subpass);

    CoreGraphics::PassId pass = CoreGraphics::CreatePass(passInfo);

    RenderBufferHandle storage;
    storage.pass = pass;
    storage.texView = texView;
    this->renderBufferMap.Add(render_buffer_id, storage);
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::DestroyRenderBuffer(uint32_t render_buffer_id)
{
    IndexT idx = this->renderBufferMap.FindIndex(render_buffer_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Render Buffer ID %d not created\n", render_buffer_id);
        return;
    }
    const RenderBufferHandle& storage = this->renderBufferMap.ValueAtIndex(render_buffer_id, idx);
    CoreGraphics::DestroyPass(storage.pass);
    this->renderBufferMap.EraseIndex(render_buffer_id, idx);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
UltralightRenderer::NextGeometryId()
{
    return this->geometryId++;
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::CreateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices)
{
    IndexT idx = this->geometryMap.FindIndex(geometry_id);
    if (idx != InvalidIndex)
    {
        n_printf("UltralightRenderer Geometry ID %d already exists\n", geometry_id);
        return;
    }

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.byteSize = vertices.size;
    vboInfo.mode = CoreGraphics::DeviceLocal;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = vertices.data;
    vboInfo.dataSize = vertices.size;
    CoreGraphics::BufferId vbo = CoreGraphics::CreateBuffer(vboInfo);

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.byteSize = indices.size;
    iboInfo.mode = CoreGraphics::DeviceLocal;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = indices.data;
    iboInfo.dataSize = indices.size;
    CoreGraphics::BufferId ibo = CoreGraphics::CreateBuffer(iboInfo);

    CoreGraphics::VertexLayoutId vlo;
    switch (vertices.format)
    {
        case ultralight::VertexBufferFormat::kVertexBufferFormat_2f_4ub_2f:
            vlo = ultralightState.vloSmall;
            break;
        case ultralight::VertexBufferFormat::kVertexBufferFormat_2f_4ub_2f_2f_28f:
            vlo = ultralightState.vloBig;
            break;
    }

    GeometryHandle storage;
    storage.vbo = vbo;
    storage.ibo = ibo;
    storage.vlo = vlo;

    this->geometryMap.Add(geometry_id, storage);
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::UpdateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices)
{
    IndexT idx = this->geometryMap.FindIndex(geometry_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Geometry ID %d has not been created\n", geometry_id);
        return;
    }
    const GeometryHandle& geo = this->geometryMap.ValueAtIndex(geometry_id, idx);

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.byteSize = vertices.size;
    vboInfo.mode = CoreGraphics::HostLocal;
    vboInfo.usageFlags = CoreGraphics::TransferBufferSource;
    vboInfo.data = vertices.data;
    vboInfo.dataSize = vertices.size;
    CoreGraphics::BufferId vbo = CoreGraphics::CreateBuffer(vboInfo);

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.byteSize = indices.size;
    iboInfo.mode = CoreGraphics::HostLocal;
    iboInfo.usageFlags = CoreGraphics::TransferBufferSource;
    iboInfo.data = indices.data;
    iboInfo.dataSize = indices.size;
    CoreGraphics::BufferId ibo = CoreGraphics::CreateBuffer(iboInfo);

    // Lock setup command buffer for update
    CoreGraphics::CmdBufferId setupCmd = CoreGraphics::LockGraphicsSetupCommandBuffer();

    CoreGraphics::BufferCopy from, to;
    from.offset = 0;
    to.offset = 0;

    // Perform copy
    CoreGraphics::CmdCopy(setupCmd, vbo, { from }, geo.vbo, { to }, vertices.size);
    CoreGraphics::CmdCopy(setupCmd, ibo, { from }, geo.ibo, { to }, indices.size);

    // Finish command buffer
    CoreGraphics::UnlockGraphicsSetupCommandBuffer();

    // Delete temporary buffer
    CoreGraphics::DestroyBuffer(vbo);
    CoreGraphics::DestroyBuffer(ibo);
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::DestroyGeometry(uint32_t geometry_id)
{
    IndexT idx = this->geometryMap.FindIndex(geometry_id);
    if (idx == InvalidIndex)
    {
        n_printf("UltralightRenderer Geometry ID %d has not been created\n", geometry_id);
        return;
    }
    const GeometryHandle& geo = this->geometryMap.ValueAtIndex(geometry_id, idx);
    CoreGraphics::DestroyBuffer(geo.vbo);
    CoreGraphics::DestroyBuffer(geo.ibo);
    this->geometryMap.EraseIndex(geometry_id, idx);
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::UpdateCommandList(const ultralight::CommandList& list)
{
    this->commands.SetSize(list.size);
    memcpy(this->commands.Begin(), list.commands, list.size * sizeof(ultralight::Command));
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::BeginSynchronize()
{
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::EndSynchronize()
{
}

//------------------------------------------------------------------------------
/**
*/
uint
GetTexture(UltralightRenderer* renderer, uint textureId)
{
    IndexT idx = renderer->textureMap.FindIndex(textureId);
    if (idx != InvalidIndex)
    {
        const UltralightRenderer::TextureHandle& tex = renderer->textureMap.ValueAtIndex(textureId, idx);
        return CoreGraphics::TextureGetBindlessHandle(tex.tex);
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::PreDraw(const ultralight::RenderTarget& view)
{
    // Update quad render state
    ultralightState.backbufferDrawState.texture1 = GetTexture(this, view.texture_id);
    ultralightState.backbufferDrawOffset = CoreGraphics::SetConstants(ultralightState.backbufferDrawState);

    for (const ultralight::Command& cmd : this->commands)
    {
        if (cmd.command_type == ultralight::kCommandType_DrawGeometry)
        {
            // Commit constants
            Staticui::PerDrawState uniforms;
            memcpy(uniforms.Clip, cmd.gpu_state.clip, sizeof(cmd.gpu_state.clip));
            uniforms.ClipSize = cmd.gpu_state.clip_size;
            memcpy(uniforms.Scalar4, cmd.gpu_state.uniform_scalar, sizeof(cmd.gpu_state.uniform_scalar));
            memcpy(uniforms.Vector, cmd.gpu_state.uniform_vector, sizeof(cmd.gpu_state.uniform_vector));

            ultralight::Matrix tempMat;
            tempMat.Set(cmd.gpu_state.transform);

            ultralight::Matrix modelProjection;
            modelProjection.SetOrthographicProjection(cmd.gpu_state.viewport_width, cmd.gpu_state.viewport_height, true);
            modelProjection.Transform(tempMat);
            memcpy(uniforms.Transform, modelProjection.GetMatrix4x4().data, sizeof(modelProjection.GetMatrix4x4().data));

            // Setup textures
            if (cmd.gpu_state.enable_texturing)
            {
                uniforms.texture1 = 0;
                uniforms.texture2 = 0;
                if (cmd.gpu_state.texture_1_id > 0)
                    uniforms.texture1 = GetTexture(this, cmd.gpu_state.texture_1_id);
                if (cmd.gpu_state.texture_2_id > 0)
                    uniforms.texture2 = GetTexture(this, cmd.gpu_state.texture_2_id);
            }

            uint offset = CoreGraphics::SetConstants(sizeof(Staticui::PerDrawState));
            this->constantOffsets.Append(offset);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::Render(const CoreGraphics::CmdBufferId& cmds, IndexT bufferIndex)
{
    if (this->commands.IsEmpty())
        return;

    CoreGraphics::CmdSetPrimitiveTopology(cmds, CoreGraphics::PrimitiveTopology::TriangleList);

    static CoreGraphics::PassId currentPass = CoreGraphics::InvalidPassId;
    static CoreGraphics::BufferId currentVbo = CoreGraphics::InvalidBufferId;
    static CoreGraphics::BufferId currentIbo = CoreGraphics::InvalidBufferId;
    static CoreGraphics::VertexLayoutId currentVlo = CoreGraphics::InvalidVertexLayoutId;
    uint shaderType = -1;
    uint constantOffsetCounter = 0;
    for (const ultralight::Command& cmd : this->commands)
    {
        IndexT idx = this->renderBufferMap.FindIndex(cmd.gpu_state.render_buffer_id);
        n_assert(idx != InvalidIndex);
        const RenderBufferHandle& renderBuffer = this->renderBufferMap.ValueAtIndex(cmd.gpu_state.render_buffer_id, idx);

        if (renderBuffer.pass != currentPass)
        {
            if (currentPass != CoreGraphics::InvalidPassId)
            {
                CoreGraphics::CmdEndPass(cmds);
            }

            currentVbo = CoreGraphics::InvalidBufferId;
            currentIbo = CoreGraphics::InvalidBufferId;
            currentVlo = CoreGraphics::InvalidVertexLayoutId;
            shaderType = -1;

            CoreGraphics::CmdBeginPass(cmds, renderBuffer.pass);
            currentPass = renderBuffer.pass;
        }

        if (cmd.command_type == ultralight::kCommandType_DrawGeometry)
        {
            Math::rectangle<SizeT> viewport;
            viewport.top = 0;
            viewport.left = 0;
            viewport.right = cmd.gpu_state.viewport_width;
            viewport.bottom = cmd.gpu_state.viewport_height;
            CoreGraphics::CmdSetViewport(cmds, viewport, 0);

            if (cmd.gpu_state.enable_scissor)
            {
                Math::rectangle<SizeT> scissor;
                scissor.top = cmd.gpu_state.scissor_rect.top;
                scissor.bottom = cmd.gpu_state.scissor_rect.bottom;
                scissor.left = cmd.gpu_state.scissor_rect.left;
                scissor.right = cmd.gpu_state.scissor_rect.right;
                CoreGraphics::CmdSetScissorRect(cmds, scissor, 0);
            }

            IndexT idx = this->geometryMap.FindIndex(cmd.geometry_id);
            n_assert(idx != InvalidIndex);
            const GeometryHandle& geo = this->geometryMap.ValueAtIndex(cmd.geometry_id, idx);

            if (shaderType != cmd.gpu_state.shader_type)
            {
                switch (cmd.gpu_state.shader_type)
                {
                    case ultralight::ShaderType::kShaderType_FillPath:
                        CoreGraphics::CmdSetShaderProgram(cmds, ultralightState.ultralightShader1);
                        break;
                    case ultralight::ShaderType::kShaderType_Fill:
                        CoreGraphics::CmdSetShaderProgram(cmds, ultralightState.ultralightShader2);
                        break;
                }
                shaderType = cmd.gpu_state.shader_type;
            }

            // Setup input assembly
            if (currentVbo != geo.vbo)
            {
                CoreGraphics::CmdSetVertexBuffer(cmds, 0, geo.vbo, 0);
                currentVbo = geo.vbo;
            }

            if (currentIbo != geo.ibo)
            {
                CoreGraphics::CmdSetIndexBuffer(cmds, geo.ibo, 0);
                currentIbo = geo.ibo;
            }

            if (geo.vlo != currentVlo)
            {
                CoreGraphics::CmdSetVertexLayout(cmds, geo.vlo);
                currentVlo = geo.vlo;
            }

            // Set resource table
            CoreGraphics::CmdSetResourceTable(cmds, ultralightState.resourceTable, NEBULA_DYNAMIC_OFFSET_GROUP, CoreGraphics::GraphicsPipeline, { this->constantOffsets[constantOffsetCounter++]});

            // Apply pipeline
            CoreGraphics::CmdSetGraphicsPipeline(cmds);

            CoreGraphics::PrimitiveGroup primGroup;
            primGroup.SetBaseIndex(cmd.indices_offset);
            primGroup.SetBaseVertex(0);
            primGroup.SetNumIndices(cmd.indices_count);
            primGroup.SetNumVertices(0);
            CoreGraphics::CmdDraw(cmds, primGroup);
        }
    }

    // End pass if we have one pending
    if (currentPass != CoreGraphics::InvalidPassId)
    {
        CoreGraphics::CmdEndPass(cmds);
    }

    currentPass = CoreGraphics::InvalidPassId;
    currentVbo = CoreGraphics::InvalidBufferId;
    currentIbo = CoreGraphics::InvalidBufferId;
    currentVlo = CoreGraphics::InvalidVertexLayoutId;
    shaderType = -1;

    this->commands.Clear();
    this->constantOffsets.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
UltralightRenderer::DrawToBackbuffer(const CoreGraphics::CmdBufferId& cmds, IndexT bufferIndex)
{
    CoreGraphics::CmdSetPrimitiveTopology(cmds, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetShaderProgram(cmds, ultralightState.ultralightShader3);
    CoreGraphics::CmdSetVertexLayout(cmds, ultralightState.vloEmpty);

    // Set resource table
    CoreGraphics::CmdSetResourceTable(cmds, ultralightState.resourceTable, NEBULA_DYNAMIC_OFFSET_GROUP, CoreGraphics::GraphicsPipeline, { ultralightState.backbufferDrawOffset });

    // Apply pipeline
    CoreGraphics::CmdSetGraphicsPipeline(cmds);

    CoreGraphics::PrimitiveGroup prim;
    prim.SetNumVertices(6);
    CoreGraphics::CmdDraw(cmds, prim);
}

} // namespace StaticUI
