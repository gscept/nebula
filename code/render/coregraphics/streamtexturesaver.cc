//------------------------------------------------------------------------------
//  streamtexturesaver.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/streamtexturesaver.h"
#include "io/ioserver.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
bool
SaveTexture(const Resources::ResourceId& id, const IO::URI& path, IndexT mip, CoreGraphics::ImageFileFormat::Code code)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(path);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        return false;
    }

    const bool result = SaveTexture(id, stream, mip, code);
    stream->Close();
    return result;
}

//------------------------------------------------------------------------------
/**
    Texture export is currently not implemented for Vulkan in this branch.
*/
bool
SaveTexture(const Resources::ResourceId& id, const Ptr<IO::Stream>& stream, IndexT mip, CoreGraphics::ImageFileFormat::Code code)
{
    (void)id;
    (void)stream;
    (void)mip;
    (void)code;
    return false;
}

} // namespace CoreGraphics
