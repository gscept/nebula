//------------------------------------------------------------------------------
//  @file importwindow.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "importwindow.h"
#include "dynui/imguicontext.h"

#include "coregraphics/texture.h"
#include "graphics/view.h"

#include "toolkitutil/asset/assetimporter.h"
#include "toolkitutil/asset/assetpackager.h"
#include "dynui/imguifiledialog/ImGuiFileDialog.h"
#include "editor/ui/windowserver.h"
#include "editor/tools/livebatcher.h"
#include "editor/ui/windows/assetbrowser.h"
#include "editor/ui/windows/asseteditor/materialasseteditor.h"
#include "materials/gpulang/materialtemplatesgpulang.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"

#include "io/ioserver.h"
#include "console.h"

#include "dynui/nebula_icons.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
AssetImporterWindow::AssetImporterWindow()
{
}

//------------------------------------------------------------------------------
/**
*/
AssetImporterWindow::~AssetImporterWindow()
{
}

struct ModelImportSettings
{
    bool removeRedundantVertices = 1;
    bool calculateNormals = 0;
    bool flipUVs = 0;
    bool importColors = 1;
    bool importSecondaryUVs = 1;
    bool calculateTangents = 0;
    bool calculateRigidSkin = 0;
    bool replaceExistingMesh = 0;
    int scale = 1; // 0 means cm, 1 means m, 2 means km
};

struct TextureImport
{
    Util::String file;                      // the image, or the +X face of a cube
    Util::String name;                      // name of the work texture
    Util::FixedArray<Util::String> faces;   // the six images of a cube (+X, -X, +Y, -Y, +Z, -Z), or empty
    ToolkitUtil::TextureResourceT settings;
    CoreGraphics::TextureId texture = CoreGraphics::InvalidTextureId;
    Ids::Id32 imguiId;
    Resources::ResourceId resId = Resources::InvalidResourceId;
    IndexT previewFace = 0;
    IO::Path destination;                   // folder in work:assets
    float exposureStops = 0.0f;             // preview exposure for HDR textures
    int previewMip = 0;
    bool copyToSource = true;               // copy the images to the matching src:source folder and import those, for new textures
    bool reimport = false;                  // importing an existing texture again from its images, the folder can't be changed
    uint previewRequest = 0;                // the preview load the entry waits for, see PreviewRequests
};

struct ModelImport
{
    Util::String file;
    ModelImportSettings settings;
    IO::Path destination;                   // folder in work:assets
    bool copyToSource = true;               // copy the file to the matching src:source folder and import that, for new assets
    bool createMaterial = false;            // create a default material named after the model next to it and use it for its placeholder materials
    bool reimport = false;                  // importing an existing asset again from its source, the folder can't be changed, no material is created
    Util::String materialTemplate;          // template of the material created, empty picks it by the shapes, see CreateModelMaterial
};

static Util::Array<TextureImport> TextureResources;
// preview loads are told apart by a number counted up for each one, the load callback looks its entry up by it. It can't refer
// to the entry, the entry moves when the array grows, and is gone when imported or canceled while the preview still loads
static uint PreviewRequests = 0;
// previews of imported and canceled textures, released the next frame since the frame they're erased in may still draw them
struct RetiredPreview
{
    Ids::Id32 imguiId;
    Resources::ResourceId resId;
};
static Util::Array<RetiredPreview> RetiredPreviews;
static Util::Array<ModelImport> FbxFiles;
static Util::Array<ModelImport> GltfFiles;

// the asset browser calls back later, the folder is handed to the window that asked for it on its next frame
static Util::String FolderPickRequester;
static IO::Path PickedFolder;
static bool FolderPicked = false;

//------------------------------------------------------------------------------
/**
    Turns the typed folder into a folder relative to work:assets, "assets/" and
    empty, "." and ".." parts are dropped
*/
static IO::Path
DestinationFromText(const char* text)
{
    Util::String folder = text;
    folder.ConvertBackslashes();
    Util::Array<Util::String> parts = folder.Tokenize("/");
    Util::String result;
    for (IndexT i = 0; i < parts.Size(); i++)
    {
        Util::String part = parts[i];
        part.Trim(" \t");
        if (part.IsEmpty() || part == "." || part == ".." || (i == 0 && result.IsEmpty() && part == "assets"))
            continue;
        if (!result.IsEmpty())
            result.Append("/");
        result.Append(part);
    }
    return IO::Path::Folder(result);
}

//------------------------------------------------------------------------------
/**
    Load the settings an existing model asset was imported with, returns false if there's no such asset
*/
static bool
LoadModelSettings(const IO::Path& folder, const Util::String& file, ModelImportSettings& settings)
{
    IO::Path oldAssetPath = folder;
    oldAssetPath.SetFile(file.ExtractFileName(), "mdl");
    IO::URI oldAssetURI = oldAssetPath.WorkURI("assets");
    if (!IO::FileExists(oldAssetURI))
        return false;
    Ptr<IO::Stream> stream = IO::CreateStream(oldAssetURI);
    if (!stream->Open())
        return false;

    // Do cheap reading, to avoid deserializing the whole mesh and packaged texture assets
    const ToolkitUtil::ModelAsset* modelAsset = flatbuffers::GetRoot<ToolkitUtil::ModelAsset>((const uint8_t*)stream->MemoryMap());
    settings.removeRedundantVertices = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_RemoveRedundantVertices);
    settings.calculateNormals = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateNormals);
    settings.flipUVs = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_FlipUVs);
    settings.calculateTangents = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateTangentFrame);
    settings.calculateRigidSkin = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateRigidSkinning);
    settings.importColors = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_ImportVertexColors);
    settings.importSecondaryUVs = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_ImportSecondaryUVs);
    // the combo only offers these, assets from before the scale was stored are in meters
    const float scale = modelAsset->scale();
    settings.scale = scale < 0.1f ? 0 : (scale > 10.0f ? 2 : 1);

    stream->MemoryUnmap();
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
    Skinned meshes have no vertex layout with vertex colors or secondary UVs, so a skinned model that's imported
    for the first time doesn't import them by default
