#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ImageConverter
    
    Loads an image file of "any" format using DevIL, optionally scales it
    and writes it out as a TGA file. The reason for this is that many
    external tools cannot read PSD files directly.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class ImageConverter
{
public:
    /// error codes
    enum Result
    {
        Success = 1,
        ErrorOpenSrcFile,
        ErrorWriteDstFile,
    };

    /// constructor
    ImageConverter();
    /// destructor
    ~ImageConverter();
    
    /// set source filename
    void SetSrcFile(const Util::String& srcFile);
    /// set target filename
    void SetDstFile(const Util::String& dstFile);
    /// set max width (must be 2^N)
    void SetMaxWidth(SizeT maxWidth);
    /// set max height (must be 2^N)
    void SetMaxHeight(SizeT maxHeight);
    
    /// perform conversion
    Result Convert();
    /// post-convert: get number of channels in the texture (RGB=3, RGBA=4)
    SizeT GetDstNumChannels() const;
    /// post-convert: get image width
    SizeT GetDstWidth() const;
    /// post-convert: get image height
    SizeT GetDstHeight() const;

private:
    Util::String srcFile;
    Util::String dstFile;
    SizeT maxWidth;
    SizeT maxHeight;
    SizeT dstNumChannels;
    SizeT dstWidth;
    SizeT dstHeight;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ImageConverter::SetSrcFile(const Util::String& f)
{
    this->srcFile = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ImageConverter::SetDstFile(const Util::String& f)
{
    this->dstFile = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ImageConverter::SetMaxWidth(SizeT w)
{
    this->maxWidth = w;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ImageConverter::SetMaxHeight(SizeT h)
{
    this->maxHeight = h;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ImageConverter::GetDstNumChannels() const
{
    return this->dstNumChannels;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ImageConverter::GetDstWidth() const
{
    return this->dstWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ImageConverter::GetDstHeight() const
{
    return this->dstHeight;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------


    