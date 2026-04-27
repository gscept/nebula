//------------------------------------------------------------------------------
//  @file particleasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "particleasseteditor.h"
#include "editor/commandmanager.h"
#include "particles/emitterattrs.h"
#include "particles/particleresource.h"
#include "particles/particlecontext.h"
#include "pjson/pjson.h"
#include "io/filestream.h"
#include "io/jsonwriter.h"
#include "resources/resourceserver.h"
#include "coregraphics/textureloader.h"
#include "materials/materialloader.h"
#include "tinyfiledialogs.h"
#include "editor/tools/pathconverter.h"
#include "materialasseteditor.h"
#include "toolkit-common/converters/binaryxmlconverter.h"

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
        Materials::MaterialId material = Materials::InvalidMaterialId;
        Math::mat4 transform;
        AssetEditorItem materialItem;
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
        writer->Add(Materials::MaterialGetName(emitters[i].material), "material");
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
void
DrawCurvePreview(ImDrawList* draw_list, Particles::EnvelopeCurve& curve, bool& toggled)
{
    ImVec2 size = ImVec2(Math::min(100, ImGui::GetContentRegionAvail().x), ImGui::GetTextLineHeight());
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 points[4];
    const float* limits = curve.GetLimits();
    float normalizationFactor = 1.0f / (limits[1] - limits[0]);

    const float* values = curve.GetValues();
    float keyPos0 = curve.GetKeyPos0();
    float keyPos1 = curve.GetKeyPos1();
    float padding = 2.0f;
    size.x -= padding;
    size.y -= padding;
    points[0] = cursorPos + ImVec2(0, (1.0f - values[0] * normalizationFactor) * size.y);
    points[1] = cursorPos + ImVec2(size.x * keyPos0, (1.0f - values[1] * normalizationFactor) * size.y);
    points[2] = cursorPos + ImVec2(size.x * keyPos1, (1.0f - values[2] * normalizationFactor) * size.y);
    points[3] = cursorPos + ImVec2(size.x * 1.0f, (1.0f - values[3] * normalizationFactor) * size.y);

    ImVec2 paddedCursorMin = cursorPos - ImVec2(padding, padding);
    ImVec2 paddedCursorMax = cursorPos + size + ImVec2(padding, padding);
    draw_list->AddRectFilled(paddedCursorMin, paddedCursorMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
    draw_list->PushClipRect(paddedCursorMin, paddedCursorMax, true);
    draw_list->AddBezierCubic(points[0], points[1], points[2], points[3], ImGui::GetColorU32(ImGuiCol_ButtonActive), 2.0f, 64);
    draw_list->PopClipRect();
    draw_list->AddRect(paddedCursorMin, paddedCursorMax, ImGui::GetColorU32(ImGuiCol_Border));
    ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + 0x789);
    bool ret = ImGui::InvisibleButton("", size);
    if (ImGui::IsItemClicked())
    {
        toggled = !toggled;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
bool
DrawCurve(ImDrawList* draw_list, Particles::EnvelopeCurve& curve, bool& toggled)
{
    bool changed = false;

    ImVec2 region = ImGui::GetWindowSize();
    region.x = ImGui::GetContentRegionAvail().x;

    const float* values = curve.GetValues();
    float keyPos0 = curve.GetKeyPos0();
    float keyPos1 = curve.GetKeyPos1();
    float modifyValues[4];
    memcpy(modifyValues, values, sizeof(modifyValues));

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
        for (uint i = 0; i < 4; i++)
        {
            float newValue = modifyValues[i];
            float normalizedValue = limits[0] + newValue * (limits[1] - limits[0]);
            modifyValues[i] = Math::clamp(modifyValues[i], modifyLimits[0], modifyLimits[1]);
        }
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + 0x456);
    if (ImGui::DragFloat("Max", &modifyLimits[1], 0.5f, 0.0f, 10000.0f))
    {
        curve.SetLimits(modifyLimits[0], modifyLimits[1]);
        changed = true;
        for (uint i = 0; i < 4; i++)
        {
            float newValue = modifyValues[i];
            float normalizedValue = limits[0] + newValue * (limits[1] - limits[0]);
            modifyValues[i] = Math::clamp(modifyValues[i], modifyLimits[0], modifyLimits[1]);
        }
    }
    ImGui::PopID();

    Dynui::ImGuiCloseButton(toggled, reinterpret_cast<intptr_t>(&curve));


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

    ImU32 Colors[] =
    {
        ImGui::GetColorU32(ImGuiCol_ButtonActive),     // key point 0 fill
        ImGui::GetColorU32(ImGuiCol_ButtonHovered),      // key point 0 outline
        ImGui::GetColorU32(ImGuiCol_ButtonActive),        // control point 0
        ImGui::GetColorU32(ImGuiCol_ButtonActive),        // control point 1
        ImGui::GetColorU32(ImGuiCol_ButtonActive),     // key point 1 fill
        ImGui::GetColorU32(ImGuiCol_ButtonHovered),      // key point 1 outline
    };

    float normalizationFactor = 1.0f / (limits[1] - limits[0]);
    
    ImVec2 window = ImVec2(Math::min(narrowRegion.x, MIN_CURVE_WINDOW_WIDTH), Math::min(narrowRegion.y, MIN_CURVE_WINDOW_HEIGHT));
    points[0] = narrowCursor + ImVec2(0.0f, (1.0f - values[0] * normalizationFactor) * window.y);
    points[1] = narrowCursor + ImVec2(narrowRegion.x * keyPos0, (1.0f - values[1] * normalizationFactor) * window.y);
    points[2] = narrowCursor + ImVec2(narrowRegion.x * keyPos1, (1.0f - values[2] * normalizationFactor) * window.y);
    points[3] = narrowCursor + ImVec2(narrowRegion.x * 1.0f, (1.0f - values[3] * normalizationFactor) * window.y);
    draw_list->AddRectFilled(cursorPos, cursorPos + ImVec2(region.x, MIN_CURVE_WINDOW_HEIGHT), ImGui::GetColorU32(ImGuiCol_FrameBg), 8.0f);
    draw_list->AddRect(narrowCursor, narrowCursor + ImVec2(narrowRegion.x, MIN_CURVE_WINDOW_HEIGHT - INNER_WINDOW_PADDING*2), ImGui::GetColorU32(ImGuiCol_FrameBgActive), 2.0f);
    draw_list->AddBezierCubic(points[0], points[1], points[2], points[3], ImGui::GetColorU32(ImGuiCol_SeparatorActive), 2.0f, 64);
    draw_list->AddLine(points[0], points[1], ImGui::GetColorU32(ImGuiCol_ButtonActive));
    draw_list->AddLine(points[2], points[3], ImGui::GetColorU32(ImGuiCol_ButtonActive));
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
            float y_val = modifyValues[i];
            if (i == 1)
            {
                keyPos0 = Math::clamp((io.MousePos.x - cursorPos.x) / region.x, 0.f, 1.f);
                Util::String text = Util::Format("X: %f, Y: %f", keyPos0, y_val);
                ImVec2 size = ImGui::CalcTextSize(text.AsCharPtr());
                ImVec2 pos = ImVec2(Math::min(points[i].x + 3, cursorPos.x + region.x - size.x - 3), points[i].y + 3);
                draw_list->AddText(pos, IM_COL32(255, 255, 255, 255), Util::Format("X: %f, Y: %f", keyPos0, y_val).AsCharPtr());
            }
            else if (i == 2)
            {
                keyPos1 = Math::clamp((io.MousePos.x - cursorPos.x) / region.x, 0.f, 1.f);
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
            modifyValues[i] = Math::clamp((1.0f - ((io.MousePos.y - cursorPos.y) / window.y)) * (1.0f / normalizationFactor), limits[0], limits[1]);
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
    static bool EditorsOpen[Particles::EmitterAttrs::EnvelopeAttr::NumEnvelopeAttrs] = { false };
#define CURVE_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();\
        ImGui::Text(#desc); \
        Particles::EnvelopeCurve curve = data->emitters[selectedEmitter].attrs->GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::attr); \
        ImGui::SameLine();\
        DrawCurvePreview(draw_list, curve, EditorsOpen[Particles::EmitterAttrs::EnvelopeAttr::attr]); \
        ImGui::SetCursorScreenPos(cursorPos + ImVec2(0, ImGui::GetTextLineHeight() + 5));\
        if (EditorsOpen[Particles::EmitterAttrs::EnvelopeAttr::attr])\
        {\
            ImGui::PushID(reinterpret_cast<intptr_t>(&curve) + Particles::EmitterAttrs::EnvelopeAttr::attr);\
            {\
                if (DrawCurve(draw_list, curve, EditorsOpen[Particles::EmitterAttrs::EnvelopeAttr::attr])) \
                {\
                    data->emitters[selectedEmitter].attrs->SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::attr, curve); \
                    assetEditor->Edit();\
                    Particles::ParticleContext::RecalculateEnvelopeSamples(item->previewObject);\
                    Particles::ParticleContext::Play(item->previewObject, Particles::ParticleContext::PlayMode::RestartIfPlaying);\
                }\
            }\
            ImGui::PopID();\
        }\
        ImGui::Dummy(ImVec2(0,0));\
    }

#define FLOAT_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        float f = data->emitters[selectedEmitter].attrs->GetFloat(Particles::EmitterAttrs::FloatAttr::attr); \
        if (ImGui::DragFloat("##" #attr, &f, 0.1f, 0.0f, 10000.0f)) \
        {\
            data->emitters[selectedEmitter].attrs->SetFloat(Particles::EmitterAttrs::FloatAttr::attr, f); \
            assetEditor->Edit();\
            Particles::ParticleContext::RecalculateEnvelopeSamples(item->previewObject);\
            Particles::ParticleContext::Play(item->previewObject, Particles::ParticleContext::PlayMode::RestartIfPlaying);\
        }\
    }

#define INT_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        int f = data->emitters[selectedEmitter].attrs->GetInt(Particles::EmitterAttrs::IntAttr::attr); \
        if (ImGui::DragInt("##" #attr, &f, 1.0f, 0, 10000)) \
        {\
            data->emitters[selectedEmitter].attrs->SetInt(Particles::EmitterAttrs::IntAttr::attr, f); \
            assetEditor->Edit();\
            Particles::ParticleContext::RecalculateEnvelopeSamples(item->previewObject);\
            Particles::ParticleContext::Play(item->previewObject, Particles::ParticleContext::PlayMode::RestartIfPlaying);\
        }\
    }

#define BOOL_PARAM(desc, attr) \
    { \
        ImGui::Dummy(ImVec2(0, PARAM_PADDING));\
        ImGui::Text(#desc); \
        bool f = data->emitters[selectedEmitter].attrs->GetBool(Particles::EmitterAttrs::BoolAttr::attr); \
        if (ImGui::Checkbox("##" #attr, &f)) \
        {\
            data->emitters[selectedEmitter].attrs->SetBool(Particles::EmitterAttrs::BoolAttr::attr, f); \
            assetEditor->Edit();\
            Particles::ParticleContext::RecalculateEnvelopeSamples(item->previewObject);\
            Particles::ParticleContext::Play(item->previewObject, Particles::ParticleContext::PlayMode::RestartIfPlaying);\
        }\
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
        if (ImGui::BeginTable("ParticleEditorTable", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupScrollFreeze(2, 1);
            ImGui::TableNextColumn();
            assetEditor->viewport.Render();
            ImGui::TableNextColumn();
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
                BOOL_PARAM(Loop, Looping)
            }
            if (ImGui::CollapsingHeader("Movement"))
            {
                CURVE_PARAM(Initial velocity, StartVelocity)
                CURVE_PARAM(Rotation velocity, RotationVelocity)
                CURVE_PARAM(Size, Size)
                CURVE_PARAM(Spread min, SpreadMin)
                CURVE_PARAM(Spread max, SpreadMax)
                CURVE_PARAM(Air resistance, AirResistance)
                CURVE_PARAM(Velocity weight, VelocityFactor)
                CURVE_PARAM(Mass, Mass)
                CURVE_PARAM(Time scale, TimeManipulator)
            }
            if (ImGui::CollapsingHeader("Material"))
            {
                FLOAT_PARAM(Tile Size (in pixels), TextureTile)
                INT_PARAM(Tiles per row, AnimPhases)
                FLOAT_PARAM(Tile Change time (per second), PhasesPerSecond)
                BOOL_PARAM(Face camera, Billboard)
                BOOL_PARAM(Camera fade, ViewAngleFade)

                // Use the material editor to edit a particles material
                if (selectedEmitter != -1)
                {
                    MaterialEditor(assetEditor, &data->emitters[selectedEmitter].materialItem);
                }

                CURVE_PARAM(Red Tint, Red)
                CURVE_PARAM(Green Tint, Green)
                CURVE_PARAM(Blue Tint, Blue)
                CURVE_PARAM(Opacity, Alpha)
            }
            ImGui::EndTable();
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
        emitter.material = emitters.materials[i];
        emitter.attrs = &emitters.attrs[i];
        emitter.transform = emitters.transform[i];
        if (emitters.meshes[i] != Resources::InvalidResourceId)
            emitter.mesh = Resources::ResourceServer::Instance()->GetName(emitters.meshes[i]);
        else
            emitter.mesh = "";

        const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
        MaterialEditorItemData* matItemData = item->allocator.Alloc<MaterialEditorItemData>();
        matItemData->constants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
        matItemData->images = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
        matItemData->originalConstants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
        matItemData->originalImages = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
        emitter.materialItem.data = matItemData;
        matItemData->renderViewport = false;
        emitter.materialItem.asset.material = item->asset.material;
        emitter.materialItem.assetType = Presentation::AssetEditor::AssetType::Material;
        itemData->emitters.Append(emitter);

        // Copy over material constants
        ubyte* currentData = Materials::MaterialGetConstants(item->asset.material);
        memcpy(matItemData->constants, currentData, materialTemplate->bufferSize);
        memcpy(matItemData->originalConstants, currentData, materialTemplate->bufferSize);

        for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
        {
            auto kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
            Resources::ResourceId res = Materials::MaterialGetTexture(item->asset.material, kvp.Value()->textureIndex);
            if (res.resourceId == Resources::InvalidResourceId.resourceId)
                res = Resources::CreateResource(kvp.Value()->resource, "editor");
            Resources::CreateResourceListener(res, [matItemData, i](Resources::ResourceId res)
            {
                matItemData->images[i].res = res;
                matItemData->images[i].texture.layer = 0;
                matItemData->images[i].texture.mip = 0;
                matItemData->images[i].texture.nebulaHandle = res.resource;
                memcpy(&matItemData->originalImages[i], &matItemData->images[i], sizeof(ImageHolder));
            });

            matItemData->images[i].res = res;
            matItemData->images[i].texture.layer = 0;
            matItemData->images[i].texture.mip = 0;
            matItemData->images[i].texture.nebulaHandle = res.resource;
            memcpy(&matItemData->originalImages[i], &matItemData->images[i], sizeof(ImageHolder));
        }
    }

    Particles::ParticleContext::RegisterEntity(item->previewObject);
    Particles::ParticleContext::Setup(item->previewObject, item->name, 1 << 3);
    Particles::ParticleContext::Play(item->previewObject, Particles::ParticleContext::PlayMode::RestartIfPlaying);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    Util::String output = Editor::PathConverter::StripAssetName(item->name.AsString());
    Ptr<IO::FileStream> stream = IO::FileStream::Create();
    stream->SetURI(item->name.Value());
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    ParticleSerialize(stream, static_cast<ParticleAssetItemData*>(item->data)->emitters);
    assetEditor->Unedit(item->editCounter);
    item->editCounter = 0;

    /*
    Util::String newMaterialPath = item->name.Value();
    newMaterialPath.ChangeFileExtension("sur");
    Util::String exportPath = newMaterialPath;
    exportPath.ChangeAssignMapping("sur", "proj:work/assets/");
    Ptr<IO::FileStream> matStream = IO::FileStream::Create();
    matStream->SetURI(newMaterialPath);
    matStream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    MaterialSerialize(matStream, item->, res.material, matTemplate);
    */
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
ParticleNew(const Ptr<IO::Stream>& stream, const Util::String& path)
{
    ParticleAssetItemData::ParticleAsset res;
    res.name = "New Particle Emitter";
    Particles::EmitterAttrs attrs;

    #define INIT_CURVE_ONE(curve)\
        {\
            auto curve = attrs.GetEnvelope(Particles::EmitterAttrs::curve);\
            curve.Setup(1,1,1,1, 0.33, 0.66, 0, 0, Particles::EnvelopeCurve::Sine);\
            attrs.SetEnvelope(Particles::EmitterAttrs::curve, curve);\
        }

    INIT_CURVE_ONE(Red);
    INIT_CURVE_ONE(Blue);
    INIT_CURVE_ONE(Green);
    INIT_CURVE_ONE(Alpha);
    INIT_CURVE_ONE(LifeTime);
    INIT_CURVE_ONE(EmissionFrequency);
    INIT_CURVE_ONE(Size)
    attrs.SetFloat(Particles::EmitterAttrs::SizeRandomize, 1.0f);
    attrs.SetFloat(Particles::EmitterAttrs::EmissionDuration, 10);
    attrs.SetBool(Particles::EmitterAttrs::Looping, true);
    res.attrs = &attrs;

    // Setup new material
    MaterialTemplatesGPULang::Entry* matTemplate = &MaterialTemplatesGPULang::base::__ParticleLit.entry;
    Util::String newMaterialPath = path;
    newMaterialPath.ChangeFileExtension("sur");
    Util::String exportPath = newMaterialPath;
    exportPath.ChangeAssignMapping("sur", "proj:work/assets/");
    res.material = Materials::CreateMaterial(matTemplate, exportPath);

    MaterialEditorItemData itemData;
    itemData.constants = ArrayAllocStack<ubyte>(matTemplate->bufferSize);
    itemData.images = ArrayAllocStack<ImageHolder>(matTemplate->numTextures);
    itemData.originalConstants = ArrayAllocStack<ubyte>(matTemplate->bufferSize);
    itemData.originalImages = ArrayAllocStack<ImageHolder>(matTemplate->numTextures);

    ubyte* currentData = Materials::MaterialGetConstants(res.material);
    memcpy(itemData.constants, currentData, matTemplate->bufferSize);
    memcpy(itemData.originalConstants, currentData, matTemplate->bufferSize);

    for (IndexT i = 0; i < matTemplate->textures.Size(); i++)
    {
        auto kvp = matTemplate->textures.KeyValuePairAtIndex(i);
        Resources::ResourceId tex = Materials::MaterialGetTexture(res.material, kvp.Value()->textureIndex);
        if (tex.resourceId == Resources::InvalidResourceId.resourceId)
            tex = Resources::CreateResource(kvp.Value()->resource, "editor");
        Resources::CreateResourceListener(tex, [itemData, i](Resources::ResourceId res)
        {
            itemData.images[i].res = res;
            itemData.images[i].texture.layer = 0;
            itemData.images[i].texture.mip = 0;
            itemData.images[i].texture.nebulaHandle = res.resource;
            memcpy(&itemData.originalImages[i], &itemData.images[i], sizeof(ImageHolder));
        });

        itemData.images[i].res = tex;
        itemData.images[i].texture.layer = 0;
        itemData.images[i].texture.mip = 0;
        itemData.images[i].texture.nebulaHandle = tex.resource;
        memcpy(&itemData.originalImages[i], &itemData.images[i], sizeof(ImageHolder));
    }

    memcpy(itemData.originalImages, itemData.images, sizeof(ImageHolder) * matTemplate->numTextures);

    Ptr<IO::FileStream> matStream = IO::FileStream::Create();
    matStream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    matStream->SetURI(newMaterialPath);
    MaterialSerialize(matStream, &itemData, res.material, matTemplate);

    // Also perform export
    ToolkitUtil::BinaryXmlConverter converter;
    ToolkitUtil::Logger logger;
    converter.ConvertFile(newMaterialPath, exportPath, logger);

    ArrayFreeStack(matTemplate->numTextures, itemData.originalImages);
    ArrayFreeStack(matTemplate->bufferSize, itemData.originalConstants);
    ArrayFreeStack(matTemplate->numTextures, itemData.images);
    ArrayFreeStack(matTemplate->bufferSize, itemData.constants);

    ParticleSerialize(stream, { res });
}

} // namespace Presentation
