#pragma once

#include "coregraphics/texture.h"

#undef PostMessage
#include "tb_renderer.h"

namespace TBUI
{

class TBUIRenderer;

class TBUIBitmap : public tb::TBBitmap
{
public:
  TBUIBitmap(TBUIRenderer* renderer);

  /** Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
to make sure any active batch is being flushed before the bitmap is deleted. */
  ~TBUIBitmap() override;

  bool Init(int width, int height, unsigned int* data);

  int Width() override;
  int Height() override;

  /** Update the bitmap with the given data (in BGRA32 format).
    Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
    to make sure any active batch is being flushed before the bitmap is changed. */
  void SetData(unsigned int* data) override;

  inline CoreGraphics::TextureId GetTexture() const
  {
    return texture;
  }

private:
  TBUIRenderer* renderer;
  int width = 0;
  int height = 0;
  CoreGraphics::TextureId texture;
};
} // namespace TBUI
