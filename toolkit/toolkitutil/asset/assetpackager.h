#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetPackager

    Asset packaging converts assets in Nebula to their binary format, which can be loaded at runtime by the engine.


    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/uri.h"
#include "toolkit-common/platform.h"

namespace ToolkitUtil
{
class Logger;

/// Package a model .namdl
bool PackageModel(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package an animation resource .naani
bool PackageAnimation(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a skeleton resource .naske
bool PackageSkeleton(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
// Package a mesh .namsh
bool PackageMesh(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a texture .natex
bool PackageTexture(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package an audio file .naaud
bool PackageAudio(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a material .namat
bool PackageMaterial(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a particle .napat
bool PackageParticle(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a physics .actor
bool PackagePhysics(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);

}