//------------------------------------------------------------------------------
//  @file meshasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshasseteditor.h"
#include "visibility/visibilitycontext.h"
namespace Presentation
{

static int selectedRenderMode = 0;
static Materials::MaterialId SolidMaterial = Materials::InvalidMaterialId, WireframeMaterial = Materials::InvalidMaterialId;

//------------------------------------------------------------------------------
/**
*/
void 
MeshEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    static const char* renderModes[] = { "Fill", "Wireframe" };
    static int newMode = 0;
    ImGui::Combo("Render Mode", &newMode, renderModes, IM_ARRAYSIZE(renderModes));
    if (selectedRenderMode != newMode)
    {
        Materials::MaterialId mat;
        if (newMode == 0)
            mat = SolidMaterial;
        else
            mat = WireframeMaterial;
        Models::ModelContext::ChangeMaterial(item->previewObject, mat);
        selectedRenderMode = newMode;
    }
    assetEditor->viewport.Render();
    SizeT numMeshes = CoreGraphics::MeshResourceGetNumMeshes(item->asset.mesh);
    for (IndexT i = 0; i < numMeshes; i++)
    {
        if (ImGui::CollapsingHeader(Util::Format("Mesh %d", i).AsCharPtr(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            CoreGraphics::MeshId mesh = CoreGraphics::MeshResourceGetMesh(item->asset.mesh, i);
            CoreGraphics::MeshIdLock _0(mesh);

            CoreGraphics::VertexLayoutId vlo = CoreGraphics::MeshGetVertexLayout(mesh);
            ImGui::Text(Util::Format("Vertex Format: %s", CoreGraphics::VertexLayoutGetName(vlo).Value()).AsCharPtr());

            const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
            for (IndexT j = 0; j < groups.Size(); j++)
            {
                ImGui::PushFont(Dynui::ImguiBoldFont);
                ImGui::Text(Util::Format("Primitive Group %d", j).AsCharPtr());
                ImGui::PopFont();

                ImGui::Indent();
                if (groups[j].GetNumIndices() > 0)
                    ImGui::Text(Util::Format("Triangles: %d", groups[j].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList)).AsCharPtr());
                else
                {
                    ImGui::Text(Util::Format("Triangles %d", groups[j].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList)).AsCharPtr());
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 100, 100, 255));
                    ImGui::Text("Primitive Group is not reusing vertices, please check the mesh");
                    ImGui::PopStyleColor();
                }
                ImGui::Unindent();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshSetup(AssetEditorItem* item)
{
    if (SolidMaterial == Materials::InvalidMaterialId)
    {
        SolidMaterial = Materials::CreateMaterial(&MaterialTemplatesGPULang::base::__EditorPreviewFill.entry, "EditorPreviewMeshFill");
        WireframeMaterial = Materials::CreateMaterial(&MaterialTemplatesGPULang::base::__EditorPreviewWireframe.entry, "EditorPreviewMeshWireframe");
    }
    Models::ModelContext::RegisterEntity(item->previewObject);
    Models::ModelContext::Setup(
        item->previewObject,
        Math::mat4(),
        Math::bbox(),
        SolidMaterial,
        CoreGraphics::MeshResourceGetMesh(item->asset.mesh, 0),
        0,
        Graphics::StageMask(1 << 3)
    );
    Models::ModelContext::SetAlwaysVisible(item->previewObject);
    Visibility::ObservableContext::RegisterEntity(item->previewObject);
    Visibility::ObservableContext::Setup(item->previewObject, Visibility::VisibilityEntityType::Model);
}

//------------------------------------------------------------------------------
/**
*/
void
MeshDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
    Models::ModelContext::DeregisterEntity(item->previewObject);
    Visibility::ObservableContext::DeregisterEntity(item->previewObject);
}

//------------------------------------------------------------------------------
/**
*/
void
MeshShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
    Models::ModelContext::SetStageMask(item->previewObject, show ? 1 << 3 : 0x0);
}

} // namespace Presentation
