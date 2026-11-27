#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::ResourcesPanel
    The GPU textures the renderer has made, loaded ones and render targets alike, with their size,
    format and memory, and a preview of the one picked: a mip, a layer, the channels and a value
    range for textures that hold data rather than color.

    What it lists is what the renderer tracks in an editor build, it doesn't load anything.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "neweditor/core/editorpanel.h"
#include "coregraphics/texture.h"
#include "ids/id.h"

namespace GraphicsFeature
{

class ResourcesPanel final : public Editor::EditorPanel
{
    __DeclareClass(ResourcesPanel)
public:
    ResourcesPanel();
    virtual ~ResourcesPanel();

    void Draw() override;

protected:
    bool OnSetup() override;

private:
    /// the table of textures
    void DrawList();
    /// the picked texture and how it is shown
    void DrawPreview();

    /// resources [list [filter]|total] - print the tracked textures or what they take
    void CommandResources(const Util::Array<Util::String>& args);

    char filter[128];
    CoreGraphics::TextureId selected;
    Ids::Id32 previewId;

    int mip;
    int layer;
    bool red, green, blue, alpha;
    bool useAlpha;
    bool useRange;
    float rangeMin, rangeMax, rangeLimit;
    bool pointFilter;
    float zoom;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
