//------------------------------------------------------------------------------
//  @file particleasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "particleasseteditor.h"
#include "editor/commandmanager.h"
#include "particles/emitterattrs.h"
#include "particles/particleresource.h"
#include "pjson/pjson.h"
#include "io/filestream.h"
#include "io/jsonwriter.h"
#include "resources/resourceserver.h"
#include "coregraphics/textureloader.h"
#include "tinyfiledialogs.h"
#include "editor/tools/pathconverter.h"

#include "dynui/imguicontext.h"

namespace IO
{

//------------------------------------------------------------------------------
/**
*/
template <>
void
JsonWriter::Add(const Particles::EnvelopeCurve& value, const Util::String& name)
{
    auto& alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    const float* values = value.GetValues();
    const float* limits = value.GetLimits();
    val.add_value(values[0], alloc);
    val.add_value(values[1], alloc);
    val.add_value(values[2], alloc);
    val.add_value(values[3], alloc);
    val.add_value(limits[0], alloc);
    val.add_value(limits[1], alloc);
    val.add_value(value.GetKeyPos0(), alloc);
    val.add_value(value.GetKeyPos1(), alloc);
    val.add_value(value.GetFrequency(), alloc);
    val.add_value(value.GetAmplitude(), alloc);
    val.add_value(value.GetModFunc(), alloc);

    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

} // namespace IO

namespace Presentation
{

struct ParticleAssetItemData
{
    struct ParticleAsset
    {
        Util::StringAtom name;
        Particles::EmitterAttrs* attrs;
        Resources::ResourceName mesh = "";
        ImageHolder albedo, material, normals;
        Math::mat4 transform;
    };
    Util::Array<ParticleAsset> emitters;
};

//------------------------------------------------------------------------------
/**
*/
struct CMDParticleSetTexture : public Edit::Command
{
    ~CMDParticleSetTexture() {};
    const char* Name() override
    {
        return "Particle Set Texture";
    };

    bool Execute() override
    {
        n_assert(this->item->assetType == AssetEditor::AssetType::Particle);
        assetEditor->Edit();
        item->grabFocus = true;
        item->editCounter++;
        this->res = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, true, false);
        auto itemData = (const ParticleAssetItemData*)item->data;

        /// Todo, update material

        return true;
    };

    bool Unexecute() override
    {
        Resources::DiscardResource(this->res);
        assetEditor->Unedit();
        item->grabFocus = true;
        item->editCounter--;
        this->res = Resources::CreateResource(this->oldAsset, "editor", nullptr, nullptr, true, false);
        auto itemData = (const ParticleAssetItemData*)item->data;

        /// Todo, update material

        return true;
    };
    AssetEditorItem* item;
    AssetEditor* assetEditor;
    uint hash;
    uint index;
    uint bindlessOffset;
    Resources::ResourceName asset;

