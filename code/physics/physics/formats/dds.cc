//------------------------------------------------------------------------------
//  @file dds.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/geometry.h"
#include "physics/formats/formats.h"
#include "io/ioserver.h"
#include "io/stream.h"

#if WITH_NEBULA_PHYSICS_FORMAT_DDS
#include "gliml.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
bool
DdsHeightField(Util::StringAtom resource, HeightFieldInput& out)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(resource.AsString()));
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return false;
    }

    void* heightData = stream->MemoryMap();
    uint heightSize = stream->GetSize();
    n_assert(heightData != nullptr);

    bool parsed = false;

    gliml::context ctx;
    if (ctx.load_dds(heightData, heightSize))
    {
        int depth = ctx.image_depth(0, 0);
        int width = ctx.image_width(0, 0);
        int height = ctx.image_height(0, 0);
        n_assert(depth == 1);

        // decode the format locally instead of going through CoreGraphics::Gliml
        if (ctx.image_format() == GLIML_FOURCC_DXT10 && ctx.image_internal_format() == gliml::DXGI_FORMAT_R16_UNORM)
        {
            const ushort* heightBuffer = (const ushort*)ctx.image_data(0, 0);

            out.width = (SizeT)width;
            out.height = (SizeT)height;
            out.samples.Resize(out.width * out.height);

            // need to flip to be in same orientation with rendering
            for (SizeT i = 0; i < out.width; i++)
            {
                for (SizeT j = 0; j < out.height; j++)
                {
                    out.samples[i * out.width + j] = heightBuffer[j * out.width + i];
                }
            }
            parsed = true;
        }
        else
        {
            n_error("DdsHeightField: '%s' is not an R16 heightmap", resource.Value());
        }
    }

    stream->MemoryUnmap();
    stream->Close();
    return parsed;
}

} // namespace Physics
#endif
