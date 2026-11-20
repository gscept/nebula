#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetImporter

    Asset importing is a set of functions used to import external files to Nebula.
    type of external asset that Nebula supports, image files (png, tga, etc), model files (fbx, gltf, etc) or audio files (wav, ogg, etc).

    The output is a Nebula defined asset, which can be used to package the asset into a binary format which the engine can load at runtime.
    These output assets are supposed to be submitted with version control such that other developers can use them without needing to have the original source files.

    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/uri.h"
#include "io/path.h"
#include "util/fixedarray.h"
#include "toolkitutil/base/exporttypes.h"
#include "flat/texture.h"

namespace ToolkitUtil
{
class Logger;

/// Import an FBX
bool ImportFBX(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger, bool force = true);
// Import a GLTF or GLB
bool ImportGLTF(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger, bool force = true);
/// Check if a model file (fbx, gltf or glb) has skinned meshes. Skinned meshes can't have vertex colors or secondary UVs,
/// there's no vertex layout combining them, so they shouldn't be imported with them
bool ModelSourceIsSkinned(const IO::URI& file);
/// Import a model asset (folder:file:mdl) again from the file it was imported from, with the flags and scale it was imported with.
/// The scene is generated again, keeping the materials assigned to its nodes. Assets from before the source was recorded are
/// looked for as src:source/folder/file.fbx, .gltf or .glb
bool ReimportModel(const IO::Path& asset, ToolkitUtil::Logger* logger, Util::String& outError);
/// Get the file a model asset (folder:file:mdl) was imported from, see ReimportModel. Fails if there's none, or importing it would create a
/// differently named asset because the asset was renamed
bool GetModelSource(const IO::Path& asset, IO::URI& outSource, Util::String& outError);
/// Get the images a texture asset (folder:file:tex) was imported from, one, or the six faces of a cube (+X, -X, +Y, -Y, +Z, -Z).
/// Textures from before the source was recorded are looked for as src:source/folder/file.png, .jpg, .tga and the other image types
bool GetTextureSource(const IO::Path& asset, Util::Array<IO::URI>& outFiles, Util::String& outError);
/// Give the shapes of a model asset (folder:file:mdl) that importing gave a placeholder material, as it does for FBX materials and glTF
/// primitives without one, a material (folder:file:mat). Others, like physics shapes and materials kept from before, stay as they are.
/// outAssignedSkins is how many of the outAssigned shapes are skinned
bool AssignPlaceholderMaterials(const IO::Path& asset, const Util::String& material, SizeT& outAssigned, SizeT& outAssignedSkins, Util::String& outError);
/// Get how an asset refers to the file it was imported from, src: relative when it's in the project so it survives moving the project
Util::String GetSourceReference(const IO::URI& file);
/// Get the folder in src:source the source of an asset belongs in, the one matching the asset folder (folder:: is src:source/folder)
IO::URI GetSourceFolder(const IO::Path& assetFolder);
/// Check if a file is in src:source already
bool IsInSourceFolder(const IO::URI& file);
/// Copy the file an asset is imported from into a folder, a gltf along with the buffers and images it references. outFile is the copy,
/// which can be imported so the asset references it as its source
bool CopyImportSource(const IO::URI& file, const IO::URI& destinationFolder, IO::URI& outFile, Util::String& outError);
/// Setup a texture resource based on file name
ToolkitUtil::TextureResourceT SetupTextureImportSettingsFromPath(const IO::URI& file);
/// Get the cube face (0-5 for +X, -X, +Y, -Y, +Z, -Z) an image is named after (sky_px.png, sky_negx.hdr, right.jpg, ...), or InvalidIndex
IndexT GetCubeFaceFromPath(const IO::URI& file, Util::String& cubeName);
/// Import a texture, a single image or a cross/strip image of a cube texture (see texture.cube_layout)
bool ImportTexture(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::TextureResourceT& texture, ToolkitUtil::Logger* logger);
/// Import six images (+X, -X, +Y, -Y, +Z, -Z) as the faces of a cube texture
bool ImportCubeTexture(const Util::FixedArray<IO::URI>& faces, const IO::URI& destinationFolder, const Util::String& name, ToolkitUtil::TextureResourceT& texture, ToolkitUtil::Logger* logger);
/// Import an audio file
bool ImportAudio(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Logger* logger);

}