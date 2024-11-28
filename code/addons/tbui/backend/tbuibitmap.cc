#include "tbuibitmap.h"
#include "tbuirenderer.h"

namespace TBUI
{
TBUIBitmap::TBUIBitmap(TBUIRenderer* renderer)
    : renderer(renderer),
      texture(CoreGraphics::InvalidTextureId)
{
}

TBUIBitmap::~TBUIBitmap()
{
    renderer->FlushBitmap(this);
    CoreGraphics::DestroyTexture(texture);
    texture = CoreGraphics::InvalidTextureId;
}

bool
TBUIBitmap::Init(int width, int height, unsigned int* data)
{
    this->width = width;
    this->height = height;

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "tbui_generated_texture"_atm;
    texInfo.usage = CoreGraphics::TextureUsage::SampleTexture;
    texInfo.tag = "tbui"_atm;
    texInfo.data = data;
    texInfo.dataSize = width * height * CoreGraphics::PixelFormat::ToSize(CoreGraphics::PixelFormat::SRGBA8);
    texInfo.type = CoreGraphics::TextureType::Texture2D;
    texInfo.format = CoreGraphics::PixelFormat::SRGBA8;
    texInfo.width = width;
    texInfo.height = height;

    texture = CoreGraphics::CreateTexture(texInfo);

    //SetData(data);
    return true;
}

int
TBUIBitmap::Width()
{
    return width;
}

int
TBUIBitmap::Height()
{
    return height;
}

void
TBUIBitmap::SetData(unsigned int* data)
{
    SizeT dataSize = width * height * CoreGraphics::PixelFormat::ToSize(CoreGraphics::PixelFormat::SRGBA8);
    CoreGraphics::TextureUpdate(renderer->GetCmdBufferId(), texture, width, height, 0, 0, data, dataSize);
}
} // namespace TBUI
