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
};

struct ModelImport
{
    Util::String file;
    ModelImportSettings settings;
    IO::Path destination;                   // folder in work:assets
};

static Util::Array<TextureImport> TextureResources;
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
    Destination folder on its own line, filling the window width, with the file it imports to
    below it. Returns true if the folder was picked, or typed and the field was left, so
    settings of an existing asset can be loaded.
*/
static bool
DestinationField(const Util::String& requester, IO::Path& destination, const Util::String& fileName, IO::Path& lastDestination)
{
    bool changed = false;
    if (FolderPicked && FolderPickRequester == requester)
    {
        destination = PickedFolder;
        lastDestination = destination;
        FolderPicked = false;
        changed = true;
    }

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

    Util::String target = destination.GetFolder().IsEmpty()
        ? Util::Format("assets/%s", fileName.AsCharPtr())
        : Util::Format("assets/%s/%s", destination.GetFolder().AsCharPtr(), fileName.AsCharPtr());
    ImGui::TextDisabled("Imports to");
    ImGui::SameLine();
    ImGui::TextUnformatted(target.AsCharPtr());
    if (!IO::IoServer::Instance()->DirectoryExists(destination.WorkURI("assets")))
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "(new folder)");
    }
    ImGui::Separator();
    return changed;
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetImporterWindow::Run(SaveMode save)
{
    static Util::Array<Util::String> Files;
    static IO::Path Destination;

    const auto loadAssetFlags = [](IO::Path& path, const Util::String& file, Presentation::ModelImportSettings& settings)
    {
        // Load potential asset settings
        IO::Path oldAssetPath = path;
        oldAssetPath.SetFile(file.ExtractFileName(), "mdl");
        IO::URI oldAssetURI = oldAssetPath.WorkURI("assets");
        if (IO::FileExists(oldAssetURI))
        {
            Ptr<IO::Stream> stream = IO::CreateStream(oldAssetURI);
            if (stream->Open())
            {
                void* data = stream->MemoryMap();
                SizeT size = stream->GetSize();

                // Do cheap reading, to avoid deserializing the whole mesh and packaged texture assets
                const ToolkitUtil::ModelAsset* modelAsset = flatbuffers::GetRoot<ToolkitUtil::ModelAsset>((const uint8_t*)data);
                settings.removeRedundantVertices = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_RemoveRedundantVertices);
                settings.calculateNormals = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateNormals);
                settings.flipUVs = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_FlipUVs);
                settings.calculateTangents = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateTangentFrame);
                settings.calculateRigidSkin = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_CalculateRigidSkinning);
                settings.importColors = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_ImportVertexColors);
                settings.importSecondaryUVs = AllBits(modelAsset->flags(), ToolkitUtil::OptimizationFlags::OptimizationFlags_ImportSecondaryUVs);

                stream->MemoryUnmap();
                stream->Close();

                return true;
            }
        }
        return false;
    };

    const auto loadTextureSettings = [](IO::Path& path, const Util::String& name, ToolkitUtil::TextureResourceT& settings)
    {
        // Load potential asset settings
        IO::Path oldAssetPath = IO::Path::File(path.GetFolderAndFile(), name, "tex");
        IO::URI oldAssetURI = oldAssetPath.WorkURI("assets");
        if (IO::FileExists(oldAssetURI))
        {
            Ptr<IO::Stream> stream = IO::CreateStream(oldAssetURI);
            if (stream->Open())
            {
                void* data = stream->MemoryMap();
                SizeT size = stream->GetSize();

                const ToolkitUtil::TextureResource* textureAsset = flatbuffers::GetRoot<ToolkitUtil::TextureResource>((const uint8_t*)data);
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
        }
        return false;
    };

    auto console = (Presentation::Console*)Presentation::ConsoleWindow;
    static ToolkitUtil::Logger logger;

    Util::Array<Util::String> images;
    for (const auto& file : Dynui::ImguiDragAndDropFiles)
    {
        static Util::String ext;
        ext = file.GetFileExtension();

        if (ext == "fbx")
        {
            ModelImportSettings fbxSettings;
            fbxSettings.replaceExistingMesh = true;
            loadAssetFlags(Destination, file, fbxSettings);
            FbxFiles.Append({ file, fbxSettings, Destination });
        }
        else if (ext == "gltf" || ext == "glb")
        {
            ModelImportSettings gltfSettings;
            gltfSettings.replaceExistingMesh = true;
            loadAssetFlags(Destination, file, gltfSettings);
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
        loadTextureSettings(import.destination, import.name, import.settings);
        TextureResources.Append(import);
    }
    auto batcher = (Editor::LiveBatcher*)Presentation::BatcherWindow;
    // imported entries are erased after the loops, erasing inside a range-for steps past the shrunk array
    IndexT textureResourceIndex = 0, importedTexture = InvalidIndex;
    for (auto& [file, name, faces, textureResource, texture, imguiId, resId, previewFace, destination, exposureStops, previewMip] : TextureResources)
    {
        Util::String title = faces.IsEmpty()
            ? Util::Format("Import Texture: %s", file.ExtractFileName().AsCharPtr())
            : Util::Format("Import Cube Texture: %s", name.AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {
            const Util::String fileNameNoExt = name;
            // an existing texture in the new folder brings its settings along
            bool settingsLoaded = false;
            if (DestinationField(title, destination, fileNameNoExt + ".natex", Destination))
                settingsLoaded = loadTextureSettings(destination, name, textureResource);

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
                            resId = Resources::CreateResource(
                                outputFileName,
                                "editor",
                                [&resId, &texture, imguiId, exposure = Math::exp2(exposureStops)](Resources::ResourceId id)
                            {
                                resId = id;
                                texture = id.resourceId;
                                Dynui::ImguiTextureId tex;
                                tex.nebulaHandle = texture;
                                tex.tonemap = 1;
                                tex.exposure = exposure;
                                Dynui::SetImguiTextureIdData(imguiId, tex);
                            });
                            texture = resId.resourceId;
                        }

                        sourceStream->Close();
                    }
                }

                Util::String saveButton =
                    Util::String::Sprintf("%s Import %s", ICON_ttf_SAVE, fileNameNoExt.AsCharPtr());
                if (ImGui::Button(saveButton.AsCharPtr()))
                {
                    IO::IoServer::Instance()->CreateDirectory(destination.WorkURI("assets"));
                    if (faces.IsEmpty())
                    {
                        ToolkitUtil::ImportTexture(file, destination.WorkURI("assets"), textureResource, &logger);
                    }
                    else
                    {
                        Util::FixedArray<IO::URI> faceFiles(6);
                        for (IndexT i = 0; i < 6; i++)
                            faceFiles[i] = faces[i];
                        ToolkitUtil::ImportCubeTexture(faceFiles, destination.WorkURI("assets"), fileNameNoExt, textureResource, &logger);
                    }
                    importedTexture = textureResourceIndex;
                    auto workAsset = IO::Path::File(destination.GetFolder(), fileNameNoExt, "tex");
                    batcher->BatchFile(workAsset);
                }

                ImGui::EndTable();
            } // End of table
        }
        // End() has to match every Begin(), also for collapsed windows
        ImGui::End();
        textureResourceIndex++;
    }
    if (importedTexture != InvalidIndex)
        TextureResources.EraseIndex(importedTexture);


    IndexT fbxFileIndex = 0, importedFbx = InvalidIndex;
    for (auto& [file, settings, destination] : FbxFiles)
    {
        Util::String title = Util::Format("Import FBX: %s", file.ExtractFileName().AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {
            Util::String fileNameNoExt = file.ExtractFileName();
            fileNameNoExt.StripFileExtension();
            if (DestinationField(title, destination, fileNameNoExt + ".nasset", Destination))
                loadAssetFlags(destination, file, settings);

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");

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
                IO::IoServer::Instance()->CreateDirectory(destination.WorkURI("assets"));
                ToolkitUtil::ImportFBX(file, destination.WorkURI("assets"), (ToolkitUtil::ImportFlags)flags, scale, &logger);
                IO::Path asset = destination;
                asset.SetFile(fileNameNoExt, "mdl");
                batcher->BatchAsset(asset);
                importedFbx = fbxFileIndex;
            }
        }
        ImGui::End();
        fbxFileIndex++;
    }
    if (importedFbx != InvalidIndex)
        FbxFiles.EraseIndex(importedFbx);

    IndexT gltfFileIndex = 0, importedGltf = InvalidIndex;
    for (auto& [file, settings, destination] : GltfFiles)
    {
        Util::String title = Util::Format("Import GLTF: %s", file.ExtractFileName().AsCharPtr());
        if (ImGui::Begin(title.AsCharPtr()))
        {
            Util::String fileNameNoExt = file.ExtractFileName();
            fileNameNoExt.StripFileExtension();
            if (DestinationField(title, destination, fileNameNoExt + ".nasset", Destination))
                loadAssetFlags(destination, file, settings);

            ImGui::Checkbox("Replace existing mesh", &settings.replaceExistingMesh);
            ImGui::Checkbox("Remove redundant vertices", &settings.removeRedundantVertices);
            ImGui::Checkbox("Calculate normals", &settings.calculateNormals);
            ImGui::Checkbox("Flip UVs", &settings.flipUVs);
            ImGui::Checkbox("Import vertex colors", &settings.importColors);
            ImGui::Checkbox("Import secondary UVs", &settings.importSecondaryUVs);
            ImGui::Checkbox("Calculate tangents", &settings.calculateTangents);
            ImGui::Checkbox("Calculate rigid skinning", &settings.calculateRigidSkin);
            ImGui::Combo("Scale", &settings.scale, "centimeters\000meters\000kilometers\000");

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
                IO::IoServer::Instance()->CreateDirectory(destination.WorkURI("assets"));
                ToolkitUtil::ImportGLTF(file, destination.WorkURI("assets"), (ToolkitUtil::ImportFlags)flags, scale, &logger);
                IO::Path asset = destination;
                asset.SetFile(fileNameNoExt, "mdl");
                batcher->BatchAsset(asset);
                importedGltf = gltfFileIndex;
            }
        }
        ImGui::End();
        gltfFileIndex++;
    }
    if (importedGltf != InvalidIndex)
        GltfFiles.EraseIndex(importedGltf);
    logger.Flush();
}

//------------------------------------------------------------------------------
/**
*/
bool 
AssetImporterWindow::ShouldRun() const
{
    return !TextureResources.IsEmpty() || !GltfFiles.IsEmpty() || !FbxFiles.IsEmpty() || !Dynui::ImguiDragAndDropFiles.IsEmpty();
}

} // namespace Presentation
