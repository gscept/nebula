#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::StreamTextureSaverBase
  
    Allows to save texture data in a standard file format into a stream.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resourcesaver.h"
#include "io/stream.h"
#include "coregraphics/imagefileformat.h"

//------------------------------------------------------------------------------
namespace Base
{
class StreamTextureSaverBase : public Resources::ResourceSaver
{
    __DeclareClass(StreamTextureSaverBase);
public:
    /// constructor
    StreamTextureSaverBase();
    /// destructor
    virtual ~StreamTextureSaverBase();
    
    /// set stream to save to
    void SetStream(const Ptr<IO::Stream>& stream);
    /// get save-stream
    const Ptr<IO::Stream>& GetStream() const;
    /// set file format (default is JPG)
    void SetFormat(CoreGraphics::ImageFileFormat::Code fmt);
    /// get file format
    CoreGraphics::ImageFileFormat::Code GetFormat() const;
    /// set the mip level to save (default is 0, the top level)
    void SetMipLevel(IndexT mipLevel);
    /// get the mip level to save
    IndexT GetMipLevel() const;
    /// called by resource when a save is requested
    virtual bool OnSave();

protected:
    Ptr<IO::Stream> stream;  
    CoreGraphics::ImageFileFormat::Code format;
    IndexT mipLevel;
};

//------------------------------------------------------------------------------
/**
*/
inline void
StreamTextureSaverBase::SetStream(const Ptr<IO::Stream>& s)
{
    this->stream = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
StreamTextureSaverBase::GetStream() const
{
    return this->stream;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StreamTextureSaverBase::SetFormat(CoreGraphics::ImageFileFormat::Code fmt)
{
    this->format = fmt;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ImageFileFormat::Code
StreamTextureSaverBase::GetFormat() const
{
    return this->format;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StreamTextureSaverBase::SetMipLevel(IndexT l)
{
    this->mipLevel = l;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
StreamTextureSaverBase::GetMipLevel() const
{
    return this->mipLevel;
}

} // namespace Base
//------------------------------------------------------------------------------


