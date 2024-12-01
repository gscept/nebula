//------------------------------------------------------------------------------
//  backend/tbuirenderer.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/texture.h"
#include "tbuirenderer.h"
#include "tbuibitmap.h"

namespace TBUI
{

//------------------------------------------------------------------------------
/**
*/
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

//------------------------------------------------------------------------------
/**
*/
void
TBUIRenderer::RenderBatch(Batch* tbBatch)
{
    CoreGraphics::TextureId texture = CoreGraphics::InvalidTextureId;
    TBUIBitmap* bitmap = (TBUIBitmap*)tbBatch->bitmap;
    if (bitmap)
        texture = bitmap->GetTexture();

    TBUIBatch batch(texture);
    batch.clipRect = this->clipRect;

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

    this->batches.Append(batch);
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIRenderer::SetClipRect(const tb::TBRect& rect)
{
    this->clipRect = Math::intRectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIRenderer::SetCmdBufferId(const CoreGraphics::CmdBufferId& cmdBufferId)
{
    this->cmdBufferId = cmdBufferId;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::CmdBufferId&
TBUIRenderer::GetCmdBufferId() const
{
    return this->cmdBufferId;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<TBUIBatch>
TBUIRenderer::RenderView(TBUIView* view, int32_t width, int32_t height)
{
    this->batches.Clear();

    this->clipRect = {0, 0, (int32_t)width, (int32_t)height};

    BeginPaint(width, height);
    view->InvokePaint(tb::TBWidget::PaintProps());
    EndPaint();

    return this->batches;
}
} // namespace TBUI
