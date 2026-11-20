#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AssetPackager

    Asset packaging converts assets in Nebula to their binary format, which can be loaded at runtime by the engine.


    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/uri.h"
#include "util/array.h"
#include "util/string.h"
#include "toolkitutil/platform.h"
#include <memory>
#include <vector>

namespace ToolkitUtil
{

struct SceneResourceT;
struct AnimResourceT;
struct AnimClipSectionT;
struct AnimClipEventT;
struct SkeletonResourceT;
struct JointMaskAssetT;
struct MeshResourceT;
struct TextureResourceT;
struct AudioResourceT;
struct MaterialResourceT;
struct ParticleResourceT;
struct PhysicsResourceT;
class Logger;


/// Package model from memory
/// Get the names of the nodes the skin fragments of a model are packaged as, in the order of its skins. Each fragment is a node of
/// its own, named after its shape, the fragments a shape is split into are numbered
Util::Array<Util::String> GetSkinNodeNames(const ToolkitUtil::SceneResourceT* model);
bool PackageModel(const ToolkitUtil::SceneResourceT* model, const Util::String& fileName, const IO::URI& destinationFolder, ToolkitUtil::Platform::Code platform, ToolkitUtil::Logger* logger);
/// Check that a clip section can be cut from an animation: it has a name a clip can have that no other clip has,
/// the clip it's cut from exists, and its range is within that clip. Returns false with the reason otherwise
bool ValidateClipSection(const ToolkitUtil::AnimResourceT* anim, const ToolkitUtil::AnimClipSectionT& section, Util::String& outError);
/// Cut the sections of the animation's clips into clips of their own, which are added after the clips of the animation
/// the clip is in. Sections that can't be cut are skipped with a warning
void CutClipSections(ToolkitUtil::AnimResourceT* anim, const std::vector<std::unique_ptr<ToolkitUtil::AnimClipSectionT>>& sections, ToolkitUtil::Logger* logger);
/// Check that an event can be added to a clip of an animation, or to the clip a section is cut into: its name and
/// category fit the nax file, and it's within the clip. Returns false with the reason otherwise
bool ValidateClipEvent(const ToolkitUtil::AnimResourceT* anim, const std::vector<std::unique_ptr<ToolkitUtil::AnimClipSectionT>>& sections, const ToolkitUtil::AnimClipEventT& event, Util::String& outError);
/// Add the events to the clips, cut the sections and add the events of the sections' clips. Events of a clip that are
/// in a section are in the section's clip as well. What can't be added or cut is skipped with a warning
void BuildClips(ToolkitUtil::AnimResourceT* anim, const std::vector<std::unique_ptr<ToolkitUtil::AnimClipSectionT>>& sections, const std::vector<std::unique_ptr<ToolkitUtil::AnimClipEventT>>& events, ToolkitUtil::Logger* logger);
/// Check that the joint mask at the index can be packaged: there's a skeleton, and the mask has a name no mask before it
/// in the list has. Returns false with the reason otherwise
bool ValidateJointMask(const ToolkitUtil::SkeletonResourceT* skeleton, const std::vector<std::unique_ptr<ToolkitUtil::JointMaskAssetT>>& masks, IndexT index, Util::String& outError);
/// Set the joint masks of the scene from the masks of the model asset, with a weight per joint of the first skeleton.
/// Masks without a name or with one used before are skipped, joints not in the skeleton are left out, with warnings
void BuildJointMasks(ToolkitUtil::SceneResourceT* scene, const ToolkitUtil::SkeletonResourceT* skeleton, const std::vector<std::unique_ptr<ToolkitUtil::JointMaskAssetT>>& masks, ToolkitUtil::Logger* logger);
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