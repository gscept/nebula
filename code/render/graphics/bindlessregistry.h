#pragma once
//------------------------------------------------------------------------------
/**
	@class Graphics::BindlessRegistry

	Handles binding of bindless resources

	@copyright
	(C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Graphics
{

struct BindlessRegistryCreateInfo
{

};

typedef uint32_t BindlessIndex;

/// Create bindless registry
void CreateBindlessRegistry(const BindlessRegistryCreateInfo& info);
/// Destroy bindless registry
void DestroyBindlessRegistry();

/// Register texture
BindlessIndex RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, bool depth = false, bool stencil = false);
/// Reregister texture on the same slot
void ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, BindlessIndex id, bool depth = false, bool stencil = false);
/// Unregister texture
void UnregisterTexture(const BindlessIndex id, const CoreGraphics::TextureType type);

} // namespace Graphics
