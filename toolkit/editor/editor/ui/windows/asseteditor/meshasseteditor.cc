//------------------------------------------------------------------------------
//  @file meshasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshasseteditor.h"
#include "visibility/visibilitycontext.h"
namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
void 
MeshEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
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
    Resources::ResourceId placeholderMat = Resources::CreateResource("syssur:placeholder.sur", "preview", nullptr, nullptr, true, false);
    Models::ModelContext::Setup(
        item->previewObject,
        Math::mat4(),
        Math::bbox(),
        placeholderMat.resourceId,
        CoreGraphics::MeshResourceGetMesh(item->asset.mesh, 0),
        0,
        Graphics::StageMask(1 << 3)
    );
    Visibility::ObservableContext::RegisterEntity(item->previewObject);
    Visibility::ObservableContext::Setup(item->previewObject, Visibility::VisibilityEntityType::Model);

}

} // namespace Presentation
