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