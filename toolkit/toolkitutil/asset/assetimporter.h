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

namespace ToolkitUtil
{
class Logger;

/// Import an FBX
bool ImportFBX(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger);
// Import a GLTF or GLB
bool ImportGLTF(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::ImportFlags importFlags, float scale, ToolkitUtil::Logger* logger);
/// Import a texture
bool ImportTexture(const IO::URI& file, const IO::URI& destinationFolder);
/// Import an audio file
bool ImportAudio(const IO::URI& file, const IO::URI& destinationFolder);

}