    Resources::ResourceName oldAsset;

private:
    Resources::ResourceId res;
};


//------------------------------------------------------------------------------
/**
*/
void
ParticleSerialize(const Ptr<IO::Stream>& stream, const Util::Array<ParticleAssetItemData::ParticleAsset>& emitters)
{
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    writer->Open();
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();

    writer->BeginArray("emitters");

    for (SizeT i = 0; i < emitters.Size(); i++)
    {
        writer->BeginObject("");

        writer->Add(emitters[i].name, "name");
        writer->Add(emitters[i].mesh.Value(), "mesh");
        writer->Add(texLoader->GetName(emitters[i].albedo.res), "albedo");
        writer->Add(texLoader->GetName(emitters[i].material.res), "material");
        writer->Add(texLoader->GetName(emitters[i].normals.res), "normals");
        writer->Add(emitters[i].transform, "transform");

        writer->BeginObject("floats");
        for (uint j = 0; j < Particles::EmitterAttrs::FloatAttr::NumFloatAttrs; j++)
        {
            writer->Add(emitters[i].attrs->GetFloat((Particles::EmitterAttrs::FloatAttr)j), Particles::FloatAttrNames[j]);
        }
        writer->End();

        writer->BeginObject("bools");
        for (uint j = 0; j < Particles::EmitterAttrs::BoolAttr::NumBoolAttrs; j++)
        {
            writer->Add(emitters[i].attrs->GetBool((Particles::EmitterAttrs::BoolAttr)j), Particles::BoolAttrNames[j]);
        }
        writer->End();


        writer->BeginObject("ints");
        for (uint j = 0; j < Particles::EmitterAttrs::IntAttr::NumIntAttrs; j++)
        {
            writer->Add(emitters[i].attrs->GetInt((Particles::EmitterAttrs::IntAttr)j), Particles::IntAttrNames[j]);
        }
        writer->End();

        writer->BeginObject("vecs");
        for (uint j = 0; j < Particles::EmitterAttrs::Float4Attr::NumFloat4Attrs; j++)
        {
            writer->Add(emitters[i].attrs->GetVec4((Particles::EmitterAttrs::Float4Attr)j), Particles::Float4AttrNames[j]);
        }
        writer->End();


        writer->BeginObject("curves");
        for (uint j = 0; j < Particles::EmitterAttrs::EnvelopeAttr::NumEnvelopeAttrs; j++)
        {
            Particles::EnvelopeCurve curve = emitters[i].attrs->GetEnvelope((Particles::EmitterAttrs::EnvelopeAttr)j);
            writer->Add(curve, Particles::EnvelopeAttrNames[j]);
        }
        writer->End();

        // Close object
        writer->End();
    }

    // Close array
    writer->End();

    writer->Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawCurve(ImDrawList* draw_list, Particles::EnvelopeCurve& curve)
{
    bool changed = false;

    ImVec2 region = ImGui::GetWindowSize();
    region.x = ImGui::GetContentRegionAvail().x;

    float itemWidth = region.x / 4.0f;
    const float* limits = curve.GetLimits();
    float modifyLimits[2];
    memcpy(modifyLimits, limits, sizeof(modifyLimits));
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + 0x123);
    if (ImGui::DragFloat("Min", &modifyLimits[0], 0.5f, 0.0f, 10000.0f))
    {
        curve.SetLimits(modifyLimits[0], modifyLimits[1]);
        changed = true;
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + 0x456);
    if (ImGui::DragFloat("Max", &modifyLimits[1], 0.5f, 0.0f, 10000.0f))
    {
        curve.SetLimits(modifyLimits[0], modifyLimits[1]);
        changed = true;
    }
    ImGui::PopID();

    const float* values = curve.GetValues();
    float keyPos0 = curve.GetKeyPos0();
    float keyPos1 = curve.GetKeyPos1();
    float modifyValues[4];
    memcpy(modifyValues, values, sizeof(modifyValues));
    ImVec2 points[4];
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    static const int MIN_CURVE_WINDOW_HEIGHT = 200;
    static const int MIN_CURVE_WINDOW_WIDTH = 100;
    static const int INNER_WINDOW_PADDING = 5;
    static const float HANDLE_SIZE = 5.0f;
    static const float HALF_HANDLE_SIZE = HANDLE_SIZE * 0.5f;
    ImVec2 narrowCursor = cursorPos + ImVec2(INNER_WINDOW_PADDING, INNER_WINDOW_PADDING);
    region.y = Math::min(region.y, MIN_CURVE_WINDOW_HEIGHT);
    ImVec2 narrowRegion = region - ImVec2(INNER_WINDOW_PADDING, INNER_WINDOW_PADDING) * 2;
    ImGuiIO io = ImGui::GetIO();

    static ImColor Colors[] =
    {
        IM_COL32(100, 100, 100, 255), // key point 0 fill
        IM_COL32(128, 128, 128, 255), // key point 0 outline
        IM_COL32(255, 79, 9, 255),    // control point 0
        IM_COL32(255, 79, 9, 255),    // control point 1
        IM_COL32(100, 100, 100, 255), // key point 1 fill
        IM_COL32(128, 128, 128, 255), // key point 1 outline
    };
    
    ImVec2 window = ImVec2(Math::min(narrowRegion.x, MIN_CURVE_WINDOW_WIDTH), Math::min(narrowRegion.y, MIN_CURVE_WINDOW_HEIGHT));
    points[0] = narrowCursor + ImVec2(0.0f, values[0] * window.y);
    points[1] = narrowCursor + ImVec2(narrowRegion.x * keyPos0, values[1] * window.y);
    points[2] = narrowCursor + ImVec2(narrowRegion.x * keyPos1, values[2] * window.y);
    points[3] = narrowCursor + ImVec2(narrowRegion.x * 1.0f, values[3] * window.y);
    draw_list->AddRectFilled(cursorPos, cursorPos + ImVec2(region.x, MIN_CURVE_WINDOW_HEIGHT), IM_COL32(50, 50, 50, 255), 8.0f);
    draw_list->AddRect(narrowCursor, narrowCursor + ImVec2(narrowRegion.x, MIN_CURVE_WINDOW_HEIGHT - INNER_WINDOW_PADDING*2), IM_COL32(100, 100, 100, 255), 2.0f);
    draw_list->AddBezierCubic(points[0], points[1], points[2], points[3], IM_COL32(100, 100, 100, 255), 2.0f, 64);
    draw_list->AddLine(points[0], points[1], IM_COL32(128, 128, 128, 255));
    draw_list->AddLine(points[2], points[3], IM_COL32(128, 128, 128, 255));
    draw_list->AddCircleFilled(points[0], HANDLE_SIZE,          Colors[0]);
    draw_list->AddCircleFilled(points[0], HANDLE_SIZE * 0.75,   Colors[1]);
    draw_list->AddCircleFilled(points[1], HANDLE_SIZE,          Colors[2]);
    draw_list->AddCircleFilled(points[2], HANDLE_SIZE,          Colors[3]);
    draw_list->AddCircleFilled(points[3], HANDLE_SIZE,          Colors[4]);
    draw_list->AddCircleFilled(points[3], HANDLE_SIZE * 0.75,   Colors[5]);
    
    const char* pointNames[4] = { "P0", "P1", "P2", "P3" };
    for (SizeT i = 0; i < 4; i++)
    {
        ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + i);

        ImGui::SetCursorScreenPos(points[i] - ImVec2(HANDLE_SIZE, HANDLE_SIZE));
        ImGui::InvisibleButton(pointNames[i], ImVec2(HANDLE_SIZE*2, HANDLE_SIZE*2));

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            float y_val = limits[0] + modifyValues[i] * (limits[1] - limits[0]);
            if (i == 1)
            {
                keyPos0 = Math::clamp((io.MousePos.x - cursorPos.x) / region.x, 0.0f, 1.0f);
                Util::String text = Util::Format("X: %f, Y: %f", keyPos0, y_val);
                ImVec2 size = ImGui::CalcTextSize(text.AsCharPtr());
                ImVec2 pos = ImVec2(Math::min(points[i].x + 3, cursorPos.x + region.x - size.x - 3), points[i].y + 3);
                draw_list->AddText(pos, IM_COL32(255, 255, 255, 255), Util::Format("X: %f, Y: %f", keyPos0, y_val).AsCharPtr());
            }
            else if (i == 2)
            {
                keyPos1 = Math::clamp((io.MousePos.x - cursorPos.x) / region.x, 0.0f, 1.0f);
                Util::String text = Util::Format("X: %f, Y: %f", keyPos1, y_val);
                ImVec2 size = ImGui::CalcTextSize(text.AsCharPtr());
                ImVec2 pos = ImVec2(Math::min(points[i].x + 3, cursorPos.x + region.x - size.x - 3), points[i].y + 3);
                draw_list->AddText(pos, IM_COL32(255, 255, 255, 255), Util::Format("X: %f, Y: %f", keyPos1, y_val).AsCharPtr());
            }
            else
            {
                Util::String text = Util::Format("Y: %f", y_val);
                ImVec2 size = ImGui::CalcTextSize(text.AsCharPtr());
                ImVec2 pos = ImVec2(Math::min(points[i].x + 3, cursorPos.x + region.x - size.x - 3), points[i].y + 3);
                draw_list->AddText(pos, IM_COL32(255, 255, 255, 255), Util::Format("Y: %f", y_val).AsCharPtr());
            }
            modifyValues[i] = Math::clamp((io.MousePos.y - cursorPos.y) / window.y, 0.0f, 1.0f);
            changed = true;
        }

