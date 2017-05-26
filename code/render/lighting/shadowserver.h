#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::ShadowServer
    
    The ShadowServer setups and controls the global aspects of the dynamic
    shadow system.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "lighting/vk/vkshadowserver.h"
namespace Lighting
{
class ShadowServer : public VkShadowServer
{
	__DeclareClass(ShadowServer);
	__DeclareSingleton(ShadowServer);
public:
	/// constructor
	ShadowServer();
	/// destructor
	virtual ~ShadowServer();
};
} // namespace Lighting
#elif __OGL4__
namespace Lighting
{
class ShadowServer : public OGL4ShadowServer
{
	__DeclareClass(ShadowServer);
	__DeclareSingleton(ShadowServer);
public:
	/// constructor
	ShadowServer();
	/// destructor
	virtual ~ShadowServer();
};
} // namespace Lighting
#elif __DX11__
namespace Lighting
{
class ShadowServer : public D3D11ShadowServer
{
	__DeclareClass(ShadowServer);
	__DeclareSingleton(ShadowServer);
public:
	/// constructor
	ShadowServer();
	/// destructor
	virtual ~ShadowServer();
};
} // namespace Lighting
#elif __DX9__
#include "lighting/d3d9/d3d9shadowserver.h"
namespace Lighting
{
class ShadowServer : public D3D9ShadowServer
{
    __DeclareClass(ShadowServer);
    __DeclareSingleton(ShadowServer);
public:
    /// constructor
    ShadowServer();
    /// destructor
    virtual ~ShadowServer();
};
} // namespace Lighting
#elif __WII__
#include "lighting/wii/wiishadowserver.h"
namespace Lighting
{
class ShadowServer : public Wii::WiiShadowServer
{
    __DeclareClass(ShadowServer);
    __DeclareSingleton(ShadowServer);
public:
    /// constructor
    ShadowServer();
    /// destructor
    virtual ~ShadowServer();
};
} // namespace Lighting
#elif __PS3__
#include "lighting/ps3/ps3shadowserver.h"
namespace Lighting
{
class ShadowServer : public PS3::PS3ShadowServer
{
    __DeclareClass(ShadowServer);
    __DeclareSingleton(ShadowServer);
public:
    /// constructor
    ShadowServer();
    /// destructor
    virtual ~ShadowServer();
};
} // namespace Lighting
#else
#error "ShadowServer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
