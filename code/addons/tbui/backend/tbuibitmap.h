#pragma once
//------------------------------------------------------------------------------
/**
    Turbobadger UI Bitmap

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/texture.h"
#include "tb_renderer.h"

//------------------------------------------------------------------------------
namespace TBUI
{

class TBUIRenderer;

//------------------------------------------------------------------------------
class TBUIBitmap : public tb::TBBitmap
{
public:

    ///
    TBUIBitmap(TBUIRenderer* renderer);

    /** Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
        to make sure any active batch is being flushed before the bitmap is deleted. */
    ~TBUIBitmap() override;

    ///
    bool Init(int width, int height, unsigned int* data);

    ///
    int Width() override;
    ///
    int Height() override;

    /** Update the bitmap with the given data (in BGRA32 format).
        Note: Implementations for batched renderers should call TBRenderer::FlushBitmap
        to make sure any active batch is being flushed before the bitmap is changed. */
    void SetData(unsigned int* data) override;

    ///
    CoreGraphics::TextureId GetTexture() const;
  
private:
    TBUIRenderer* renderer;
    int width = 0;
    int height = 0;
    CoreGraphics::TextureId texture;
};

//------------------------------------------------------------------------------
/*
*/
inline CoreGraphics::TextureId 
TBUIBitmap::GetTexture() const
{
    return this->texture;
}
} // namespace TBUI
