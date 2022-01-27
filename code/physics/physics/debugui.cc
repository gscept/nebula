//------------------------------------------------------------------------------
//  physicsinterface.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "physics/debugui.h"
#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "dynui/imguicontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "physics/utils.h"
#include "imgui.h"
#include "graphics/cameracontext.h"
#include "flat/physics/material.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "io/safefilestream.h"
#include "io/textwriter.h"


using namespace physx;

namespace
{
struct DebugState
{
    DebugState()
    {
        flags.Add("World Axes",true);
        flagsMap.Add("World Axes",PxVisualizationParameter::eWORLD_AXES);
        flags.Add("Body Axes",true);
        flagsMap.Add("Body Axes",PxVisualizationParameter::eBODY_AXES);
        flags.Add("Mass Axes",true);
        flagsMap.Add("Mass Axes",PxVisualizationParameter::eBODY_MASS_AXES);
        flags.Add("Linear Velocity",true);
        flagsMap.Add("Linear Velocity",PxVisualizationParameter::eBODY_LIN_VELOCITY);
        flags.Add("Ang Velocity",true);
        flagsMap.Add("Ang Velocity",PxVisualizationParameter::eBODY_ANG_VELOCITY);
        flags.Add("Contact",true);
        flagsMap.Add("Contact",PxVisualizationParameter::eCONTACT_POINT);
        flags.Add("Contact Normal",true);
        flagsMap.Add("Contact Normal",PxVisualizationParameter::eCONTACT_NORMAL);
        flags.Add("Contact Error",true);
        flagsMap.Add("Contact Error",PxVisualizationParameter::eCONTACT_ERROR);
        flags.Add("Contact Force",true);
        flagsMap.Add("Contact Force",PxVisualizationParameter::eCONTACT_FORCE);
        flags.Add("Actor Axes",true);
        flagsMap.Add("Actor Axes",PxVisualizationParameter::eACTOR_AXES);
        flags.Add("Collision AABB",false);
        flagsMap.Add("Collision AABB",PxVisualizationParameter::eCOLLISION_AABBS);
        flags.Add("Collision Shape",false);
        flagsMap.Add("Collision Shape",PxVisualizationParameter::eCOLLISION_SHAPES);
        flags.Add("Collision Compounds",true);
        flagsMap.Add("Collision Compounds",PxVisualizationParameter::eCOLLISION_COMPOUNDS);
        flags.Add("Collision Facenormals",false);
        flagsMap.Add("Collision Facenormals",PxVisualizationParameter::eCOLLISION_FNORMALS);
        flags.Add("Collision Edges",false);
        flagsMap.Add("Collision Edges",PxVisualizationParameter::eCOLLISION_EDGES);
        flags.Add("Collision Dynamic",true);
        flagsMap.Add("Collision Dynamic",PxVisualizationParameter::eCOLLISION_DYNAMIC);
        flags.Add("Collision Static",true);
        flagsMap.Add("Collision Static",PxVisualizationParameter::eCOLLISION_STATIC);
        flags.Add("Joint Frames",true);
        flagsMap.Add("Joint Frames",PxVisualizationParameter::eJOINT_LOCAL_FRAMES);
        flags.Add("Joint Limits",true);
        flagsMap.Add("Joint Limits",PxVisualizationParameter::eJOINT_LIMITS);
        flags.Add("Culling",true);
        flagsMap.Add("Culling",PxVisualizationParameter::eCULL_BOX);
        flags.Add("MBP Regions",true);
        flagsMap.Add("MBP Regions",PxVisualizationParameter::eMBP_REGIONS);
    }

    void ApplyAllFlags()
    {
        for (auto& scene : Physics::state.activeScenes)
        {
            for (auto& entry : this->flags)
            {
                scene.scene->setVisualizationParameter(this->flagsMap[entry.Key()], entry.Value()? 1.0f : 0.0f);
            }
        }
    }

