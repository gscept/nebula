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

struct SceneResourceT;
struct AnimResourceT;
struct SkeletonResourceT;
struct MeshResourceT;
struct TextureResourceT;
struct AudioResourceT;
struct MaterialResourceT;
struct ParticleResourceT;
struct PhysicsResourceT;
class Logger;


/// Package model from memory
bool PackageModel(const ToolkitUtil::SceneResourceT* model, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package animation from memory
bool PackageAnimation(const ToolkitUtil::AnimResourceT* anim, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package skeleton from memory
bool PackageSkeleton(const ToolkitUtil::SkeletonResourceT* skel, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package mesh from memory
bool PackageMesh(const ToolkitUtil::MeshResourceT* mesh, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package texture from memory
bool PackageTexture(const ToolkitUtil::TextureResourceT* tex, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package audio from memory
bool PackageAudio(const ToolkitUtil::AudioResourceT* audio, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package material from memory
bool PackageMaterial(const ToolkitUtil::MaterialResourceT* mat, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package particle from memory
bool PackageParticle(const ToolkitUtil::ParticleResourceT* par, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package physics from memory
bool PackagePhysics(const ToolkitUtil::PhysicsResourceT* phy, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);

/// Package a texture .natex
bool PackageTextureFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package an audio file .naaud
bool PackageAudioFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a material .namat
bool PackageMaterialFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Package a particle .napat
bool PackageParticleFile(const IO::URI& file, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);

}