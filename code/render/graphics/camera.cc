//------------------------------------------------------------------------------
// camera.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "camera.h"

namespace Graphics
{

__ImplementClass(Graphics::Camera, 'CAME', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Camera::Camera()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Camera::~Camera()
{
	// empty
}

} // namespace Graphics