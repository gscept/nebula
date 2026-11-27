//------------------------------------------------------------------------------
//  @file resourcespanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "graphicsfeature/editor/resourcespanel.h"

#if WITH_NEBULA_EDITOR
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "coregraphics/pixelformat.h"
#include "dynui/imguicontext.h"
#include "memory/memory.h"
#include "imgui.h"
#include <algorithm>

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::ResourcesPanel, 'RESP', Editor::EditorPanel);

namespace
{

//------------------------------------------------------------------------------
/**
    The bytes of one texel, or of one block for the compressed formats. The engine's helpers each
    leave some formats out and stop on them, so the panel keeps its own table
*/
void
FormatFootprint(CoreGraphics::PixelFormat::Code format, uint64_t& outBlock, uint64_t& outBytes)
{
    using namespace CoreGraphics;
    outBlock = 1;
    switch (format)
    {
    case PixelFormat::DXT1: case PixelFormat::DXT1A: case PixelFormat::DXT1sRGB: case PixelFormat::DXT1AsRGB: case PixelFormat::BC4:
        outBlock = 4; outBytes = 8; return;
    case PixelFormat::DXT3: case PixelFormat::DXT5: case PixelFormat::DXT3sRGB: case PixelFormat::DXT5sRGB:
    case PixelFormat::BC5: case PixelFormat::BC6H: case PixelFormat::BC6HU: case PixelFormat::BC7: case PixelFormat::BC7sRGB:
        outBlock = 4; outBytes = 16; return;
    case PixelFormat::R8: case PixelFormat::S8:
        outBytes = 1; return;
    case PixelFormat::R16F: case PixelFormat::R16: case PixelFormat::R5G6B5: case PixelFormat::R5G5B5A1: case PixelFormat::R4G4B4A4:
    case PixelFormat::B5G6R5: case PixelFormat::B5G6R5A1: case PixelFormat::B4G4R4A4:
        outBytes = 2; return;
    case PixelFormat::R8G8B8: case PixelFormat::B8G8R8: case PixelFormat::D16S8:
        outBytes = 3; return;
    case PixelFormat::D32S8:
        outBytes = 5; return;
    case PixelFormat::R32G32F: case PixelFormat::R32G32: case PixelFormat::R16G16B16A16F: case PixelFormat::R16G16B16A16:
        outBytes = 8; return;
    case PixelFormat::R32G32B32F: case PixelFormat::R32G32B32:
        outBytes = 12; return;
    case PixelFormat::R32G32B32A32F: case PixelFormat::R32G32B32A32:
        outBytes = 16; return;
    default:
        outBytes = 4; return;
    }
}

//------------------------------------------------------------------------------
/**
    What the texture takes on the GPU, every mip and layer, compressed formats by their blocks
*/
uint64_t
TextureBytes(const CoreGraphics::TextureId tex)
{
    const CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
    const CoreGraphics::PixelFormat::Code format = CoreGraphics::TextureGetPixelFormat(tex);
    uint64_t block = 1, texel = 4;
    FormatFootprint(format, block, texel);
    const SizeT mips = Math::max<SizeT>(1, CoreGraphics::TextureGetNumMips(tex));
    const SizeT layers = Math::max<SizeT>(1, CoreGraphics::TextureGetNumLayers(tex));
    const SizeT samples = Math::max<SizeT>(1, CoreGraphics::TextureGetNumSamples(tex));

    uint64_t bytes = 0;
    uint64_t w = Math::max<SizeT>(1, dims.width), h = Math::max<SizeT>(1, dims.height), d = Math::max<SizeT>(1, dims.depth);
    for (SizeT mip = 0; mip < mips; mip++)
    {
        bytes += ((w + block - 1) / block) * ((h + block - 1) / block) * d * texel;
        w = Math::max<uint64_t>(1, w / 2);
        h = Math::max<uint64_t>(1, h / 2);
        if (CoreGraphics::TextureGetType(tex) == CoreGraphics::Texture3D)
            d = Math::max<uint64_t>(1, d / 2);
    }
    return bytes * layers * samples;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FormatBytes(uint64_t bytes)
{
    if (bytes >= 1_GB)
        return Util::Format("%.2f GB", bytes / double(1_GB));
    if (bytes >= 1_MB)
        return Util::Format("%.2f MB", bytes / double(1_MB));
    if (bytes >= 1_KB)
        return Util::Format("%.2f KB", bytes / double(1_KB));
    return Util::Format("%llu B", (unsigned long long)bytes);
}

//------------------------------------------------------------------------------
/**
*/
const char*
TypeName(CoreGraphics::TextureType type)
{
    switch (type)
    {
    case CoreGraphics::Texture1D: return "1D";
    case CoreGraphics::Texture2D: return "2D";
    case CoreGraphics::Texture3D: return "3D";
    case CoreGraphics::TextureCube: return "Cube";
    case CoreGraphics::Texture1DArray: return "1D array";
    case CoreGraphics::Texture2DArray: return "2D array";
    case CoreGraphics::TextureCubeArray: return "Cube array";
    default: return "?";
    }
}

struct Row
{
    CoreGraphics::TextureId tex;
    Util::String name;
    CoreGraphics::TextureDimensions dims;
    CoreGraphics::PixelFormat::Code format;
    uint64_t bytes;
    SizeT mips, layers, samples;
    CoreGraphics::TextureType type;
};

//------------------------------------------------------------------------------
/**
    The renderer's list is copied first, a texture made or let go while the table draws would move it
*/
Util::Array<Row>
GatherRows(const char* filter)
{
    const Util::Array<CoreGraphics::TextureId> textures = CoreGraphics::TrackedTextures;
    Util::String lowerFilter(filter);
    lowerFilter.ToLower();

    Util::Array<Row> rows;
    rows.Reserve(textures.Size());
    for (const CoreGraphics::TextureId tex : textures)
    {
        Row row;
        row.tex = tex;
        row.name = CoreGraphics::TextureGetName(tex).Value();
        if (row.name.IsEmpty())
            row.name = Util::Format("(unnamed %u)", tex.id);
        if (!lowerFilter.IsEmpty())
        {
            Util::String lower = row.name;
            lower.ToLower();
            if (lower.FindStringIndex(lowerFilter) == InvalidIndex)
                continue;
        }
        row.dims = CoreGraphics::TextureGetDimensions(tex);
        row.format = CoreGraphics::TextureGetPixelFormat(tex);
        row.bytes = TextureBytes(tex);
        row.mips = CoreGraphics::TextureGetNumMips(tex);
        row.layers = CoreGraphics::TextureGetNumLayers(tex);
        row.samples = CoreGraphics::TextureGetNumSamples(tex);
        row.type = CoreGraphics::TextureGetType(tex);
        rows.Append(row);
    }
    return rows;
}

} // namespace

//------------------------------------------------------------------------------
/**
    The panel starts closed, the panels menu opens it
*/
ResourcesPanel::ResourcesPanel() :
    selected(CoreGraphics::InvalidTextureId),
    previewId(Ids::InvalidId32),
    mip(0),
    layer(0),
    red(true), green(true), blue(true), alpha(true),
    useAlpha(false),
    useRange(false),
    rangeMin(0.0f), rangeMax(1.0f), rangeLimit(1.0f),
    pointFilter(true),
    zoom(1.0f)
{
    this->name = "resources";
    this->open = false;
    this->filter[0] = '\0';
}

//------------------------------------------------------------------------------
/**
*/
ResourcesPanel::~ResourcesPanel()
{
    if (this->previewId != Ids::InvalidId32)
        Dynui::DeallocateImguiTextureId(this->previewId);
}

//------------------------------------------------------------------------------
/**
*/
bool
ResourcesPanel::OnSetup()
{
    this->core->GetCommands().Register("resources", "resources [list [filter]|total] - the GPU textures the renderer has made",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ResourcesPanel, &ResourcesPanel::CommandResources>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcesPanel::Draw()
{
    // a texture let go since the last frame is not shown any more
    if (this->selected != CoreGraphics::InvalidTextureId && CoreGraphics::TrackedTextures.FindIndex(this->selected) == InvalidIndex)
        this->selected = CoreGraphics::InvalidTextureId;

    if (ImGui::BeginTable("##resources", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.4f);
        ImGui::TableNextColumn();
        this->DrawList();
        ImGui::TableNextColumn();
        this->DrawPreview();
        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcesPanel::DrawList()
{
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##filter", "Filter by name", this->filter, sizeof(this->filter));

    Util::Array<Row> rows = GatherRows(this->filter);
    uint64_t total = 0;
    for (const Row& row : rows)
        total += row.bytes;
    ImGui::TextDisabled("%d textures, %s", rows.Size(), FormatBytes(total).AsCharPtr());

    const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
    if (!ImGui::BeginTable("##textures", 7, flags))
        return;

    enum Column { Name, Type, Dimensions, Format, Mips, Samples, Size };
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0.0f, Name);
    ImGui::TableSetupColumn("Type", 0, 0.0f, Type);
    ImGui::TableSetupColumn("Dimensions", 0, 0.0f, Dimensions);
    ImGui::TableSetupColumn("Format", 0, 0.0f, Format);
    ImGui::TableSetupColumn("Mips", 0, 0.0f, Mips);
    ImGui::TableSetupColumn("Samples", ImGuiTableColumnFlags_DefaultHide, 0.0f, Samples);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_PreferSortDescending, 0.0f, Size);
    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableHeadersRow();

    if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs != nullptr && specs->SpecsCount > 0)
    {
        const ImGuiTableColumnSortSpecs spec = specs->Specs[0];
        const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
        std::sort(rows.Begin(), rows.End(), [&spec, ascending](const Row& a, const Row& b)
        {
            int order = 0;
            switch (spec.ColumnUserID)
            {
            case Type: order = (int)a.type - (int)b.type; break;
            case Dimensions:
            {
                const int64_t pa = (int64_t)a.dims.width * a.dims.height * Math::max<SizeT>(1, a.dims.depth);
                const int64_t pb = (int64_t)b.dims.width * b.dims.height * Math::max<SizeT>(1, b.dims.depth);
                order = pa < pb ? -1 : (pa > pb ? 1 : 0);
                break;
            }
            case Format: order = (int)a.format - (int)b.format; break;
            case Mips: order = a.mips - b.mips; break;
            case Samples: order = a.samples - b.samples; break;
            case Size: order = a.bytes < b.bytes ? -1 : (a.bytes > b.bytes ? 1 : 0); break;
            default: order = Util::String::StrCmp(a.name.AsCharPtr(), b.name.AsCharPtr()); break;
            }
            if (order == 0)
                order = Util::String::StrCmp(a.name.AsCharPtr(), b.name.AsCharPtr());
            return ascending ? order < 0 : order > 0;
        });
    }

    ImGuiListClipper clipper;
    clipper.Begin(rows.Size());
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const Row& row = rows[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID((int)row.tex.id);
            if (ImGui::Selectable(row.name.AsCharPtr(), row.tex == this->selected, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (this->selected != row.tex)
                {
                    this->mip = 0;
                    this->layer = 0;
                    this->useRange = CoreGraphics::PixelFormat::IsDepthFormat(row.format) || CoreGraphics::PixelFormat::IsHighDynamicRange(row.format);
                }
                this->selected = row.tex;
            }
            ImGui::SetItemTooltip("%s", row.name.AsCharPtr());
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(TypeName(row.type));
            ImGui::TableNextColumn();
            if (row.dims.depth > 1)
                ImGui::Text("%dx%dx%d", row.dims.width, row.dims.height, row.dims.depth);
            else
                ImGui::Text("%dx%d", row.dims.width, row.dims.height);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(CoreGraphics::PixelFormat::ToString(row.format).AsCharPtr());
            ImGui::TableNextColumn();
            ImGui::Text("%d", row.mips);
            ImGui::TableNextColumn();
            ImGui::Text("%d", row.samples);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(FormatBytes(row.bytes).AsCharPtr());
        }
    }
    ImGui::EndTable();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcesPanel::DrawPreview()
{
    if (this->selected == CoreGraphics::InvalidTextureId)
    {
        ImGui::TextDisabled("Pick a texture to see it");
        return;
    }

    const CoreGraphics::TextureId tex = this->selected;
    const CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
    const CoreGraphics::PixelFormat::Code format = CoreGraphics::TextureGetPixelFormat(tex);
    const SizeT mips = Math::max<SizeT>(1, CoreGraphics::TextureGetNumMips(tex));
    const SizeT layers = Math::max<SizeT>(1, CoreGraphics::TextureGetNumLayers(tex));

    ImGui::TextWrapped("%s", CoreGraphics::TextureGetName(tex).Value());
    ImGui::TextDisabled("%s %dx%d, %s, %s", TypeName(CoreGraphics::TextureGetType(tex)), dims.width, dims.height,
        CoreGraphics::PixelFormat::ToString(format).AsCharPtr(), FormatBytes(TextureBytes(tex)).AsCharPtr());

    // a multisampled texture or a 3D one can't be shown by the 2D preview
    const bool previewable = CoreGraphics::TextureGetNumSamples(tex) <= 1 && CoreGraphics::TextureGetType(tex) != CoreGraphics::Texture3D;
    if (!previewable)
    {
        ImGui::TextDisabled("Multisampled and 3D textures can't be previewed");
        return;
    }

    if (ImGui::BeginChild("##options", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY))
    {
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderInt("Mip", &this->mip, 0, mips - 1);
        if (layers > 1)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt("Layer", &this->layer, 0, layers - 1);
        }
        ImGui::Checkbox("R", &this->red);
        ImGui::SameLine();
        ImGui::Checkbox("G", &this->green);
        ImGui::SameLine();
        ImGui::Checkbox("B", &this->blue);
        ImGui::SameLine();
        ImGui::Checkbox("A", &this->alpha);
        ImGui::SameLine();
        ImGui::Checkbox("Blend alpha", &this->useAlpha);
        ImGui::Checkbox("Point filter", &this->pointFilter);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("Zoom", &this->zoom, 0.1f, 8.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
        ImGui::Checkbox("Range", &this->useRange);
        ImGui::SetItemTooltip("Map the values between min and max to black and white, for depth and data textures");
        if (this->useRange)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputFloat("Limit", &this->rangeLimit);
            this->rangeLimit = Math::max(this->rangeLimit, 0.001f);
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloatRange2("##range", &this->rangeMin, &this->rangeMax, this->rangeLimit * 0.001f, 0.0f, this->rangeLimit, "Min %.3f", "Max %.3f");
        }
    }
    ImGui::EndChild();

    this->mip = Math::clamp(this->mip, 0, mips - 1);
    this->layer = Math::clamp(this->layer, 0, layers - 1);

    Dynui::ImguiTextureId info;
    info.nebulaHandle = tex;
    info.mip = this->mip;
    info.layer = this->layer;
    info.useAlpha = this->useAlpha;
    info.useRange = this->useRange;
    info.rangeMin = this->rangeMin;
    info.rangeMax = this->rangeMax;
    info.red = this->red;
    info.green = this->green;
    info.blue = this->blue;
    info.alpha = this->alpha;
    info.pointFilter = this->pointFilter;
    info.tonemap = CoreGraphics::PixelFormat::IsHighDynamicRange(format) && !this->useRange;
    if (this->previewId == Ids::InvalidId32)
        this->previewId = Dynui::AllocateImguiTextureId(info);
    else
        Dynui::SetImguiTextureIdData(this->previewId, info);

    if (ImGui::BeginChild("##image", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const float width = ImGui::GetContentRegionAvail().x * this->zoom;
        const float ratio = dims.width > 0 ? dims.height / float(dims.width) : 1.0f;
        ImGui::Image(ImTextureRef(this->previewId), ImVec2(width, width * ratio));
    }
    ImGui::EndChild();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourcesPanel::CommandResources(const Util::Array<Util::String>& args)
{
    const bool list = !args.IsEmpty() && args[0] == "list";
    if (!args.IsEmpty() && !list && args[0] != "total")
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "resources [list [filter]|total]");
        return;
    }

    const Util::Array<Row> rows = GatherRows(list && args.Size() > 1 ? args[1].AsCharPtr() : "");
    uint64_t total = 0;
    for (const Row& row : rows)
    {
        total += row.bytes;
        if (list)
            this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("  %s %s %dx%d %s mips %d, %s", row.name.AsCharPtr(), TypeName(row.type),
                row.dims.width, row.dims.height, CoreGraphics::PixelFormat::ToString(row.format).AsCharPtr(), row.mips, FormatBytes(row.bytes).AsCharPtr()));
    }
    this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("resources: %d textures, %s", rows.Size(), FormatBytes(total).AsCharPtr()));
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
