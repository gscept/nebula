//------------------------------------------------------------------------------
//  physicsinterface.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "PxPhysicsAPI.h"
#include "physics/debugui.h"
#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "dynui/imguicontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "physics/utils.h"
#include "imgui.h"
#include "flat/physics/material.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "io/safefilestream.h"
#include "io/textwriter.h"
#include "core/cvar.h"

using namespace physx;

namespace Physics
{

// enable/disable physics visualization
Core::CVar* cl_debug_draw_physics               = Core::CVarCreate(Core::CVarType::CVar_Int, "cl_debug_draw_physics", "0", "Set to 1 will enable physics debug drawing");

// visualization parameters
Core::CVar* cl_physics_draw_scale               = Core::CVarCreate(Core::CVar_Float, "cl_physics_draw_scale", "1.0", "Visualization drawing scale.");
Core::CVar* cl_physics_draw_scale_points        = Core::CVarCreate(Core::CVar_Float, "cl_physics_draw_scale_points", "1.0", "Point visualization drawing scale.");
Core::CVar* cl_physics_draw_scale_lines         = Core::CVarCreate(Core::CVar_Float, "cl_physics_draw_scale_lines", "1.0", "Line visualization drawing scale.");
Core::CVar* cl_physics_draw_scale_triangles     = Core::CVarCreate(Core::CVar_Float, "cl_physics_draw_scale_triangles", "1.0", "Triangle visualization drawing scale.");
Core::CVar* cl_physics_draw_world_axes          = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_world_axes", "0", "Visualize the world axes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_body_axes           = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_body_axes", "0", "Visualize a bodies axes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_body_mass_axes      = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_body_mass_axes", "0", "Visualize a body's mass axes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_body_lin_velocity   = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_body_lin_velocity", "0", "Visualize the bodies linear velocity. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_body_ang_velocity   = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_body_ang_velocity", "0", "Visualize the bodies angular velocity. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_contact_point       = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_contact_point", "0", " Visualize contact points. Will enable contact information. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_contact_normal      = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_contact_normal", "0", "Visualize contact normals. Will enable contact information. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_contact_error       = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_contact_error", "0", " Visualize contact errors. Will enable contact information. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_contact_force       = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_contact_force", "0", "Visualize Contact forces. Will enable contact information. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_actor_axes          = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_actor_axes", "0", "Visualize actor axes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_aabbs     = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_aabbs", "0", "Visualize bounds (AABBs in world space). Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_shapes    = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_shapes", "1", "Shape visualization. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_axes      = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_axes", "0", "Shape axis visualization. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_compounds = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_compounds", "0", "Compound visualization (compound AABBs in world space). Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_fnormals  = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_fnormals", "0", "Mesh & convex face normals. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_edges     = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_edges", "0", "Active edges for meshes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_static    = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_static", "0", "Static pruning structures. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_collision_dynamic   = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_collision_dynamic", "0", "Dynamic pruning structures. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_joint_local_frames  = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_joint_local_frames", "0", "Visualize joint local axes. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_joint_limits        = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_joint_limits", "0", "Visualize joint limits. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_cull_box            = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_cull_box", "0", "Visualize culling box. Needs cl_debug_draw_physics set to 1.");
Core::CVar* cl_physics_draw_mbp_regions         = Core::CVarCreate(Core::CVar_Int, "cl_physics_draw_mbp_regions", "0", "MBP regions. Needs cl_debug_draw_physics set to 1.");

struct DebugState
{
    bool enabled = false;
    DebugDrawInterface drawInterface;
};
DebugState dstate;

//--------------------------------------------------------------------------
/**
*/
void
SetDebugDrawInterface(DebugDrawInterface const& drawInterface)
{
    dstate.drawInterface = drawInterface;
}

//--------------------------------------------------------------------------
/**
*/
void
UpdateDebugDrawingParameters()
{
    for (int sceneIndex = 0; sceneIndex < state.activeScenes.Size(); sceneIndex++)
    {
        PxScene* scene = state.activeScenes[sceneIndex].scene;
        float drawingScale = Core::CVarReadFloat(cl_physics_draw_scale);
        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, dstate.enabled ? drawingScale : 0.0f);

        const float VIS_SCALE = 1.0f;

        scene->setVisualizationParameter(PxVisualizationParameter::eWORLD_AXES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_world_axes));
        scene->setVisualizationParameter(PxVisualizationParameter::eBODY_AXES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_body_axes));
        scene->setVisualizationParameter(PxVisualizationParameter::eBODY_MASS_AXES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_body_mass_axes));
        scene->setVisualizationParameter(PxVisualizationParameter::eBODY_LIN_VELOCITY, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_body_lin_velocity));
        scene->setVisualizationParameter(PxVisualizationParameter::eBODY_ANG_VELOCITY, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_body_ang_velocity));
        scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_contact_point));
        scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_contact_normal));
        scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_ERROR, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_contact_error));
        scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_contact_force));
        scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_actor_axes));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_aabbs));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_shapes));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AXES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_axes));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_COMPOUNDS, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_compounds));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_FNORMALS, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_fnormals));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_EDGES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_edges));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_STATIC, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_static));
        scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_collision_dynamic));
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_joint_local_frames));
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_joint_limits));
        scene->setVisualizationParameter(PxVisualizationParameter::eCULL_BOX, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_cull_box));
        scene->setVisualizationParameter(PxVisualizationParameter::eMBP_REGIONS, VIS_SCALE * (bool)Core::CVarReadInt(cl_physics_draw_mbp_regions));
    }
}

