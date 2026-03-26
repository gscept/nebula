//------------------------------------------------------------------------------
//  @file particleasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "particleasseteditor.h"
#include "particles/emitterattrs.h"
#include "pjson/pjson.h"
#include "io/jsonwriter.h"

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
    Particles::EmitterAttrs attrs;
    Resources::ResourceName mesh;
};

//------------------------------------------------------------------------------
/**
*/
void
ParticleSave(const Ptr<IO::Stream>& stream, const Particles::EmitterAttrs& attrs, const Resources::ResourceName& mesh)
{
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    writer->Open();

    writer->BeginArray("emitters");

    writer->BeginObject("");

    writer->Add(mesh.Value(), "mesh");


    writer->BeginObject("floats");
    for (uint i = 0; i < Particles::EmitterAttrs::FloatAttr::NumFloatAttrs; i++)
    {
        writer->Add(attrs.GetFloat((Particles::EmitterAttrs::FloatAttr)i), Particles::FloatAttrNames[i]);
    }
    writer->End();

    writer->BeginObject("bools");
    for (uint i = 0; i < Particles::EmitterAttrs::BoolAttr::NumBoolAttrs; i++)
    {
        writer->Add(attrs.GetBool((Particles::EmitterAttrs::BoolAttr)i), Particles::BoolAttrNames[i]);
    }
    writer->End();


    writer->BeginObject("ints");
    for (uint i = 0; i < Particles::EmitterAttrs::IntAttr::NumIntAttrs; i++)
    {
        writer->Add(attrs.GetInt((Particles::EmitterAttrs::IntAttr)i), Particles::IntAttrNames[i]);
    }
    writer->End();

    writer->BeginObject("vecs");
    for (uint i = 0; i < Particles::EmitterAttrs::Float4Attr::NumFloat4Attrs; i++)
    {
        writer->Add(attrs.GetVec4((Particles::EmitterAttrs::Float4Attr)i), Particles::Float4AttrNames[i]);
    }
    writer->End();


    writer->BeginObject("curves");
    for (uint i = 0; i < Particles::EmitterAttrs::EnvelopeAttr::NumEnvelopeAttrs; i++)
    {
        Particles::EnvelopeCurve curve = attrs.GetEnvelope((Particles::EmitterAttrs::EnvelopeAttr)i);
        writer->Add(curve, Particles::EnvelopeAttrNames[i]);
    }
    writer->End();

    // Close object
    writer->End();

    // Close array
    writer->End();

    writer->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
DrawCurve(ImDrawList* draw_list, Particles::EnvelopeCurve& curve)
{
    const float* values = curve.GetValues();
    ImVec2 points[4];
    draw_list->AddBezierCubic(points[0], points[1], points[2], points[3], IM_COL32(255, 0, 0, 255), 2.0f, 64);
    draw_list->AddCircleFilled(points[0], 5.0f, IM_COL32(255,0,0,255));
    draw_list->AddCircleFilled(points[1], 5.0f, IM_COL32(0,255,0,255));
    draw_list->AddCircleFilled(points[2], 5.0f, IM_COL32(0,255,0,255));
    draw_list->AddCircleFilled(points[3], 5.0f, IM_COL32(255,0,0,255));
    draw_list->AddLine(points[0], points[1], IM_COL32(100, 100, 100, 255));
    draw_list->AddLine(points[2], points[3], IM_COL32(100,100,100,255));

    ImGui::SetCursorScreenPos(ImVec2(points[0].x - 5, points[0].y - 5));
    ImGui::InvisibleButton("p0", ImVec2(10,10));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) 
    {
        points[0].x += ImGui::GetIO().MouseDelta.x;
        points[0].y += ImGui::GetIO().MouseDelta.y;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ParticleAssetItemData* data = static_cast<ParticleAssetItemData*>(item->data);
    if (ImGui::CollapsingHeader("Emission"))
    {
        if (ImGui::CollapsingHeader("Emission Frequency"))
	    {
            Particles::EnvelopeCurve curve = data->attrs.GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::EmissionFrequency);
            DrawCurve(draw_list, curve);
            data->attrs.SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::EmissionFrequency, curve);
	    }

        if (ImGui::CollapsingHeader("Life Time"))
	    {
            Particles::EnvelopeCurve curve = data->attrs.GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::LifeTime);
            DrawCurve(draw_list, curve);
            data->attrs.SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::LifeTime, curve);
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
    Particles::EmitterAttrs emitter;
    Resources::ResourceName mesh = "";
    ParticleSave(stream, emitter, mesh);
}

} // namespace Presentation
