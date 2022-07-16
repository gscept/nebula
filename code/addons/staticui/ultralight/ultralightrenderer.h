#pragma once
//------------------------------------------------------------------------------
/**
    UltralightRenderer

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/hashtable.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/pass.h"
#include "Ultralight/Ultralight.h"
#include "AppCore/Platform.h"
namespace StaticUI
{

class UltralightRenderer : public ultralight::GPUDriver
{
public:

    /// Constructor
    UltralightRenderer();
    /// Destructor
    ~UltralightRenderer();

    /// Get next texture
    uint32_t NextTextureId();
    /// Create texture
    void CreateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap) override;
    /// Update texture
    void UpdateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap) override;
    /// Destroy texture
    void DestroyTexture(uint32_t texture_id) override;

    /// Get next render buffer id
    uint32_t NextRenderBufferId();
    /// Create render target
    void CreateRenderBuffer(uint32_t render_buffer_id, const ultralight::RenderBuffer& buffer);
    /// Destroy render target
    void DestroyRenderBuffer(uint32_t render_buffer_id);

    /// Get next geometry
    uint32_t NextGeometryId();
    /// Create geometry
    void CreateGeometry(uint32_t geometry_id,
                                const ultralight::VertexBuffer& vertices,
                                const ultralight::IndexBuffer& indices);
    /// Update geometry
    void UpdateGeometry(uint32_t geometry_id,
                                const ultralight::VertexBuffer& vertices,
                                const ultralight::IndexBuffer& indices);

    /// Destroy geometry
    void DestroyGeometry(uint32_t geometry_id);

    /// Fill list of draw commands
    void UpdateCommandList(const ultralight::CommandList& list);

    /// Begin synchronization
    void BeginSynchronize() override;
    /// End synchronization
    void EndSynchronize() override;

    /// Run through command lists and setup constants
    void PreDraw(const ultralight::RenderTarget& view);
    /// Execute command lists
    void Render(const CoreGraphics::CmdBufferId& cmds, IndexT bufferIndex);

    /// Render to screen
    void DrawToBackbuffer(const CoreGraphics::CmdBufferId& cmds, IndexT bufferIndex);

private:
    friend uint GetTexture(UltralightRenderer* renderer, uint textureId);
    struct TextureHandle
    {
        CoreGraphics::TextureId tex;
        bool initial;
    };
    Util::HashTable<uint, TextureHandle> textureMap;

    struct RenderBufferHandle
    {
        CoreGraphics::PassId pass;

        CoreGraphics::TextureViewId texView;
    };
    Util::HashTable<uint, RenderBufferHandle> renderBufferMap;

    struct GeometryHandle
    {
        CoreGraphics::BufferId vbo;
        CoreGraphics::BufferId ibo;
        CoreGraphics::VertexLayoutId vlo;
    };
    Util::HashTable<uint, GeometryHandle> geometryMap;
    uint textureId, renderBufferId, geometryId;

    Util::Array<ultralight::Command> commands;
    Util::Array<uint> constantOffsets;
};


} // namespace StaticUI
