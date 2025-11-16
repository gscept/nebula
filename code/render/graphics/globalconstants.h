#pragma once
//------------------------------------------------------------------------------
/**
	@class Graphics::GlobalConstants

	Management place for global constants, such as those in the tick/frame groups

	@copyright
	(C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "gpulang/render/system_shaders/shared.h"
namespace Graphics
{

struct GlobalConstantsCreateInfo
{

};

/// Create global constants
void CreateGlobalConstants(const GlobalConstantsCreateInfo& info);
/// Destroy global constants
void DestroyGlobalConstants();

/// Run when starting frame
void AllocateGlobalConstants();

/// Set tick params
void UpdateTickParams(const Shared::PerTickParams::STRUCT& tickParams);
/// Set view params
void UpdateViewConstants(const Shared::ViewConstants::STRUCT& viewConstants);
/// Set camera params
void UpdateShadowConstants(const Shared::ShadowViewConstants::STRUCT& shadowViewConstants);
/// Flush constants by recording update command to command buffer
void FlushUpdates(const CoreGraphics::CmdBufferId buf, const CoreGraphics::QueueType queue);

/// Get frame constant offsets
void GetOffsets(uint64_t& tickOffset, uint64_t& viewOffset, uint64_t& shadowOffset);

/// Get tick params constant buffer
const Shared::PerTickParams::STRUCT& GetTickParams();
/// Get current view constants
const Shared::ViewConstants::STRUCT& GetViewConstants();
/// Get current shadow view constnats
const Shared::ShadowViewConstants::STRUCT& GetShadowViewConstants();

/// Set global irradiance and cubemaps
void SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips);
/// Setup gbuffer bindings
void SetupBufferConstants();

/// Get per-tick resource table for graphics
const CoreGraphics::ResourceTableId GetFrameResourceTable(uint32_t bufferIndex);

/// Get per-tick resource table for graphics
const CoreGraphics::ResourceTableId GetTickResourceTable(uint32_t bufferIndex);
}
