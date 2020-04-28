#pragma once
//------------------------------------------------------------------------------
/**
	Handles cameras

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "camerasettings.h"
#include "math/scalar.h"
namespace Graphics
{

class CameraContext : public Graphics::GraphicsContext
{
	_DeclareContext();
public:

	/// constructor
	CameraContext();
	/// destructor
	virtual ~CameraContext();

	/// create context
	static void Create();

	/// runs before frame is updated
	static void UpdateCameras(const Graphics::FrameContext& ctx);

	/// setup as projection and fov
	static void SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar);
	/// setup as ortographic
	static void SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar);

	/// set transform
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& mat);
	/// get transform
	static const Math::mat4& GetTransform(const Graphics::GraphicsEntityId id);

	/// get projection
	static const Math::mat4& GetProjection(const Graphics::GraphicsEntityId id);
	/// get view-projection
	static const Math::mat4& GetViewProjection(const Graphics::GraphicsEntityId id);
	/// get settings
	static const CameraSettings& GetSettings(const Graphics::GraphicsEntityId id);

	/// called if the window size has changed
	static void OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);

private:

	enum
	{
		Camera_Settings,
		Camera_Projection,
		Camera_View,
		Camera_ViewProjection
	};
	typedef Ids::IdAllocator<
		Graphics::CameraSettings,
		Math::mat4,				// projection
		Math::mat4,				// view-transform
		Math::mat4				// view-projection
	> CameraAllocator;

	static CameraAllocator cameraAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(const Graphics::ContextEntityId id);
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
CameraContext::Alloc()
{
	return cameraAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline void
CameraContext::Dealloc(const Graphics::ContextEntityId id)
{
	cameraAllocator.Dealloc(id.id);
}

} // namespace Graphics
