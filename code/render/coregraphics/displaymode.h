#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::DisplayMode
  
    Describe a fullscreen display mode or window dimensions.
      
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "core/types.h"
#include "coregraphics/pixelformat.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class DisplayMode
{
public:
    /// default constructor
    DisplayMode();
    /// windowed mode constructor
	DisplayMode(uint x, uint y, SizeT w, SizeT h);
    /// fullscreen constructor
	DisplayMode(SizeT w, SizeT h, PixelFormat::Code p);
    /// generic constructor
	DisplayMode(uint x, uint y, SizeT w, SizeT h, PixelFormat::Code p);
    /// equality operator
    bool operator==(const DisplayMode& rhs) const;
    /// inequality operator
    bool operator!=(const DisplayMode& rhs) const;
    /// set x position
    void SetXPos(uint x);
    /// get x position
    uint GetXPos() const;
    /// set y position
    void SetYPos(uint y);
    /// get y position
    uint GetYPos() const;
    /// set width
	void SetWidth(SizeT w);
    /// get width
	SizeT GetWidth() const;
    /// set height
	void SetHeight(SizeT h);
    /// get height
	SizeT GetHeight() const;
    /// set pixel format
    void SetPixelFormat(PixelFormat::Code p);
    /// get pixel format
    PixelFormat::Code GetPixelFormat() const;
    /// set aspect ratio
    void SetAspectRatio(float a);
    /// get aspect ratio
    float GetAspectRatio() const;

#if __OGL4__
	/// set pixel format
	void SetDepthFormat(PixelFormat::Code p) { this->depthFormat = p; }
	/// get pixel format
	PixelFormat::Code GetDepthFormat() const { return this->depthFormat; }
private:
	PixelFormat::Code depthFormat;
public:
#endif

	/// using DirectX 11, the refresh rate exists within the display mode
	void SetRefreshRate( uint refreshRate );
	/// gives you the refresh rate
	uint GetRefreshRate() const;


private:
	
    uint xPos;
    uint yPos;
	SizeT width;
	SizeT height;
	uint refreshRate;
    float aspectRatio;
    PixelFormat::Code pixelFormat;
	
};

//------------------------------------------------------------------------------
/**
*/
inline
DisplayMode::DisplayMode() :
    xPos(0),
    yPos(0),
    width(1024),
    height(768),
	refreshRate(60),
    aspectRatio(4.0f / 3.0f),
	pixelFormat(PixelFormat::SRGBA8)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This constructor is suitable for windowed modes.
*/
inline
DisplayMode::DisplayMode(uint x, uint y, SizeT w, SizeT h) :
    xPos(x),
    yPos(y),
    width(w),
    height(h),
	refreshRate(60),
    aspectRatio(float(w) / float(h)),
	pixelFormat(PixelFormat::SRGBA8)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This constructor is suitable for fullscreen modes.
*/
inline
DisplayMode::DisplayMode(SizeT w, SizeT h, PixelFormat::Code p) :
    xPos(0),
    yPos(0),
    width(w),
    height(h),
	refreshRate(60),
    aspectRatio(float(w) / float(h)),
    pixelFormat(p)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
DisplayMode::DisplayMode(uint x, uint y, SizeT w, SizeT h, PixelFormat::Code p) :
    xPos(x),
    yPos(y),
    width(w),
    height(h),
	refreshRate(60),
    aspectRatio(float(w) / float(h)),
    pixelFormat(p)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
DisplayMode::operator==(const DisplayMode& rhs) const
{
    return ((this->xPos == rhs.xPos) &&
            (this->yPos == rhs.yPos) &&
            (this->width == rhs.width) &&
            (this->height == rhs.height) &&
			(this->refreshRate == rhs.refreshRate) && 
            (this->aspectRatio == rhs.aspectRatio) &&            
            (this->pixelFormat == rhs.pixelFormat));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
DisplayMode::operator!=(const DisplayMode& rhs) const
{
    return !(*this == rhs); 
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetXPos(uint x)
{
    this->xPos = x;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
DisplayMode::GetXPos() const
{
    return this->xPos;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetYPos(uint y)
{
    this->yPos = y;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
DisplayMode::GetYPos() const
{
    return this->yPos;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetWidth(SizeT w)
{
    this->width = w;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
DisplayMode::GetWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetHeight(SizeT h)
{
    this->height = h;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
DisplayMode::GetHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetPixelFormat(PixelFormat::Code p)
{
    this->pixelFormat = p;
}

//------------------------------------------------------------------------------
/**
*/
inline PixelFormat::Code
DisplayMode::GetPixelFormat() const
{
    return this->pixelFormat;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayMode::SetAspectRatio(float a)
{
    this->aspectRatio = a;
}

//------------------------------------------------------------------------------
/**
*/
inline float
DisplayMode::GetAspectRatio() const
{
    return this->aspectRatio;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DisplayMode::SetRefreshRate( uint refreshRate )
{
	this->refreshRate = refreshRate;
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
DisplayMode::GetRefreshRate() const
{
	return this->refreshRate;
}
} // namespace CoreGraphics
//------------------------------------------------------------------------------