    float scale = 1.0f;
    Util::Dictionary<Util::String, bool> flags;
    Util::Dictionary<Util::String, physx::PxVisualizationParameter::Enum> flagsMap;
    bool enabled = false;
};
DebugState dstate;

void ApplyToScenes(PxVisualizationParameter::Enum flag, float value)
{
    for (auto& scene : Physics::state.activeScenes)
    {
        scene.scene->setVisualizationParameter(flag, value);
    }
}

void ApplyBounds(PxBounds3 const& box)
{
    for (auto& scene : Physics::state.activeScenes)
    {
        scene.scene->setVisualizationCullingBox(box);
    }
}

Math::vec4 Px2Colour(PxU32 col)
{
    const float div = 1.0f / 255.0f;
    return Math::vec4(div * (col >> 24), div * ((col & 0x00FF0000) >> 16), div * ((col & 0x0000FF00) >> 8), div * (col & 0xFF));
}
void RenderScene(PxScene* scene)
{
    const PxRenderBuffer& rb = scene->getRenderBuffer();
    // there seem to be no text buffers ever, lets assert to be sure
    n_assert(rb.getNbTexts() == 0);
    for (PxU32 i = 0, j = rb.getNbLines(); i < j; i++)
    {
        const PxDebugLine& line = rb.getLines()[i];
        Im3d::Im3dContext::DrawLine(Math::line(Px2NebPoint(line.pos0),Px2NebPoint(line.pos1)), 1.0f,Px2Colour(line.color0), Im3d::RenderFlag::AlwaysOnTop);
    }
    for (PxU32 i = 0, j = rb.getNbPoints(); i < j; i++)
    {
        const PxDebugPoint& p = rb.getPoints()[i];
        Im3d::Im3dContext::DrawPoint(Px2NebVec(p.pos), 10.0f, Px2Colour(p.color));
    }
     for (PxU32 i = 0, j = rb.getNbTriangles(); i < j; i++)
    {
        const PxDebugTriangle& t = rb.getTriangles()[i];
        Im3d::Im3dContext::DrawLine(Math::line(Px2NebPoint(t.pos0),Px2NebPoint(t.pos1)), 1.0f,Px2Colour(t.color0), Im3d::RenderFlag::AlwaysOnTop);
        Im3d::Im3dContext::DrawLine(Math::line(Px2NebPoint(t.pos1),Px2NebPoint(t.pos2)), 1.0f,Px2Colour(t.color1), Im3d::RenderFlag::AlwaysOnTop);
        Im3d::Im3dContext::DrawLine(Math::line(Px2NebPoint(t.pos2),Px2NebPoint(t.pos0)), 1.0f,Px2Colour(t.color2), Im3d::RenderFlag::AlwaysOnTop);
    }
}
void RenderPhysicsDebug()
{
    for (auto& scene : Physics::state.activeScenes)
    {
        RenderScene(scene.scene);
    }
}

}