*/
static void
SetupSkinDefaults(const Util::String& file, ModelImportSettings& settings)
{
    if (ToolkitUtil::ModelSourceIsSkinned(file))
    {
        settings.importColors = false;
        settings.importSecondaryUVs = false;
    }
}

//------------------------------------------------------------------------------
/**
    Load the settings an existing texture asset was imported with, returns false if there's no such asset
*/
static bool
LoadTextureSettings(const IO::Path& folder, const Util::String& name, ToolkitUtil::TextureResourceT& settings)
{
    IO::Path oldAssetPath = IO::Path::File(folder.GetFolderAndFile(), name, "tex");
    IO::URI oldAssetURI = oldAssetPath.WorkURI("assets");
    if (!IO::FileExists(oldAssetURI))
        return false;
    Ptr<IO::Stream> stream = IO::CreateStream(oldAssetURI);
    if (!stream->Open())
        return false;

    const ToolkitUtil::TextureResource* textureAsset = flatbuffers::GetRoot<ToolkitUtil::TextureResource>((const uint8_t*)stream->MemoryMap());
    settings.target_format = textureAsset->target_format();
    settings.compression_quality = textureAsset->compression_quality();
    settings.generate_mipmaps = textureAsset->generate_mipmaps();
    settings.invert_green = textureAsset->invert_green();
    settings.color_space = textureAsset->color_space();
    // a single image can't turn into six cube faces or the other way around
    if ((textureAsset->cube_layout() == ToolkitUtil::TextureCubeLayout_Faces) == (settings.cube_layout == ToolkitUtil::TextureCubeLayout_Faces))
        settings.cube_layout = textureAsset->cube_layout();

    stream->MemoryUnmap();
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
    Destination folder on its own line, filling the window width, with the file it imports to
    below it. Returns true if the folder was picked, or typed and the field was left, so
    settings of an existing asset can be loaded. A reimport has to go where the asset is, the
    folder is grayed out then.
*/
static bool
DestinationField(const Util::String& requester, IO::Path& destination, const Util::String& fileName, IO::Path& lastDestination, bool editable = true)
{
    bool changed = false;
    if (FolderPicked && FolderPickRequester == requester)
    {
        destination = PickedFolder;
        lastDestination = destination;
        FolderPicked = false;
        changed = true;
    }

    ImGui::BeginDisabled(!editable);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Folder");
    ImGui::SameLine();
    if (ImGui::Button(ICON_ttf_FOLDER_OPEN))
    {
        FolderPickRequester = requester;
        FolderPicked = false;
        auto assetBrowser = (AssetBrowser*)AssetBrowserPickerWindow;
        assetBrowser->PickFolder(destination, [](const IO::Path& path)
        {
            PickedFolder = path;
            FolderPicked = true;
        });
    }
    ImGui::SetItemTooltip("Pick a folder in the asset browser");
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(" assets/");
    ImGui::SameLine(0.0f, 0.0f);

    // refilled every frame, imgui keeps its own copy of the text while the field is edited
    static char buf[256];
    destination.GetFolder().CopyToBuffer(buf, sizeof(buf));
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##destination", "(root folder)", buf, sizeof(buf)))
    {
        destination = DestinationFromText(buf);
        lastDestination = destination;
    }
    changed |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::EndDisabled();

    Util::String target = destination.GetFolder().IsEmpty()
        ? Util::Format("assets/%s", fileName.AsCharPtr())
        : Util::Format("assets/%s/%s", destination.GetFolder().AsCharPtr(), fileName.AsCharPtr());
    ImGui::TextDisabled("Imports to");
    ImGui::SameLine();
    ImGui::TextUnformatted(target.AsCharPtr());
    if (!IO::DirectoryExists(destination.WorkURI("assets")))
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "(new folder)");
    }
    ImGui::Separator();
    return changed;
}

//------------------------------------------------------------------------------
/**
    For an asset new to the folder, the option to copy the files it's imported from to the src:source
    folder matching the asset folder and import the copies, so the asset references files that are in
    the project. An existing asset knows its source already, and files in src:source stay where they are.
    Returns true if the files are to be copied.
*/
static bool
SourceCopyField(const Util::Array<Util::String>& files, const IO::Path& asset, bool& copyToSource)
{
    if (IO::FileExists(asset.WorkURI("assets")))
        return false;

    bool inSource = true;
    for (const Util::String& file : files)
        inSource &= ToolkitUtil::IsInSourceFolder(file);
    bool copy = copyToSource && !inSource;
    ImGui::BeginDisabled(inSource);
    if (ImGui::Checkbox("Copy to source folder", &copy))
        copyToSource = copy;
    ImGui::EndDisabled();
    if (inSource)
    {
        ImGui::SetItemTooltip(files.Size() == 1
            ? "The file is in the source folder already, it's imported from there"
            : "The files are in the source folder already, they're imported from there");
        return false;
    }
    ImGui::SetItemTooltip("Copy to the source folder matching the asset folder and import from there, "
                          "so the source is part of the project and the asset can be imported again from it");
    if (copy)
    {
        const Util::String& folder = asset.GetFolder();
        const Util::String target = folder.IsEmpty() ? Util::String("source/") : Util::Format("source/%s/", folder.AsCharPtr());
        ImGui::SameLine();
        if (files.Size() == 1)
            ImGui::TextDisabled("to %s%s", target.AsCharPtr(), files[0].ExtractFileName().AsCharPtr());
        else
            ImGui::TextDisabled("to %s", target.AsCharPtr());

        const Util::String sourceFolder = ToolkitUtil::GetSourceFolder(asset).LocalPath();
        bool replaces = false;
        for (const Util::String& file : files)
            replaces |= !ToolkitUtil::IsInSourceFolder(file) && IO::FileExists(Util::Format("%s/%s", sourceFolder.AsCharPtr(), file.ExtractFileName().AsCharPtr()));
        if (replaces)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), files.Size() == 1 ? "(replaces the file there)" : "(replaces files there)");
        }
    }
    return copy;
}

