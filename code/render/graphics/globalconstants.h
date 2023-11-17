#pragma once
//------------------------------------------------------------------------------
/**
	@class Graphics::GlobalConstants

	Management place for global constants, such as those in the tick/frame groups

	@copyright
	(C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

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
void UpdateTickParams(const Shared::PerTickParams& tickParams);
/// Set view params
void UpdateViewConstants(const Shared::ViewConstants& viewConstants);
/// Set camera params
void UpdateShadowConstants(const Shared::ShadowViewConstants& shadowViewConstants);
/// Flush constants by recording update command to command buffer
void FlushUpdates(const CoreGraphics::CmdBufferId buf);

/// Get tick params constant buffer
const Shared::PerTickParams& GetTickParams();
/// Get current view constants
const Shared::ViewConstants& GetViewConstants();
/// Get current shadow view constnats
const Shared::ShadowViewConstants& GetShadowViewConstants();

/// Set global irradiance and cubemaps
void SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips);
/// Setup gbuffer bindings
void SetupBufferConstants(const Ptr<Frame::FrameScript>& frameScript);

/// Get per-tick resource table for graphics
const CoreGraphics::ResourceTableId GetFrameResourceTable(uint32_t bufferIndex);

/// Get per-tick resource table for graphics
const CoreGraphics::ResourceTableId GetTickResourceTable(uint32_t bufferIndex);
}
