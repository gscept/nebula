// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include <math.h>
#include "tb_renderer.h"
#include "platform/tb_system_interface.h"
#include "tb_tempbuffer.h"
#include "tbuifontrenderer.h"

namespace TBUI
{
//#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate
#include "stb_truetype.h"

TBUISTBFontRenderer::TBUISTBFontRenderer()
    : render_data(nullptr)
{
}

TBUISTBFontRenderer::~TBUISTBFontRenderer()
{
    stbtt_FreeBitmap(render_data, &font);
}

tb::TBFontMetrics
TBUISTBFontRenderer::GetMetrics()
{
    tb::TBFontMetrics metrics;
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    metrics.ascent = (int)(ascent * scale + 0.5f);
    metrics.descent = (int)((-descent) * scale + 0.5f);
    metrics.height = (int)((ascent - descent + lineGap) * scale + 0.5f);
    return metrics;
}

bool
TBUISTBFontRenderer::RenderGlyph(tb::TBFontGlyphData* data, UCS4 cp)
{
    stbtt_FreeBitmap(render_data, &font);
    render_data = stbtt_GetCodepointBitmap(&font, 0, scale, cp, &data->w, &data->h, 0, 0);
    data->data8 = render_data;
    data->stride = data->w;
    data->rgb = false;
    return data->data8 ? true : false;
}

void
TBUISTBFontRenderer::GetGlyphMetrics(tb::TBGlyphMetrics* metrics, UCS4 cp)
{
    int advance, leftSideBearing;
    const int gi = stbtt_FindGlyphIndex(&font, cp);
    stbtt_GetGlyphHMetrics(&font, gi, &advance, &leftSideBearing);
    metrics->advance = (int)roundf(advance * scale);

    int ix0, iy0;
    stbtt_GetGlyphBitmapBoxSubpixel(&font, gi, scale, scale, 0.f, 0.f, &ix0, &iy0, 0, 0);
    metrics->x = ix0;
    metrics->y = iy0;
}

int
TBUISTBFontRenderer::GetAdvance(UCS4 cp1, UCS4 cp2)
{
    int advance, leftSideBearing;
    const int gi1 = stbtt_FindGlyphIndex(&font, cp1);
    stbtt_GetGlyphHMetrics(&font, gi1, &advance, &leftSideBearing);
    if (font.kern)
        advance += stbtt_GetGlyphKernAdvance(&font, gi1, stbtt_FindGlyphIndex(&font, cp2));
    return (int)roundf(advance * scale);
}

bool
TBUISTBFontRenderer::Load(const char* filename, int size)
{
    if (!ttf_buffer.AppendFile(filename))
        return false;

    const unsigned char* ttf_ptr = (const unsigned char*)ttf_buffer.GetData();
    stbtt_InitFont(&font, ttf_ptr, stbtt_GetFontOffsetForIndex(ttf_ptr, 0));

    scale = stbtt_ScaleForPixelHeight(&font, (float)size);
    return true;
}

tb::TBFontFace*
TBUISTBFontRenderer::Create(tb::TBFontManager* font_manager, const char* filename, const tb::TBFontDescription& font_desc)
{
    if (TBUISTBFontRenderer* fr = new TBUISTBFontRenderer())
    {
        if (fr->Load(filename, (int)font_desc.GetSize()))
            if (tb::TBFontFace* font = new tb::TBFontFace(font_manager->GetGlyphCache(), fr, font_desc))
                return font;
        delete fr;
    }
    return nullptr;
}

} // namespace TBUI