        ImGui::PopID();
    }
    curve.SetKeyPos0(keyPos0);
    curve.SetKeyPos1(keyPos1);
    curve.SetValues(modifyValues[0], modifyValues[1], modifyValues[2], modifyValues[3]);
    ImGui::SetCursorScreenPos(cursorPos);
    ImGui::Dummy(ImVec2(region.x, MIN_CURVE_WINDOW_HEIGHT));
    return changed;
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ParticleAssetItemData* data = static_cast<ParticleAssetItemData*>(item->data);

    static const int PARAM_PADDING = 12;
#define CURVE_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        Particles::EnvelopeCurve curve = data->emitters[selectedEmitter].attrs->GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::attr); \
        if (DrawCurve(draw_list, curve)) \
            data->emitters[selectedEmitter].attrs->SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::attr, curve); \
    }

#define FLOAT_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        float f = data->emitters[selectedEmitter].attrs->GetFloat(Particles::EmitterAttrs::FloatAttr::attr); \
        if (ImGui::DragFloat("##" #attr, &f, 0.1f, 0.0f, 10000.0f)) \
            data->emitters[selectedEmitter].attrs->SetFloat(Particles::EmitterAttrs::FloatAttr::attr, f); \
    }

#define INT_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        int f = data->emitters[selectedEmitter].attrs->GetInt(Particles::EmitterAttrs::IntAttr::attr); \
        if (ImGui::DragInt("##" #attr, &f, 1.0f, 0, 10000)) \
            data->emitters[selectedEmitter].attrs->SetInt(Particles::EmitterAttrs::IntAttr::attr, f); \
    }

#define BOOL_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        bool f = data->emitters[selectedEmitter].attrs->etBool(Particles::EmitterAttrs::BoolAttr::attr); \
        if (ImGui::Checkbox("##" #attr, &f)) \
            data->emitters[selectedEmitter].attrs->SetBool(Particles::EmitterAttrs::BoolAttr::attr, f); \
    }
    static uint selectedEmitter = -1;
    if (ImGui::BeginListBox("Emitters"))
    {
        for (SizeT i = 0; i < data->emitters.Size(); i++)
        {
            ImGui::PushID(i + reinterpret_cast<intptr_t>(item));
            if (ImGui::Selectable(data->emitters[i].name.Value()))
            {
                selectedEmitter = i;
            }
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    if (selectedEmitter != -1)
    {
        CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();
        uint i = selectedEmitter;
        if (ImGui::CollapsingHeader("Emission"))
        {
            CURVE_PARAM(Emission Frequency, EmissionFrequency)
            FLOAT_PARAM(Emission Duration (in seconds), EmissionDuration)
            FLOAT_PARAM(Start rotation max (in radians), StartRotationMax)
            FLOAT_PARAM(Start rotation min (in radians), StartRotationMin)

            CURVE_PARAM(Life Time, LifeTime)
            CURVE_PARAM(Spread Min, SpreadMin)
            CURVE_PARAM(Spread Max, SpreadMax)

            FLOAT_PARAM(Precalculation Time, PrecalcTime)
            FLOAT_PARAM(Start Delay (in seconds), StartDelay)
        }
        if (ImGui::CollapsingHeader("Material"))
        {

#define MATERIAL_TEXTURE(desc, member) \
{\
    Util::String texturePath = Editor::PathConverter::MapToCompactPath(texLoader->GetName(data->emitters[selectedEmitter].member.res).Value());\
    if (ImGui::ImageButton(Util::Format(#desc"###%d", selectedEmitter).AsCharPtr(), &data->emitters[selectedEmitter].member.texture, ImVec2{ 32, 32 }))\
    {\
        name = #desc;\
        path = texturePath;\
        imagePressed = true;\
    }\
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))\
    {\
        if (ImGui::BeginTooltip())\
        {\
            ImGui::Image(&data->emitters[selectedEmitter].member.texture, ImVec2{ 256, 256 });\
            ImGui::EndTooltip();\
        }\
    }\
    ImGui::SameLine();\
    if (ImGui::Button(Util::Format("%s###%d", texturePath.AsCharPtr(), selectedEmitter).AsCharPtr()))\
    {\
        name = #desc;\
        path = texturePath;\
        imagePressed = true;\
    }\
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None))\
    {\
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))\
        {\
            ImGui::SetClipboardText(texturePath.AsCharPtr());\
            imagePressed = false;\
        }\
    }\
}

            bool imagePressed = false;
            const char* name = nullptr;
            Util::String path;

            MATERIAL_TEXTURE(Albedo, albedo)
            MATERIAL_TEXTURE(Material, material)
            MATERIAL_TEXTURE(Normals, normals)

            if (imagePressed)
            {
                // TODO: Replace file dialog with asset browser view/instance
                const char* patterns[] = { "*.dds" };
                const char* result = tinyfd_openFileDialog(name, IO::URI(path).LocalPath().AsCharPtr(), 1, patterns, "Texture files (DDS)", false);

                if (result != nullptr)
                {
                    //auto cmd = new CMDMaterialSetTexture;
                    //cmd->bindlessOffset = value->bindlessOffset;
                    //cmd->index = i;
                    //cmd->hash = value->hashedName;
                    //cmd->assetEditor = assetEditor;
                    //cmd->asset = path;
                    //cmd->item = item;
                    //cmd->oldAsset = name;
                    //Edit::CommandManager::Execute(cmd);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSetup(AssetEditorItem* item)
{
	auto itemData = item->allocator.Alloc<ParticleAssetItemData>();
	item->data = itemData;

    Particles::ParticleEmitters& emitters = Particles::ParticleResourceGetMutableEmitters(item->asset.particle);
    for (SizeT i = 0; i < emitters.meshes.Size(); i++)
    {
        ParticleAssetItemData::ParticleAsset emitter;
        emitter.name = emitters.name[i];
        emitter.albedo.res = emitters.albedo[i];
        emitter.albedo.texture.layer = 0;
        emitter.albedo.texture.mip = 0;
        emitter.albedo.texture.nebulaHandle = emitters.albedo[i].resource;
        emitter.material.res = emitters.material[i];
        emitter.material.texture.layer = 0;
        emitter.material.texture.mip = 0;
        emitter.material.texture.nebulaHandle = emitters.material[i].resource;
        emitter.normals.res = emitters.normals[i];
        emitter.normals.texture.layer = 0;
        emitter.normals.texture.mip = 0;
        emitter.normals.texture.nebulaHandle = emitters.normals[i].resource;
        emitter.attrs = &emitters.emitters[i];
        emitter.transform = emitters.transform[i];
        if (emitters.meshes[i] != Resources::InvalidResourceId)
            emitter.mesh = Resources::ResourceServer::Instance()->GetName(emitters.meshes[i]);
        else
            emitter.mesh = "";
        itemData->emitters.Append(emitter);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    Util::String output = Editor::PathConverter::StripAssetName(item->name.AsString());

    Util::String outFile = Util::Format("assets:%s.sur", output.AsCharPtr());
    Ptr<IO::FileStream> stream = IO::FileStream::Create();
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    ParticleSerialize(stream, static_cast<ParticleAssetItemData*>(item->data)->emitters);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
}


//------------------------------------------------------------------------------
/**
*/
void
ParticleNew(const Ptr<IO::Stream>& stream)
{
    ParticleAssetItemData::ParticleAsset res;
    ParticleSerialize(stream, { res });
}

} // namespace Presentation
