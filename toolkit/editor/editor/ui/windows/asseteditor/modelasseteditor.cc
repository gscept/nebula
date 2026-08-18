//------------------------------------------------------------------------------
//  @file modelasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelasseteditor.h"
#include "visibility/visibilitycontext.h"
#include "models/nodes/shaderstatenode.h"
#include "editor/ui/windowserver.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"
#include "editor/tools/livebatcher.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
void 
ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    if (ImGui::BeginTable("Model Editor", 2, ImGuiTableFlags_Resizable))
    {
        // Column 1
        {
            ImGui::TableSetupScrollFreeze(2, 1);
            ImGui::TableNextColumn();
            assetEditor->viewport.Render();
        }

        // Column 2
        ImGui::TableNextColumn();

        const auto UpdateMaterial = [item, assetEditor](const char* path, const char* nodeName, const Models::ModelId mdl, Models::ShaderStateNode* resourceNode)
        {
            const IndexT nodeIndex = Models::ModelContext::GetNodeIndex(item->previewObject, nodeName);
            const auto res = IO::URN("mat", path);

            // A bit of duplicated work, but first set the material on the resource level
            resourceNode->SetMaterial(res);

            // Then load the resource and update all models
            Resources::CreateResource(res, "editor", [mdl, nodeIndex](Resources::ResourceId id)
            {
                Models::ModelContext::ChangeMaterialOnModels(mdl, nodeIndex, id);
            });

            ModelEditorItemData* itemData = (ModelEditorItemData*)item->data;

            // Update node
            for (size_t i = 0; i < itemData->asset.scene->shapes.size(); i++)
            {
                auto& shape = itemData->asset.scene->shapes[i];
                if (shape->transform->name == nodeName)
                {
                    shape->material = res.AsString();
                }
            }

            assetEditor->Edit();
        };

        // Create button to change material
        const auto& nodeTable = Models::GetModelNodeTable(item->asset.model);
        for (const auto& nodePair : nodeTable)
        {
            if (nodePair.Value()->GetType() == Models::NodeType::PrimitiveNodeType)
            {
                const auto node = static_cast<Models::ShaderStateNode*>(nodePair.Value());
                const Util::StringAtom& materialName = Materials::MaterialGetName(node->GetMaterial());
                Util::String shorterMaterialName = materialName.Value();
                shorterMaterialName = shorterMaterialName.StripSubstring(IO::URI("mat:").LocalPath());
                ImGui::Text(nodePair.Key().Value());
                ImGui::Text("Material");
                ImGui::SameLine();
                if (ImGui::Button(shorterMaterialName.AsCharPtr()))
                {
                    const Ptr<BaseWindow> window = WindowServer::Instance()->GetWindow("Asset Browser");
                    window->Open();
                    window->Focus();
                }
                if (ImGui::BeginDragDropTarget())
                {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();

                    auto payload = ImGui::AcceptDragDropPayload("resource");
                    if (payload)
                    {
                        const char* string = (const char*)payload->Data;
                        UpdateMaterial(string, nodePair.Key().Value(), item->asset.model, node);
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }
        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelSetup(AssetEditorItem* item)
{
    ModelEditorItemData* itemData = item->allocator.Alloc<ModelEditorItemData>();
    Ptr<IO::Stream> assetFileStream = IO::CreateStream(item->source);
    if (assetFileStream->Open())
    {
        void* data = assetFileStream->MemoryMap();

        ToolkitUtil::ModelAssetT asset;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(asset, (uint8_t*)data);
        itemData->asset = asset;

        assetFileStream->MemoryUnmap();
        assetFileStream->Close();
    }
    item->data = itemData;

    Models::ModelContext::RegisterEntity(item->previewObject);
    Models::ModelContext::Setup(
        item->previewObject,
        item->path.LocalPath(),
        "preview",
        [gid = item->previewObject]()
        {
            Visibility::ObservableContext::RegisterEntity(gid);
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
        },
        1 << 3
    );
    Models::ModelContext::SetAlwaysVisible(item->previewObject);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    ModelEditorItemData* itemData = (ModelEditorItemData*)item->data;

    // Save file replacing the old
    Ptr<IO::Stream> stream = IO::CreateStream(item->source.LocalPath() + Util::String::FromInt((uintptr_t)itemData));
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelAsset>(itemData->asset);
        stream->Write(data.GetPtr(), data.Size());
        stream->Close();

        // A bit safer approach, copy new file to old to replace it, and delete new
        IO::CopyFile(stream->GetURI(), item->source);
        IO::DeleteFile(stream->GetURI());

        // Batch it
        Editor::LiveBatcher::BatchFile(item->source);

        assetEditor->Unedit();
    }
    else
    {
        n_printf("Failed to save asset src:assets/%s.nasset\n", item->source.AsString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
    Models::ModelContext::SetStageMask(item->previewObject, show ? 1 << 3 : 0x0);
}

} // namespace Base
