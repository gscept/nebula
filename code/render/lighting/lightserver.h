#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::LightServer
  
    The light server collects all lights contributing to the scene and
    controls the realtime lighting process.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if (__XBOX360__)
#include "lighting/lightprepass/lightprepassserver.h"
namespace Lighting
{
class LightServer : public Lighting::LightPrePassServer
{
    __DeclareClass(LightServer);
    __DeclareSingleton(LightServer);
public:
    /// constructor
    LightServer();
    /// destructor
    virtual ~LightServer();
};
} // namespace Lighting
#elif __WII__
#include "lighting/wii/wiilightserver.h"
namespace Lighting
{
class LightServer : public Wii::WiiLightServer
{
    __DeclareClass(LightServer);
    __DeclareSingleton(LightServer);
public:
    /// constructor
    LightServer();
    /// destructor
    virtual ~LightServer();
};
} // namespace Lighting
#elif __VULKAN__
#include "lighting/vk/vklightserver.h"
namespace Lighting
{
class LightServer : public Lighting::VkLightServer
{
	__DeclareClass(LightServer);
	__DeclareSingleton(LightServer);
public:
	/// constructor
	LightServer();
	/// destructor
	virtual ~LightServer();
};	
} // namespace Lighting
#elif __OGL4__
#include "lighting/ogl4/ogl4lightserver.h"
namespace Lighting
{
class LightServer : public Lighting::OGL4LightServer
{
	__DeclareClass(LightServer);
	__DeclareSingleton(LightServer);
public:
	/// constructor
	LightServer();
	/// destructor
	virtual ~LightServer();
};	
} // namespace Lighting
#elif __DX11__
#include "lighting/d3d11/d3d11lightserver.h"
namespace Lighting
{
class LightServer : public Lighting::D3D11LightServer
{
	__DeclareClass(LightServer);
	__DeclareSingleton(LightServer);
public:
	/// constructor
	LightServer();
	/// destructor
	virtual ~LightServer();
};	
} // namespace Lighting
#elif __DX9__
#include "lighting/lightprepass/lightprepassserver.h"
namespace Lighting
{
class LightServer : public Lighting::LightPrePassServer
{
    __DeclareClass(LightServer);
    __DeclareSingleton(LightServer);
public:
    /// constructor
    LightServer();
    /// destructor
    virtual ~LightServer();
};
} // namespace Lighting
#else
#error "LightServer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
