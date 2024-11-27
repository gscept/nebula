#pragma once

#include "Math/rectangle.h"
#include "tb/renderers/tb_renderer_batcher.h"
#include "tbuibatch.h"
#include "tbui/tbuiview.h"

namespace TBUI
{
class TBUIRenderer : public tb::TBRendererBatcher
{
public:
  tb::TBBitmap* CreateBitmap(int width, int height, uint32* data) override;
  void RenderBatch(Batch* batch) override;
  void SetClipRect(const tb::TBRect& rect) override;

public:
  void SetCmdBufferId(const CoreGraphics::CmdBufferId& cmdBufferId);
  const CoreGraphics::CmdBufferId& GetCmdBufferId() const;

  Util::Array<TBUIBatch> RenderView(TBUIView* view, int32_t width, int32_t height);

private:
  Math::intRectangle clipRect;
  Util::Array<TBUIBatch> batches;

  CoreGraphics::CmdBufferId cmdBufferId;
};
} // namespace TBUI
