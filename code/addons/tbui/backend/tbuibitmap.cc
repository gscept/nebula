//------------------------------------------------------------------------------
//  backend/tbuibitmap.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "tbuibitmap.h"
#include "tbuirenderer.h"


namespace TBUI
{

//------------------------------------------------------------------------------
/*
*/
TBUIBitmap::TBUIBitmap(TBUIRenderer* renderer)
    : renderer(renderer),
      texture(CoreGraphics::InvalidTextureId)
{
}

//------------------------------------------------------------------------------
/*
*/
TBUIBitmap::~TBUIBitmap()
{
    this->renderer->FlushBitmap(this);
    CoreGraphics::DestroyTexture(this->texture);
    this->texture = CoreGraphics::InvalidTextureId;
}

//------------------------------------------------------------------------------
/*
*/
bool
TBUIBitmap::Init(int width, int height, unsigned int* data)
{
    this->width = width;
    this->height = height;

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "tbui_generated_texture"_atm;
    texInfo.usage = CoreGraphics::TextureUsage::Sample;
    texInfo.tag = "tbui"_atm;
    texInfo.data = data;
    texInfo.dataSize = width * height * CoreGraphics::PixelFormat::ToSize(CoreGraphics::PixelFormat::SRGBA8);
    texInfo.type = CoreGraphics::TextureType::Texture2D;
    texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
    texInfo.width = width;
    texInfo.height = height;

    this->texture = CoreGraphics::CreateTexture(texInfo);

    //SetData(data);
    return true;
}

//------------------------------------------------------------------------------
/*
*/
int
TBUIBitmap::Width()
{
    return this->width;
}

//------------------------------------------------------------------------------
/*
*/
int
TBUIBitmap::Height()
{
    return this->height;
}

//------------------------------------------------------------------------------
/*
*/
void
TBUIBitmap::SetData(unsigned int* data)
{
    const SizeT dataSize = this->width * this->height * CoreGraphics::PixelFormat::ToSize(CoreGraphics::PixelFormat::SRGBA8);
    CoreGraphics::TextureUpdate(this->renderer->GetCmdBufferId(), this->texture, this->width, this->height, 0, 0, data, dataSize);
}
} // namespace TBUI