//------------------------------------------------------------------------------
/**
    Copy the files an asset is imported from to the source folder matching the asset folder if asked to,
    the files are changed to the copies. Files in the source folder already are left where they are.
    Returns false if they couldn't be copied
*/
static bool
CopyToSourceFolder(bool copy, Util::Array<IO::URI>& files, const IO::Path& asset, ToolkitUtil::Logger& logger)
{
    if (!copy)
        return true;
    const IO::URI sourceFolder = ToolkitUtil::GetSourceFolder(asset);
    Util::Array<IO::URI> copies;
    for (const IO::URI& file : files)
    {
        IO::URI copied = file;
        Util::String error;
        if (!ToolkitUtil::IsInSourceFolder(file) && !ToolkitUtil::CopyImportSource(file, sourceFolder, copied, error))
        {
            logger.Error("Could not copy '%s' to the source folder, nothing is imported: %s\n", file.LocalPath().AsCharPtr(), error.AsCharPtr());
            return false;
        }
        if (copied != file)
            logger.Print("Copied '%s' to '%s'\n", file.LocalPath().AsCharPtr(), copied.LocalPath().AsCharPtr());
        copies.Append(copied);
    }
    files = copies;
    return true;
}

//------------------------------------------------------------------------------
/**
    The material a model's material option creates, named after the model in its folder
*/
static IO::Path
ModelMaterialPath(const IO::Path& model)
{
    return IO::Path::File(model.GetFolder(), model.GetFile(), "mat");
}

//------------------------------------------------------------------------------
/**
    The option to create a default material for a model, named after it in its folder, and use it in
    place of the placeholder materials importing gives the shapes it has no material for. The template
    is picked for a material that doesn't exist yet, an existing one keeps its own
*/
static void
MaterialField(const IO::Path& model, bool& createMaterial, Util::String& materialTemplate)
{
    const IO::Path material = ModelMaterialPath(model);
    ImGui::Checkbox("Create material", &createMaterial);
    ImGui::SetItemTooltip("Create a default material named after the model in its folder and assign it to the shapes "
                          "that would get the placeholder material, all of them for FBX files. Shapes that have a material "
                          "from before keep it, and the material isn't created if no shape needs it");
    if (createMaterial)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("assets/%s", material.GetWorkFile().AsCharPtr());
        if (IO::FileExists(material.WorkURI("assets")))
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "(exists, it's assigned as it is)");
        }
        else
        {
            ImGui::Indent();
            if (ImGui::BeginCombo("Material template", materialTemplate.IsEmpty() ? "Automatic" : materialTemplate.AsCharPtr()))
            {
                if (ImGui::Selectable("Automatic", materialTemplate.IsEmpty()))
                    materialTemplate.Clear();
                ImGui::SetItemTooltip("GLTFSkinned if the shapes getting the material are skinned, GLTF otherwise");
                for (const auto& it : MaterialTemplatesGPULang::Lookup)
                {
                    const MaterialTemplatesGPULang::Entry* entry = it.Value();
                    if (ImGui::Selectable(entry->name, materialTemplate == entry->name))
                        materialTemplate = entry->name;
                }
                ImGui::EndCombo();
            }
            ImGui::Unindent();
        }
    }
}