//--------------------------------------------------------------------------
/**
*/
void
EnableDebugDrawing(bool enabled)
{
    dstate.enabled = enabled;
    UpdateDebugDrawingParameters();
}

//--------------------------------------------------------------------------
/**
*/
void
ApplyBounds(PxBounds3 const& box)
{
    for (auto& scene : Physics::state.activeScenes)
    {
        scene.scene->setVisualizationCullingBox(box);
    }
}

//--------------------------------------------------------------------------
/**
*/
void
DebugVisualizeScene(PxScene* scene)
{
    if (scene == nullptr)
        return;

    const PxRenderBuffer& rb = scene->getRenderBuffer();

    const float DRAW_POINT_SIZE = Core::CVarReadFloat(cl_physics_draw_scale_points);
    const float DRAW_LINE_WIDTH = Core::CVarReadFloat(cl_physics_draw_scale_lines);
    const float DRAW_TRI_WIDTH = Core::CVarReadFloat(cl_physics_draw_scale_triangles);

    if (dstate.drawInterface.DrawPoint != nullptr)
    {
        for(PxU32 i=0; i < rb.getNbPoints(); i++)
        {
            const PxDebugPoint& point = rb.getPoints()[i];
            dstate.drawInterface.DrawPoint(Px2NebVec(point.pos), Util::Color::ARGB(point.color), DRAW_POINT_SIZE);
        }
    }

    if (dstate.drawInterface.DrawLine != nullptr)
    {
        for(PxU32 i=0; i < rb.getNbLines(); i++)
        {
            const PxDebugLine& line = rb.getLines()[i];
            dstate.drawInterface.DrawLine(Px2NebVec(line.pos0), Px2NebVec(line.pos1), Util::Color::ARGB(line.color0), Util::Color::ARGB(line.color1), DRAW_LINE_WIDTH);
        }
    }

    if (dstate.drawInterface.DrawTriangle != nullptr)
    {
        for(PxU32 i=0; i < rb.getNbTriangles(); i++)
        {
            const PxDebugTriangle& tri = rb.getTriangles()[i];
            dstate.drawInterface.DrawTriangle(Px2NebVec(tri.pos0), Px2NebVec(tri.pos1), Px2NebVec(tri.pos2), Util::Color::ARGB(tri.color0), Util::Color::ARGB(tri.color1), Util::Color::ARGB(tri.color2), DRAW_TRI_WIDTH);
        }
    }
}

//--------------------------------------------------------------------------
/**
*/
bool
CheckAndResetModified(Core::CVar* cvar)
{
    bool modified = Core::CVarModified(cvar);
    Core::CVarSetModified(cvar, false);
    return modified;
}

//--------------------------------------------------------------------------
/**
*/
void
DrawPhysicsDebug()
{
    bool modified = false;
    modified |= CheckAndResetModified(cl_physics_draw_scale);
    modified |= CheckAndResetModified(cl_physics_draw_scale_points);
    modified |= CheckAndResetModified(cl_physics_draw_scale_lines);
    modified |= CheckAndResetModified(cl_physics_draw_scale_triangles);
    modified |= CheckAndResetModified(cl_physics_draw_world_axes);
    modified |= CheckAndResetModified(cl_physics_draw_body_axes);
    modified |= CheckAndResetModified(cl_physics_draw_body_mass_axes);
    modified |= CheckAndResetModified(cl_physics_draw_body_lin_velocity);
    modified |= CheckAndResetModified(cl_physics_draw_body_ang_velocity);
    modified |= CheckAndResetModified(cl_physics_draw_contact_point);
    modified |= CheckAndResetModified(cl_physics_draw_contact_normal);
    modified |= CheckAndResetModified(cl_physics_draw_contact_error);
    modified |= CheckAndResetModified(cl_physics_draw_contact_force);
    modified |= CheckAndResetModified(cl_physics_draw_actor_axes);
    modified |= CheckAndResetModified(cl_physics_draw_collision_aabbs);
    modified |= CheckAndResetModified(cl_physics_draw_collision_shapes);
    modified |= CheckAndResetModified(cl_physics_draw_collision_axes);
    modified |= CheckAndResetModified(cl_physics_draw_collision_compounds);
    modified |= CheckAndResetModified(cl_physics_draw_collision_fnormals);
    modified |= CheckAndResetModified(cl_physics_draw_collision_edges);
    modified |= CheckAndResetModified(cl_physics_draw_collision_static);
    modified |= CheckAndResetModified(cl_physics_draw_collision_dynamic);
    modified |= CheckAndResetModified(cl_physics_draw_joint_local_frames);
    modified |= CheckAndResetModified(cl_physics_draw_joint_limits);
    modified |= CheckAndResetModified(cl_physics_draw_cull_box);
    modified |= CheckAndResetModified(cl_physics_draw_mbp_regions);

    if (modified)
    {
        UpdateDebugDrawingParameters();
    }

    for (auto& scene : Physics::state.activeScenes)
    {
        DebugVisualizeScene(scene.scene);
    }
}

