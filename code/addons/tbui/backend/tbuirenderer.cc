#include "coregraphics/texture.h"
#include "tbuirenderer.h"
#include "tbuibitmap.h"

namespace TBUI
{
tb::TBBitmap*
TBUIRenderer::CreateBitmap(int width, int height, uint32* data)
{
    TBUIBitmap* bitmap = new TBUIBitmap(this);
    if (!bitmap->Init(width, height, data))
    {
        delete bitmap;
        return nullptr;
    }

    return bitmap;
}

void
TBUIRenderer::RenderBatch(Batch* tbBatch)
{
    CoreGraphics::TextureId texture = CoreGraphics::InvalidTextureId;
    TBUIBitmap* bitmap = (TBUIBitmap*)tbBatch->bitmap;
    if (bitmap)
        texture = bitmap->GetTexture();

    TBUIBatch batch(texture);
    batch.clipRect = clipRect;

    for (int i = 0; i < tbBatch->vertex_count; i++)
    {
        tb::TBRendererBatcher::Vertex tbVertex = tbBatch->vertex[i];
        TBUIVertex vertex = {
            .position = {tbVertex.x, tbVertex.y},
            .uv = {tbVertex.u, tbVertex.v},
            .color = {.v = tbVertex.col},
        };

        batch.vertices.Append(vertex);
    }

    batches.Append(batch);
}

void
TBUIRenderer::SetClipRect(const tb::TBRect& rect)
{
    clipRect = Math::intRectangle(rect.x, rect.y, rect.w, rect.h);
}

void
TBUIRenderer::SetCmdBufferId(const CoreGraphics::CmdBufferId& cmdBufferId)
{
    this->cmdBufferId = cmdBufferId;
}

const CoreGraphics::CmdBufferId&
TBUIRenderer::GetCmdBufferId() const
{
    return cmdBufferId;
}

void
TBUIRenderer::BeginBatch()
{
    batches.Clear();
}

const Util::Array<TBUIBatch>&
TBUIRenderer::EndBatch()
{
    return batches;
}
} // namespace TBUI
