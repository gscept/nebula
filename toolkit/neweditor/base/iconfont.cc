//------------------------------------------------------------------------------
//  @file iconfont.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/base/iconfont.h"
#include "neweditor/base/iconregistry.h"
#include "io/uri.h"
#include "imgui.h"

namespace Editor
{

// The font the table was built from. Lucide, ISC licensed, next to the license in the engine's font
// folder
static const char* IconFontPath = "tool:syswork/data/fonts/lucide.ttf";

//------------------------------------------------------------------------------
/**
*/
void
LoadIconFont()
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig config;
    config.MergeMode = true;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.GlyphRanges = IconGlyphRanges;

    // MergeMode targets the last source added, which is not the text face here. The first font in the
    // atlas is the one ImGui draws with. A merged source with no size takes the size of the first
    // source in that font, so a glyph sits inline with a label
    config.DstFont = io.Fonts->Fonts[0];

    if (io.Fonts->AddFontFromFileTTF(IO::URI(IconFontPath).LocalPath().AsCharPtr(), 0.0f, &config) == nullptr)
        n_warning("Could not load the ImGui icon font '%s'\n", IconFontPath);
}

} // namespace Editor