//--------------------------------------------------------------------------
/**
*/
void
RenderUI(Math::mat4 const& cameraViewTransform)
{
    RenderMaterialsUI();
    ImGui::Separator();
    if (ImGui::Checkbox("Draw physics visualization", &dstate.enabled))
    {
        Core::CVarWriteInt(cl_debug_draw_physics, (int)dstate.enabled);
        UpdateDebugDrawingParameters();
    }
    if(dstate.enabled)
    {
        static float viewRange = 50.0f;
        ImGui::PushItemWidth(100.0f);
        ImGui::DragFloat("Viewing Distance", &viewRange, 1.0f, 1.0f, 100.0f);
        ImGui::PopItemWidth();
        Math::mat4 trans =  Math::inverse(cameraViewTransform);

        Math::point center = trans.get_w() - trans.get_z() * viewRange;
        PxBounds3 bound = PxBounds3::centerExtents(Neb2PxPnt(center), PxVec3(viewRange));
        ApplyBounds(bound);

        ImGui::BeginChild("Debug Flags");
        ImGui::PushItemWidth(100.0f);
        float drawingScale = Core::CVarReadFloat(cl_physics_draw_scale);
        if (ImGui::DragFloat("Scale", &drawingScale, 0.1f, 0.01f, 10.0f))
        {
            Core::CVarWriteFloat(cl_physics_draw_scale, drawingScale);
        }
        ImGui::PopItemWidth();

        auto ParameterCheckBox = [](const char* label, Core::CVar* cvar) {
            bool value = (bool)Core::CVarReadInt(cvar);
            if (ImGui::Checkbox(label, &value))
            {
                Core::CVarWriteInt(cvar, (int)value);
                UpdateDebugDrawingParameters();
            }
        };

        ParameterCheckBox("World axes", cl_physics_draw_world_axes);
        ParameterCheckBox("Body axes", cl_physics_draw_body_axes);
        ParameterCheckBox("Body mass axes", cl_physics_draw_body_mass_axes);
        ParameterCheckBox("Body lin velocity", cl_physics_draw_body_lin_velocity);
        ParameterCheckBox("Body ang velocity", cl_physics_draw_body_ang_velocity);
        ParameterCheckBox("Contact point", cl_physics_draw_contact_point);
        ParameterCheckBox("Contact normal", cl_physics_draw_contact_normal);
        ParameterCheckBox("Contact error", cl_physics_draw_contact_error);
        ParameterCheckBox("Contact force", cl_physics_draw_contact_force);
        ParameterCheckBox("Actor axes", cl_physics_draw_actor_axes);
        ParameterCheckBox("Collision aabbs", cl_physics_draw_collision_aabbs);
        ParameterCheckBox("Collision shapes", cl_physics_draw_collision_shapes);
        ParameterCheckBox("Collision axes", cl_physics_draw_collision_axes);
        ParameterCheckBox("Collision compounds", cl_physics_draw_collision_compounds);
        ParameterCheckBox("Collision fnormals", cl_physics_draw_collision_fnormals);
        ParameterCheckBox("Collision edges", cl_physics_draw_collision_edges);
        ParameterCheckBox("Collision static", cl_physics_draw_collision_static);
        ParameterCheckBox("Collision dynamic", cl_physics_draw_collision_dynamic);
        ParameterCheckBox("Joint local frames", cl_physics_draw_joint_local_frames);
        ParameterCheckBox("Joint limits", cl_physics_draw_joint_limits);
        ParameterCheckBox("Cull box", cl_physics_draw_cull_box);
        ParameterCheckBox("Mbp regions", cl_physics_draw_mbp_regions);

        ImGui::EndChild();
    }
}

//--------------------------------------------------------------------------
/**
*/
void
RenderMaterialsUI()
{
    ImGui::Text("Physics Materials");
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
                SetPhysicsMaterial(name.AsCharPtr(),0.5f,0.5f,0.5f,1.0f);
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
}

}