//------------------------------------------------------------------------------
/**
    Assign the default material of an imported model to the shapes that got a placeholder material, and
    create it unless it exists. Nothing is created when no shape needs it, like when reimporting keeps the
    materials assigned. The model's folder is batched afterwards, which packages the material too.
    Without a template given, it's GLTFSkinned when all the shapes getting it are skinned, GLTF otherwise
*/
static void
CreateModelMaterial(const IO::Path& model, const Util::String& materialTemplate, ToolkitUtil::Logger& logger)
{
    const IO::Path material = ModelMaterialPath(model);
    const IO::URI materialURI = material.WorkURI("assets");

    SizeT assigned = 0, assignedSkins = 0;
    Util::String error;
    if (!ToolkitUtil::AssignPlaceholderMaterials(model, material.AsString(), assigned, assignedSkins, error))
    {
        logger.Error("Could not assign '%s' to '%s': %s\n", material.AsString().AsCharPtr(), model.AsString().AsCharPtr(), error.AsCharPtr());
        return;
    }
    if (assigned == 0)
        return;
    logger.Print("Assigned '%s' to %d shapes of '%s'\n", material.AsString().AsCharPtr(), assigned, model.AsString().AsCharPtr());

    if (!IO::FileExists(materialURI))
    {
        // the skinned template needs the skin weights, shapes without them can't use it, so it's only picked when all are skinned
        Util::String templateName = materialTemplate;
        if (templateName.IsEmpty())
        {
            templateName = assignedSkins == assigned ? "GLTFSkinned" : "GLTF";
            if (assignedSkins > 0 && assignedSkins < assigned)
                logger.Warning("'%s' is shared by skinned and static shapes, it's created with the template GLTF, the skinned ones need another material with GLTFSkinned\n",
                               material.AsString().AsCharPtr());
        }
        Ptr<IO::Stream> stream = IO::CreateStream(materialURI);
        stream->SetAccessMode(IO::Stream::WriteAccess);
        if (!stream->Open())
        {
            logger.Error("Could not create the material '%s'\n", materialURI.LocalPath().AsCharPtr());
            return;
        }
        MaterialNew(stream, materialURI.LocalPath(), templateName);
        stream->Close();
        logger.Print("Created the material '%s' with the template %s\n", materialURI.LocalPath().AsCharPtr(), templateName.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetImporterWindow::Run(SaveMode save)
{
    static Util::Array<Util::String> Files;
    static IO::Path Destination;

    for (const RetiredPreview& preview : RetiredPreviews)
    {
        Dynui::DeallocateImguiTextureId(preview.imguiId);
        if (preview.resId != Resources::InvalidResourceId)
            Resources::DiscardResource(preview.resId);
    }
    RetiredPreviews.Clear();

    auto console = (Presentation::Console*)Presentation::ConsoleWindow;
    static ToolkitUtil::Logger logger;

    // dropped files go to the folder selected in the asset browser, otherwise to the last folder imported to
    if (!Dynui::ImguiDragAndDropFiles.IsEmpty())
    {
        IO::Path selectedFolder;
        auto assetBrowser = (AssetBrowser*)AssetBrowserWindow;
        if (assetBrowser != nullptr && assetBrowser->GetSelectedFolder(selectedFolder))
            Destination = selectedFolder;
    }

    Util::Array<Util::String> images;
    for (const auto& file : Dynui::ImguiDragAndDropFiles)
    {
        static Util::String ext;
        ext = file.GetFileExtension();

        if (ext == "fbx")
        {
            ModelImportSettings fbxSettings;
            fbxSettings.replaceExistingMesh = true;
            if (!LoadModelSettings(Destination, file, fbxSettings))
                SetupSkinDefaults(file, fbxSettings);
            // FBX materials aren't imported, the shapes would all get the placeholder otherwise
            FbxFiles.Append({ file, fbxSettings, Destination, true, true });
        }
        else if (ext == "gltf" || ext == "glb")
        {
            ModelImportSettings gltfSettings;
            gltfSettings.replaceExistingMesh = true;
            if (!LoadModelSettings(Destination, file, gltfSettings))
                SetupSkinDefaults(file, gltfSettings);
            GltfFiles.Append({ file, gltfSettings, Destination });
        }
        else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "dds" || ext == "exr" || ext == "tif" || ext == "tiff" || ext == "cube" || ext == "hdr")
        {
            images.Append(file);
        }
    }

    // six images named after the cube faces (sky_px.png, sky_nx.png, ...) are imported as one cube texture
    Util::Dictionary<Util::String, Util::FixedArray<Util::String>> cubes;
    for (const auto& file : images)
    {
        Util::String cubeName;
        IndexT face = ToolkitUtil::GetCubeFaceFromPath(file, cubeName);
        if (face != InvalidIndex)
        {
            IndexT cube = cubes.FindIndex(file.ExtractDirName() + cubeName);
            if (cube == InvalidIndex)
                cube = cubes.Add(file.ExtractDirName() + cubeName, Util::FixedArray<Util::String>(6));
            cubes.ValueAtIndex(cube)[face] = file;
        }
    }
    for (const auto& file : images)
    {
        Util::String cubeName;
        IndexT face = ToolkitUtil::GetCubeFaceFromPath(file, cubeName);
        const Util::FixedArray<Util::String>* faces = nullptr;
        if (face != InvalidIndex)
        {
            faces = &cubes[file.ExtractDirName() + cubeName];
            if ((*faces)[face] != file || faces->FindIndex("") != InvalidIndex)
                faces = nullptr;
        }

        TextureImport import;
        import.imguiId = Dynui::AllocateImguiTextureId({});
        if (faces == nullptr)
        {
            import.file = file;
            import.name = file.ExtractFileName();
            import.name.StripFileExtension();
        }
        else if (face == 0)
        {
            import.file = (*faces)[0];
            import.name = cubeName;
            import.faces = *faces;
        }
        else
        {
            // the cube is added with its +X face
            continue;
        }
        import.settings = ToolkitUtil::SetupTextureImportSettingsFromPath(import.file);
        if (!import.faces.IsEmpty())
            import.settings.cube_layout = ToolkitUtil::TextureCubeLayout_Faces;
        import.destination = Destination;
        LoadTextureSettings(import.destination, import.name, import.settings);
        TextureResources.Append(import);
    }
    auto batcher = (Editor::LiveBatcher*)Presentation::BatcherWindow;
    // imported and canceled entries are erased after the loops, erasing inside a range-for steps past the shrunk array
    // every pending import is a tab of this window, closing a tab cancels its import
    const bool tabBar = ImGui::BeginTabBar("##Imports", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll);

    IndexT textureResourceIndex = 0, importedTexture = InvalidIndex;
    for (auto& [file, name, faces, textureResource, texture, imguiId, resId, previewFace, destination, exposureStops, previewMip, copyToSource, reimport, previewRequest] : TextureResources)
    {
        Util::String title = faces.IsEmpty()
            ? Util::Format(reimport ? "Reimport Texture: %s" : "Import Texture: %s", file.ExtractFileName().AsCharPtr())
            : Util::Format(reimport ? "Reimport Cube Texture: %s" : "Import Cube Texture: %s", name.AsCharPtr());
        bool keepTab = true;
        if (tabBar && ImGui::BeginTabItem(title.AsCharPtr(), &keepTab))
        {
            const Util::String fileNameNoExt = name;
            // an existing texture in the new folder brings its settings along
            bool settingsLoaded = false;
            if (DestinationField(title, destination, fileNameNoExt + ".natex", Destination, !reimport))
                settingsLoaded = LoadTextureSettings(destination, name, textureResource);

            if (ImGui::BeginTable("TextureImportTable", 2))
            {
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableNextColumn();
                ImTextureRef ref;
                ref._TexID = imguiId;
                ImGui::Image(ref, ImVec2(300, 300));

                // HDR previews are tonemapped, they would show up as white otherwise
                bool previewLoaded = false;
                if (resId != Resources::InvalidResourceId)
                {
                    Resources::Resource::State state = Resources::ResourceServer::Instance()->GetState(resId);
                    previewLoaded = state == Resources::Resource::Loaded || state == Resources::Resource::Reloaded;
                }
                if (previewLoaded)
                {
                    CoreGraphics::TextureIdLock _0(texture);
                    if (CoreGraphics::PixelFormat::IsHighDynamicRange(CoreGraphics::TextureGetPixelFormat(texture)))
                    {
                        ImGui::SetNextItemWidth(300.0f - ImGui::CalcTextSize("Exposure").x - ImGui::GetStyle().ItemInnerSpacing.x);
                        ImGui::SliderFloat("Exposure", &exposureStops, -10.0f, 10.0f, "%.1f EV");
                        ImGui::SetItemTooltip("Preview only, HDR textures are shown tonemapped. Ctrl+click to type a value.");
                    }

                    // the imgui texture id holds 4 bits for the mip, the preview is rebuilt without mips when they are turned off
                    const int mips = Math::min(CoreGraphics::TextureGetNumMips(texture), 16);
                    previewMip = Math::min(previewMip, mips - 1);
                    if (mips > 1)
                    {
                        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(texture);
                        Util::String mipFormat = Util::Format("%%d (%dx%d)", Math::max(dims.width >> previewMip, 1), Math::max(dims.height >> previewMip, 1));
                        ImGui::SetNextItemWidth(300.0f - ImGui::CalcTextSize("Exposure").x - ImGui::GetStyle().ItemInnerSpacing.x);
                        ImGui::SliderInt("Mip", &previewMip, 0, mips - 1, mipFormat.AsCharPtr(), ImGuiSliderFlags_AlwaysClamp);
                    }

                    // the preview gets a fresh imgui texture whenever it's rebuilt, keep exposure and mip on it
                    Dynui::ImguiTextureId tex = Dynui::ImguiTextureIdAllocator.Get<0>(imguiId);
                    const float exposure = Math::exp2(exposureStops);
                    if (tex.exposure != exposure || tex.mip != (uint)previewMip)
                    {
                        tex.exposure = exposure;
                        tex.mip = previewMip;
                        Dynui::SetImguiTextureIdData(imguiId, tex);
                    }
                }
                ImGui::TableNextColumn();
                ImGui::Spacing();

                bool changed = settingsLoaded;

                changed |= ImGui::Checkbox("Generate Mipmaps", &textureResource.generate_mipmaps);
                changed |= ImGui::Checkbox("Invert Green (Y) Channel", &textureResource.invert_green);
                ImGui::SetItemTooltip("Some formats (like BC5) expect the green channel to be inverted. Enable this if your normal "
                                      "maps look wrong after importing.");
                static const char* pixelFormatOptions[] = {
                    "R8G8B8A8",
                    "R8G8B8",
                    "R16",
                    "BC1",
                    "BC2",
                    "BC3",
                    "BC5",
                    "BC6H",
                    "BC7",
                };
                static const char* formatHints[] = {
                    "Import as a typical 8 bit per channel RGBA texture. This is the most widely supported format, but also the "
                    "largest on disk and in memory.",
                    "Import as a typical 8 bit per channel RGB texture. This is widely supported and smaller than RGBA, but doesn't "
                    "support transparency.",
                    "Import as a single channel 16 bit texture. This is useful for height maps or other data that doesn't require "
                    "color information.",
                    "Import using BC1 compression. This is a common compressed format that is widely supported, but doesn't support "
                    "alpha or has limited alpha support.",
                    "Import using BC2 compression. This is a common compressed format that supports alpha, but has limited alpha "
                    "quality.",
                    "Import using BC3 compression. This is a common compressed format that supports alpha with better quality than "
                    "BC2, but is larger on disk.",
                    "Import using BC5 compression. This is a common compressed format for normal maps, but doesn't support color "
                    "information.",
                    "Import using BC6H compression. This is a common compressed format for HDR textures, but doesn't support alpha "
                    "or color information.",
                    "Import using BC7 compression. This is a modern compressed format that supports high quality alpha and color, "
                    "but isn't supported on older hardware.",
                };
                if (ImGui::BeginCombo("Format", pixelFormatOptions[textureResource.target_format]))
                {
                    for (int i = 0; i < ToolkitUtil::TexturePixelFormat_MAX + 1; i++)
                    {
                        bool isSelected = (textureResource.target_format == i);
                        if (ImGui::Selectable(pixelFormatOptions[i], isSelected))
                        {
                            textureResource.target_format = static_cast<ToolkitUtil::TexturePixelFormat>(i);
                            changed = true;
                        }
                        ImGui::SetItemTooltip(formatHints[i]);
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                static const char* colorSpaceOptions[] = {
                    "Linear",
                    "sRGB",
                };
                if (ImGui::BeginCombo("Color space", colorSpaceOptions[textureResource.color_space]))
                {
                    for (int i = 0; i < ToolkitUtil::TextureColorSpace_MAX + 1; i++)
                    {
                        bool isSelected = (textureResource.color_space == i);
                        if (ImGui::Selectable(colorSpaceOptions[i], isSelected))
                        {
                            textureResource.color_space = static_cast<ToolkitUtil::TextureColorSpace>(i);
                            changed = true;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                static const char* compressionQualityOptions[] = {
                    "Low",
                    "High",
                };
                if (ImGui::BeginCombo("Compression quality", compressionQualityOptions[textureResource.compression_quality]))
                {
                    for (int i = 0; i < ToolkitUtil::TextureCompressionQuality_MAX + 1; i++)
                    {
                        bool isSelected = (textureResource.compression_quality == i);
                        if (ImGui::Selectable(compressionQualityOptions[i], isSelected))
                        {
                            textureResource.compression_quality = static_cast<ToolkitUtil::TextureCompressionQuality>(i);
                            changed = true;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (faces.IsEmpty())
                {
                    static const char* cubeLayoutOptions[] = {
                        "None",
                        "Faces",
                        "Horizontal cross",
                        "Vertical cross",
                        "Vertical cross, -Z flipped",
                        "Horizontal tee",
                        "Horizontal strip",
                        "Vertical strip",
                    };
                    if (ImGui::BeginCombo("Cube map", cubeLayoutOptions[textureResource.cube_layout]))
                    {
                        for (int i = 0; i < ToolkitUtil::TextureCubeLayout_MAX + 1; i++)
                        {
                            // six face images are detected by name when dropped (sky_px.png, sky_nx.png, ...)
                            if (i == ToolkitUtil::TextureCubeLayout_Faces)
                                continue;
                            bool isSelected = (textureResource.cube_layout == i);
                            if (ImGui::Selectable(cubeLayoutOptions[i], isSelected))
                            {
                                textureResource.cube_layout = static_cast<ToolkitUtil::TextureCubeLayout>(i);
                            }
                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SetItemTooltip("Import a single image holding all six faces as a cube texture, the faces are cut out when batching.");

                    // texassemble only warns about the aspect ratio and cuts the faces out of the wrong places
                    static const int layoutAspect[][2] = { { 0, 0 }, { 0, 0 }, { 4, 3 }, { 3, 4 }, { 3, 4 }, { 4, 3 }, { 6, 1 }, { 1, 6 } };
                    const int* aspect = layoutAspect[textureResource.cube_layout];
                    if (aspect[0] != 0 && resId != Resources::InvalidResourceId)
                    {
                        Resources::Resource::State state = Resources::ResourceServer::Instance()->GetState(resId);
                        if (state == Resources::Resource::Loaded || state == Resources::Resource::Reloaded)
                        {
                            CoreGraphics::TextureIdLock _0(texture);
                            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(texture);
                            if (dims.width * aspect[1] != dims.height * aspect[0])
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%s needs a %d:%d image, this one is %dx%d",
                                    cubeLayoutOptions[textureResource.cube_layout], aspect[0], aspect[1], dims.width, dims.height);
                            }
                        }
                    }
                }
                else
                {
                    // pick the face to preview, the combos swap faces if the names were matched up wrong
                    static const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
                    ImGui::SeparatorText("Cube faces");
                    for (IndexT i = 0; i < 6; i++)
                    {
                        ImGui::PushID(i);
                        if (ImGui::RadioButton(faceNames[i], previewFace == i))
                        {
                            previewFace = i;
                            changed = true;
                        }
                        ImGui::SetItemTooltip("Preview this face");
                        ImGui::SameLine(ImGui::GetFontSize() * 3.0f);
                        if (ImGui::BeginCombo("##face", faces[i].ExtractFileName().AsCharPtr()))
                        {
                            for (IndexT j = 0; j < 6; j++)
                            {
                                if (ImGui::Selectable(faces[j].ExtractFileName().AsCharPtr(), i == j) && i != j)
                                {
                                    Util::String face = faces[i];
                                    faces[i] = faces[j];
                                    faces[j] = face;
                                    changed |= previewFace == i || previewFace == j;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopID();
                    }
                }

                if (changed || resId == Resources::InvalidResourceId)
                {
                    int hashCode = file.ExtractFileName().HashCode();
                    Util::String outputFileName = Util::Format("temp:importer/%d.dds", hashCode);
                    if (resId != Resources::InvalidResourceId)
                    {
                        Resources::DiscardResource(resId);
                        resId = Resources::InvalidResourceId;
                    }
                    // cube textures are previewed one face or the whole cross at a time, imgui can only show 2D textures
                    const Util::String& previewFile = faces.IsEmpty() ? file : faces[previewFace];
                    Ptr<IO::Stream> sourceStream = IO::CreateStream(previewFile);
                    if (sourceStream->Open())
                    {
                        ToolkitUtil::TextureResourceT preview = textureResource;
                        preview.container = ToolkitUtil::SetupTextureImportSettingsFromPath(previewFile).container;
                        preview.cube_layout = ToolkitUtil::TextureCubeLayout_None;
                        preview.data.resize(sourceStream->GetSize());
                        memcpy(preview.data.data(), sourceStream->MemoryMap(), sourceStream->GetSize());

                        if (ToolkitUtil::PackageTexture(
                            &preview,
                            Util::Format("%d", hashCode),
                            "temp:importer/",
                            ToolkitUtil::Platform::Code::Win32,
                            &logger
                        ))
                        {
                            // an older preview of this entry still loading doesn't match anymore, its callback leaves the entry alone
                            const uint request = ++PreviewRequests;
                            previewRequest = request;
                            resId = Resources::CreateResource(
                                outputFileName,
                                "editor",
                                [request](Resources::ResourceId id)
                            {
                                for (TextureImport& import : TextureResources)
                                {
                                    if (import.previewRequest == request)
                                    {
                                        import.resId = id;
                                        import.texture = id.resourceId;
                                        Dynui::ImguiTextureId tex;
                                        tex.nebulaHandle = import.texture;
                                        tex.tonemap = 1;
                                        tex.exposure = Math::exp2(import.exposureStops);
                                        Dynui::SetImguiTextureIdData(import.imguiId, tex);
                                        break;
                                    }
                                }
                            });
                            texture = resId.resourceId;
                        }

                        sourceStream->Close();
                    }
                }

                const IO::Path workAsset = IO::Path::File(destination.GetFolder(), fileNameNoExt, "tex");
                Util::Array<Util::String> sourceFiles;
                if (faces.IsEmpty())
                    sourceFiles.Append(file);
                else
                    for (const Util::String& face : faces)
                        sourceFiles.Append(face);
                const bool copySource = SourceCopyField(sourceFiles, workAsset, copyToSource);

                Util::String saveButton =
                    Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, fileNameNoExt.AsCharPtr());
                if (ImGui::Button(saveButton.AsCharPtr()))
                {
                    Util::Array<IO::URI> importFiles;
                    for (const Util::String& sourceFile : sourceFiles)
                        importFiles.Append(sourceFile);
                    if (CopyToSourceFolder(copySource, importFiles, workAsset, logger))
                    {
                        IO::CreateDirectory(destination.WorkURI("assets"));
                        if (faces.IsEmpty())
                        {
                            ToolkitUtil::ImportTexture(importFiles[0], destination.WorkURI("assets"), textureResource, &logger);
                        }
                        else
                        {
                            Util::FixedArray<IO::URI> faceFiles(6);
                            for (IndexT i = 0; i < 6; i++)
                                faceFiles[i] = importFiles[i];
                            ToolkitUtil::ImportCubeTexture(faceFiles, destination.WorkURI("assets"), fileNameNoExt, textureResource, &logger);
                        }
                        importedTexture = textureResourceIndex;
                        batcher->BatchFile(workAsset);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                    importedTexture = textureResourceIndex;

                ImGui::EndTable();
            } // End of table
            ImGui::EndTabItem();
        }
        if (!keepTab)
            importedTexture = textureResourceIndex;
        textureResourceIndex++;
    }
    if (importedTexture != InvalidIndex)
    {
        const TextureImport& import = TextureResources[importedTexture];
        RetiredPreviews.Append({ import.imguiId, import.resId });
        TextureResources.EraseIndex(importedTexture);
    }


    IndexT fbxFileIndex = 0, importedFbx = InvalidIndex;
    for (auto& [file, settings, destination, copyToSource, createMaterial, reimport, materialTemplate] : FbxFiles)
    {
        Util::String title = Util::Format(reimport ? "Reimport FBX: %s" : "Import FBX: %s", file.ExtractFileName().AsCharPtr());
        bool keepTab = true;
        if (tabBar && ImGui::BeginTabItem(title.AsCharPtr(), &keepTab))
        {
            Util::String fileNameNoExt = file.ExtractFileName();
            fileNameNoExt.StripFileExtension();
            if (DestinationField(title, destination, fileNameNoExt + ".nasset", Destination, !reimport))
                LoadModelSettings(destination, file, settings);

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");
            IO::Path assetPath = destination;
            assetPath.SetFile(fileNameNoExt, "mdl");
            // a reimport keeps the materials the model has, new shapes get the placeholder
            if (!reimport)
                MaterialField(assetPath, createMaterial, materialTemplate);
            const bool copySource = SourceCopyField({ file }, assetPath, copyToSource);

            Util::String saveButton =
                Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, file.ExtractFileName().AsCharPtr());

            if (ImGui::Button(saveButton.AsCharPtr()))
            {
                uint flags = 0x0;
                flags |= settings.replaceExistingMesh ? ToolkitUtil::ImportFlags::ReplaceExistingMesh : ToolkitUtil::ImportFlags::None;
                flags |= settings.removeRedundantVertices ? ToolkitUtil::ImportFlags::RemoveRedundant : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateNormals ? ToolkitUtil::ImportFlags::CalcNormals : ToolkitUtil::ImportFlags::None;
                flags |= settings.flipUVs ? ToolkitUtil::ImportFlags::FlipUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.importColors ? ToolkitUtil::ImportFlags::ImportColors : ToolkitUtil::ImportFlags::None;
                flags |= settings.importSecondaryUVs ? ToolkitUtil::ImportFlags::ImportSecondaryUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateTangents ? ToolkitUtil::ImportFlags::CalcTangents : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateRigidSkin ? ToolkitUtil::ImportFlags::CalcRigidSkin : ToolkitUtil::ImportFlags::None;
                float scale = 1.0f;
                switch (settings.scale)
                {
                    case 0: scale = 0.01f; break; // cm to m
                    case 1: scale = 1.0f; break; // m to m
                    case 2: scale = 1000.0f; break; // km to m
                }
                Util::Array<IO::URI> importFiles = { file };
                if (CopyToSourceFolder(copySource, importFiles, assetPath, logger))
                {
                    IO::CreateDirectory(destination.WorkURI("assets"));
                    ToolkitUtil::ImportFBX(importFiles[0], destination.WorkURI("assets"), (ToolkitUtil::ImportFlags)flags, scale, &logger);
                    if (createMaterial && !reimport)
                    {
                        CreateModelMaterial(assetPath, materialTemplate, logger);
                    }
                    IO::Path asset = destination;
                    asset.SetFile(fileNameNoExt, "mdl");
                    batcher->BatchAsset(asset);
                    importedFbx = fbxFileIndex;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                importedFbx = fbxFileIndex;
            ImGui::EndTabItem();
        }
        if (!keepTab)
            importedFbx = fbxFileIndex;
        fbxFileIndex++;
    }
    if (importedFbx != InvalidIndex)
        FbxFiles.EraseIndex(importedFbx);

    IndexT gltfFileIndex = 0, importedGltf = InvalidIndex;
    for (auto& [file, settings, destination, copyToSource, createMaterial, reimport, materialTemplate] : GltfFiles)
    {
        Util::String title = Util::Format(reimport ? "Reimport GLTF: %s" : "Import GLTF: %s", file.ExtractFileName().AsCharPtr());
        bool keepTab = true;
        if (tabBar && ImGui::BeginTabItem(title.AsCharPtr(), &keepTab))
        {
            Util::String fileNameNoExt = file.ExtractFileName();
            fileNameNoExt.StripFileExtension();
            if (DestinationField(title, destination, fileNameNoExt + ".nasset", Destination, !reimport))
                LoadModelSettings(destination, file, settings);

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");
            IO::Path assetPath = destination;
            assetPath.SetFile(fileNameNoExt, "mdl");
            // a reimport keeps the materials the model has, new shapes get the placeholder
            if (!reimport)
                MaterialField(assetPath, createMaterial, materialTemplate);
            const bool copySource = SourceCopyField({ file }, assetPath, copyToSource);

            Util::String saveButton =
                Util::String::Sprintf("%s Import %s.nasset", ICON_ttf_SAVE, fileNameNoExt.AsCharPtr());

            if (ImGui::Button(saveButton.AsCharPtr()))
            {
                uint flags = 0x0;
                flags |= settings.replaceExistingMesh ? ToolkitUtil::ImportFlags::ReplaceExistingMesh : ToolkitUtil::ImportFlags::None;
                flags |= settings.removeRedundantVertices ? ToolkitUtil::ImportFlags::RemoveRedundant : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateNormals ? ToolkitUtil::ImportFlags::CalcNormals : ToolkitUtil::ImportFlags::None;
                flags |= settings.flipUVs ? ToolkitUtil::ImportFlags::FlipUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.importColors ? ToolkitUtil::ImportFlags::ImportColors : ToolkitUtil::ImportFlags::None;
                flags |= settings.importSecondaryUVs ? ToolkitUtil::ImportFlags::ImportSecondaryUVs : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateTangents ? ToolkitUtil::ImportFlags::CalcTangents : ToolkitUtil::ImportFlags::None;
                flags |= settings.calculateRigidSkin ? ToolkitUtil::ImportFlags::CalcRigidSkin : ToolkitUtil::ImportFlags::None;
                float scale = 1.0f;
                switch (settings.scale)
                {
                    case 0: scale = 0.01f; break; // cm to m
                    case 1: scale = 1.0f; break; // m to m
                    case 2: scale = 1000.0f; break; // km to m
                }
                Util::Array<IO::URI> importFiles = { file };
                if (CopyToSourceFolder(copySource, importFiles, assetPath, logger))
                {
                    IO::CreateDirectory(destination.WorkURI("assets"));
                    ToolkitUtil::ImportGLTF(importFiles[0], destination.WorkURI("assets"), (ToolkitUtil::ImportFlags)flags, scale, &logger);
                    if (createMaterial && !reimport)
                        CreateModelMaterial(assetPath, materialTemplate, logger);
                    IO::Path asset = destination;
                    asset.SetFile(fileNameNoExt, "mdl");
                    batcher->BatchAsset(asset);
                    importedGltf = gltfFileIndex;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                importedGltf = gltfFileIndex;
            ImGui::EndTabItem();
        }
        if (!keepTab)
            importedGltf = gltfFileIndex;
        gltfFileIndex++;
    }
    if (tabBar)
        ImGui::EndTabBar();
    if (importedGltf != InvalidIndex)
        GltfFiles.EraseIndex(importedGltf);
    logger.Flush();
}

//------------------------------------------------------------------------------
/**
    Add a texture asset to the texture imports, with the images it was imported from and its settings
*/
static bool
ReimportTexture(const IO::Path& asset, Util::String& outError)
{
    Util::Array<IO::URI> files;
    if (!ToolkitUtil::GetTextureSource(asset, files, outError))
        return false;

    const IO::Path folder = IO::Path::Folder(asset.GetFolder());
    for (const TextureImport& import : TextureResources)
    {
        if (import.name == asset.GetFile() && import.destination == folder)
            return true;
    }

    TextureImport import;
    import.imguiId = Dynui::AllocateImguiTextureId({});
    import.file = files[0].LocalPath();
    import.name = asset.GetFile();
    if (files.Size() == 6)
    {
        import.faces = Util::FixedArray<Util::String>(6);
        for (IndexT i = 0; i < 6; i++)
            import.faces[i] = files[i].LocalPath();
    }
    import.settings = ToolkitUtil::SetupTextureImportSettingsFromPath(import.file);
    if (!import.faces.IsEmpty())
        import.settings.cube_layout = ToolkitUtil::TextureCubeLayout_Faces;
    import.destination = folder;
    LoadTextureSettings(folder, import.name, import.settings);
    import.reimport = true;
    TextureResources.Append(import);
    return true;
}

//------------------------------------------------------------------------------
/**
    The settings are the ones the asset was imported with, and the import goes where the asset is
*/
bool
AssetImporterWindow::Reimport(const IO::Path& asset, Util::String& outError)
{
    if (asset.GetType() == "tex")
        return ReimportTexture(asset, outError);

    IO::URI source;
    if (!ToolkitUtil::GetModelSource(asset, source, outError))
        return false;

    const Util::String file = source.LocalPath();
    Util::String ext = file.GetFileExtension();
    ext.ToLower();
    Util::Array<ModelImport>* imports = nullptr;
    if (ext == "fbx")
        imports = &FbxFiles;
    else if (ext == "gltf" || ext == "glb")
        imports = &GltfFiles;
    else
    {
        outError.Format("its source '%s' isn't an fbx, gltf or glb file", file.AsCharPtr());
        return false;
    }

    const IO::Path folder = IO::Path::Folder(asset.GetFolder());
    for (const ModelImport& import : *imports)
    {
        if (import.file == file && import.destination == folder)
            return true;
    }

    ModelImport import;
    import.file = file;
    import.destination = folder;
    import.settings.replaceExistingMesh = true;
    LoadModelSettings(folder, file, import.settings);
    import.reimport = true;
    imports->Append(import);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
AssetImporterWindow::ShouldRun() const
{
    return !TextureResources.IsEmpty() || !GltfFiles.IsEmpty() || !FbxFiles.IsEmpty() || !Dynui::ImguiDragAndDropFiles.IsEmpty();
}

//------------------------------------------------------------------------------
/**
    Run (which reads dropped files and shows the imports) is only called while the window is open, so it opens
    whenever there's something new. Closing it hides the imports, they show again with the next one
*/
void
AssetImporterWindow::Update()
{
    const SizeT pending = TextureResources.Size() + FbxFiles.Size() + GltfFiles.Size();
    if (!Dynui::ImguiDragAndDropFiles.IsEmpty() || pending > this->lastPendingImports)
    {
        this->Open() = true;
        this->Focus();
    }
    this->lastPendingImports = pending;
}

} // namespace Presentation