namespace Physics
{

void RenderUI(Graphics::GraphicsEntityId camera)
{
    ImGui::Begin("Physics");
    if (ImGui::Checkbox("Debug Rendering", &dstate.enabled))
    {
        if (!dstate.enabled)
        {
            ApplyToScenes(PxVisualizationParameter::eSCALE, 0.0f);
        }
        else
        {
            ApplyToScenes(PxVisualizationParameter::eSCALE, 1.0f);
            dstate.ApplyAllFlags();
        }
    }
    if(dstate.enabled)
    {
        static float viewRange = 50.0f;
        ImGui::PushItemWidth(100.0f);
        ImGui::DragFloat("Viewing Distance", &viewRange, 1.0f, 1.0f, 100.0f);
        ImGui::PopItemWidth();
        Math::mat4 trans =  Math::inverse(Graphics::CameraContext::GetView(camera));

        Math::point center = trans.get_w() - trans.get_z() * viewRange;
        PxBounds3 bound = PxBounds3::centerExtents(Neb2PxPnt(center), PxVec3(viewRange));
        ApplyBounds(bound);

        ImGui::BeginChild("Debug Flags");
        ImGui::PushItemWidth(100.0f);
        ImGui::DragFloat("Scale", &dstate.scale, 0.1f, 0.01f, 10.0f);
        ImGui::PopItemWidth();
        
        ApplyToScenes(PxVisualizationParameter::eSCALE, dstate.scale);
        for (auto& entry : dstate.flags)
        {
            ImGui::Checkbox(entry.Key().AsCharPtr(), &entry.Value());
            ApplyToScenes(dstate.flagsMap[entry.Key()], entry.Value()?1.0f : 0.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (dstate.enabled)
    {
        RenderPhysicsDebug();
    }
    RenderMaterialsUI();
}

void RenderMaterialsUI()
{
    ImGui::Begin("Physics Materials");
    static bool hasSelection = false;
    static int selected = 0;
    static MaterialDefinitionT original;
    static MaterialDefinitionT current;
    if (!hasSelection || ImGui::Combo("Selected", &selected,
        [](void* data, int n, const char** ret)
        {
            *ret = GetMaterial(n).name.Value();
            return true;
        },
        nullptr, GetNrMaterials()))
    {
        hasSelection = true;
        const Material& mat = GetMaterial(selected);
        original.density = mat.density;
        original.dynamic_friction = mat.material->getDynamicFriction();
        original.static_friction = mat.material->getStaticFriction();
        original.restitution = mat.material->getRestitution();
        current = original;
    }
    static bool dirty = false;
    if (dirty || ImGui::DragFloat("Density", &current.density, 0.05f, 0.0f, 1000.0f))
    {
        GetMaterial(selected).density = current.density;
    }
    if (dirty || ImGui::DragFloat("Dynamic Friction", &current.dynamic_friction, 0.05f, 0.0f, 2.0f))
    {
        GetMaterial(selected).material->setDynamicFriction(current.dynamic_friction);
    }
    if (dirty || ImGui::DragFloat("Static Friction", &current.static_friction, 0.05f, 0.0f, 2.0f))
    {
        GetMaterial(selected).material->setStaticFriction(current.static_friction);
    }
    if (dirty || ImGui::DragFloat("Restitution", &current.restitution, 0.05f, 0.0f, 2.0f))
    {
        GetMaterial(selected).material->setRestitution(current.restitution);
    }
    dirty = false;
    if (ImGui::Button("Reset",ImVec2(80, 0)))
    {
        current = original;
        dirty = true;
    }
    ImGui::SameLine();
    static char nameBuf[15];
    if (ImGui::Button("Add new",ImVec2(80, 0)))
    {
        ImGui::OpenPopup("New Material");
        nameBuf[0] = 0;
    }
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("New Material", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Input", nameBuf, IM_ARRAYSIZE(nameBuf));
        if (ImGui::Button("Ok", ImVec2(80, 0)))
        {
            Util::String name(nameBuf);
            if (!name.IsEmpty())
            {
                CreateMaterial(name.AsCharPtr(),0.5f,0.5f,0.5f,1.0f);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save", ImVec2(80, 0)))
    {
        MaterialsT mtable;
        for (int i = 0, j = GetNrMaterials(); i < j; i++)
        {
            std::unique_ptr<MaterialDefinitionT> def = std::make_unique<MaterialDefinitionT>();
            Material const& mat = GetMaterial(i);
            def->density = mat.density;
            def->dynamic_friction = mat.material->getDynamicFriction();
            def->static_friction = mat.material->getStaticFriction();
            def->restitution = mat.material->getRestitution();
            def->name = mat.name.AsString();
            mtable.entries.push_back(std::move(def));
        }
        Util::String saved = SerializeFlatbufferText(Materials, mtable);
        IO::URI tablePath = "root:work/data/tables/physicsmaterials.json";
        Util::String wrappedPath = "safefile:///" + tablePath.LocalPath();
        Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(wrappedPath);
        Ptr<IO::TextWriter> writer = IO::TextWriter::Create();
        stream->SetAccessMode(IO::Stream::WriteAccess);
        writer->SetStream(stream);
        if (writer->Open())
        {
            writer->WriteString(saved);
        }
        writer->Close();
        CompileFlatbuffer(Materials, tablePath, "phys:");
    }
    ImGui::End();
}

}
