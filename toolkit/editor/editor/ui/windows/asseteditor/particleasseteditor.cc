//------------------------------------------------------------------------------
//  @file particleasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "particleasseteditor.h"
#include "particles/emitterattrs.h"
#include "particles/particleresource.h"
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
    struct ParticleAsset
    {
        Util::StringAtom name;
        Particles::EmitterAttrs attrs;
        Resources::ResourceName mesh = "";
        Resources::ResourceName albedo = "systex:white.dds", normals = "systex:nobump.dds", material = "systex:default_material.dds";
        Math::mat4 transform;
    };
    Util::Array<ParticleAsset> emitters;
};

//------------------------------------------------------------------------------
/**
*/
void
ParticleSave(const Ptr<IO::Stream>& stream, const Util::Array<ParticleAssetItemData::ParticleAsset>& emitters)
{
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    writer->Open();

    writer->BeginArray("emitters");


    for (SizeT i = 0; i < emitters.Size(); i++)
    {
        writer->BeginObject("");

        writer->Add(emitters[i].mesh.Value(), "mesh");
        writer->Add(emitters[i].albedo.Value(), "albedo");
        writer->Add(emitters[i].material.Value(), "material");
        writer->Add(emitters[i].normals.Value(), "normals");
        writer->Add(emitters[i].transform, "transform");


        writer->BeginObject("floats");
        for (uint j = 0; j < Particles::EmitterAttrs::FloatAttr::NumFloatAttrs; j++)
        {
            writer->Add(emitters[i].attrs.GetFloat((Particles::EmitterAttrs::FloatAttr)j), Particles::FloatAttrNames[j]);
        }
        writer->End();

        writer->BeginObject("bools");
        for (uint j = 0; j < Particles::EmitterAttrs::BoolAttr::NumBoolAttrs; j++)
        {
            writer->Add(emitters[i].attrs.GetBool((Particles::EmitterAttrs::BoolAttr)j), Particles::BoolAttrNames[j]);
        }
        writer->End();


        writer->BeginObject("ints");
        for (uint j = 0; j < Particles::EmitterAttrs::IntAttr::NumIntAttrs; j++)
        {
            writer->Add(emitters[i].attrs.GetInt((Particles::EmitterAttrs::IntAttr)j), Particles::IntAttrNames[j]);
        }
        writer->End();

        writer->BeginObject("vecs");
        for (uint j = 0; j < Particles::EmitterAttrs::Float4Attr::NumFloat4Attrs; j++)
        {
            writer->Add(emitters[i].attrs.GetVec4((Particles::EmitterAttrs::Float4Attr)j), Particles::Float4AttrNames[j]);
        }
        writer->End();


        writer->BeginObject("curves");
        for (uint j = 0; j < Particles::EmitterAttrs::EnvelopeAttr::NumEnvelopeAttrs; j++)
        {
            Particles::EnvelopeCurve curve = emitters[i].attrs.GetEnvelope((Particles::EmitterAttrs::EnvelopeAttr)j);
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
    for (SizeT i = 0; i < data->emitters.Size(); i++)
    {
        if (ImGui::CollapsingHeader(data->emitters[i].name.Value()))
        {
            if (ImGui::CollapsingHeader("Emission"))
            {
                if (ImGui::CollapsingHeader("Emission Frequency"))
                {
                    Particles::EnvelopeCurve curve = data->emitters[i].attrs.GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::EmissionFrequency);
                    DrawCurve(draw_list, curve);
                    data->emitters[i].attrs.SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::EmissionFrequency, curve);
                }

                if (ImGui::CollapsingHeader("Life Time"))
                {
                    Particles::EnvelopeCurve curve = data->emitters[i].attrs.GetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::LifeTime);
                    DrawCurve(draw_list, curve);
                    data->emitters[i].attrs.SetEnvelope(Particles::EmitterAttrs::EnvelopeAttr::LifeTime, curve);
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

    SizeT numEmitters = Particles::ParticleResourceGetNumEmitters(item->asset.particle);
    for (SizeT i = 0; i < numEmitters; i++)
    {
        Particles::ParticleResourceGetEmitterAttrs(item->asset.particle, i);
        ParticleAssetItemData::ParticleAsset emitter;
        emitter.name = Util::StringAtom::Sprintf("Emitter %d", i);
        itemData->emitters.Append(emitter);
    }
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
    ParticleSave(stream, { res });
}

} // namespace Presentation
